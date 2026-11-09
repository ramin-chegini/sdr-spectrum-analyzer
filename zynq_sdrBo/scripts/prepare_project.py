#!/usr/bin/env python3
"""Recreate a separate PetaLinux 2019.2 workspace; Python 3.6+, Linux only."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile


SOURCE = Path(__file__).resolve().parents[1]
APPLICATION = SOURCE.parent / "sdr_app"
KERNEL_KEY = "CONFIG_SUBSYSTEM_COMPONENT_LINUX__KERNEL_NAME_EXT_LOCAL_SRC_PATH"
APP_RECIPE = Path("project-spec/meta-user/recipes-apps/sdr-app")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_text(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def copy_file(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(str(source), str(target))


def set_config(text, key, value):
    pattern = r"(?m)^(?:" + re.escape(key) + r"=.*|# " + re.escape(key) + r" is not set)$"
    replacement = "# " + key + " is not set" if value is None else key + "=" + value
    text, count = re.subn(pattern, lambda match: replacement, text)
    if count != 1:
        raise ValueError("Expected one configuration setting: " + key)
    return text


def valid_build_path(path):
    # These paths also enter Kconfig strings and BitBake file:// URLs.
    if re.search(r"[\s;\"'$\{\}]", str(path)):
        raise ValueError("Use a path without whitespace or recipe delimiters: " + str(path))


def check_xsa(path):
    with zipfile.ZipFile(str(path)) as archive:
        if "system.hwh" not in archive.namelist():
            raise ValueError("XSA must contain the system.hwh hardware handoff")
        if archive.testzip() is not None:
            raise ValueError("Corrupted XSA archive")
        handoff = ET.fromstring(archive.read("system.hwh"))
        info = handoff.find("SYSTEMINFO")
        if handoff.get("VIVADOVERSION") != "2019.2":
            raise ValueError("Export the hardware handoff with Vivado 2019.2")
        if info is None or (info.get("ARCH"), info.get("DEVICE"), info.get("PACKAGE")) != (
                "zynq", "7z020", "clg400"):
            raise ValueError("Expected the P201PRO Zynq-7020 / clg400 hardware handoff")


def restore_user_layer(source, output, application, dependencies, source_paths):
    """Restore custom inputs after hardware import, which may regenerate DTS files."""
    for item in (source / "project-spec/meta-user").rglob("*"):
        if item.is_file():
            copy_file(item, output / item.relative_to(source))
    # A fresh checkout of each dependency lives inside this generated workspace.
    for name, relative in (
            ("libiio", "recipes-support/libiio/libiio_git.bbappend"),
            ("libad9361-iio", "recipes-support/libad9361-iio/libad9361-iio_git.bb")):
        recipe = output / "project-spec/meta-user" / relative
        text = recipe.read_text(encoding="utf-8")
        url = "git://{};protocol=file;branch={}".format(
            source_paths[name].as_posix(), dependencies[name]["branch"])
        text, count = re.subn(r'(?m)^SRC_URI = "[^"]*"$',
                              lambda match: 'SRC_URI = "' + url + '"', text)
        if count != 1:
            raise ValueError("Expected one pinned SRC_URI in " + str(recipe))
        write_text(recipe, text)

    hashes = {}
    for directory in ("src", "include"):
        for item in (application / directory).rglob("*"):
            if item.is_file():
                relative = item.relative_to(application)
                copy_file(item, output / APP_RECIPE / "files" / relative)
                hashes[relative.as_posix()] = sha256(item)
    if not hashes:
        raise ValueError("Application sources are missing: " + str(application))
    return hashes


def stage_snapshot(source, output, application, lock, source_paths, xsa):
    """Stage project inputs only; this function does not invoke PetaLinux or Git."""
    for relative in ("config.project", "project-spec/attributes",
                     "project-spec/configs/rootfs_config"):
        copy_file(source / relative, output / relative)
    text = (source / "project-spec/configs/config").read_text(encoding="utf-8")
    text = set_config(text, KERNEL_KEY, '"' + source_paths["linux"].as_posix() + '"')
    # SD boot is preserved. Host-side copying to /tftpboot is unnecessary and
    # would require a machine-specific writable directory.
    text = set_config(text, "CONFIG_SUBSYSTEM_COPY_TO_TFTPBOOT", None)
    write_text(output / "project-spec/configs/config", text)
    copy_file(xsa, output / "hardware/system.xsa")
    dependencies = {item["name"]: item for item in lock["dependencies"]}
    return restore_user_layer(source, output, application, dependencies, source_paths)


def run(command, cwd=None):
    print("+ " + " ".join(str(value) for value in command), flush=True)
    subprocess.run([str(value) for value in command], cwd=str(cwd) if cwd else None, check=True)


def checkout_dependency(dependency, destination, local_sources):
    origin = str(local_sources / dependency["name"]) if local_sources else dependency["url"]
    if local_sources and not (local_sources / dependency["name"]).is_dir():
        raise ValueError("Missing local Git repository: " + origin)
    # Copy Git objects and check out the locked revision. Never copy a dirty
    # Windows kernel worktree, and never reset or modify the supplied repository.
    run(["git", "-c", "core.autocrlf=false", "clone", "--no-hardlinks",
         "--single-branch", "--branch", dependency["branch"], "--no-checkout",
         origin, destination])
    run(["git", "-C", destination, "config", "core.autocrlf", "false"])
    run(["git", "-C", destination, "-c", "core.autocrlf=false", "checkout",
         "--detach", dependency["commit"]])
    head = subprocess.check_output(["git", "-C", str(destination), "rev-parse", "HEAD"]).decode().strip()
    if head != dependency["commit"]:
        raise ValueError("Dependency checkout differs from lock: " + dependency["name"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path,
                        help="New PetaLinux project directory on the Linux filesystem")
    parser.add_argument("--local-sources", type=Path,
                        help="Optional existing parent of linux, libiio and libad9361-iio Git repositories")
    hardware = parser.add_mutually_exclusive_group(required=True)
    hardware.add_argument("--xsa", type=Path, help="XSA exported from the hardware design to boot")
    hardware.add_argument("--use-baseline-xsa", action="store_true",
                          help="Explicitly use the original PetaLinux handoff, not a newly verified FPGA export")
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("Run preparation in Ubuntu 18.04 with PetaLinux 2019.2, on a Linux filesystem.")
    output = args.output.expanduser().resolve()
    valid_build_path(output)
    if output.exists():
        parser.error("Choose a new output directory; existing projects are not overwritten.")
    local_sources = args.local_sources.expanduser().resolve() if args.local_sources else None
    if local_sources:
        valid_build_path(local_sources)
    if not os.environ.get("PETALINUX"):
        parser.error("Source your PetaLinux 2019.2 settings.sh before running this script.")
    for command in ("petalinux-create", "petalinux-config", "git"):
        if not shutil.which(command):
            parser.error("Required command is not on PATH: " + command)
    lock = json.loads((SOURCE / "dependencies.lock.json").read_text(encoding="utf-8"))
    run([sys.executable, SOURCE / "scripts/check_project.py"])
    xsa = args.xsa.expanduser().resolve() if args.xsa else SOURCE / lock["baseline_hardware"]["path"]
    check_xsa(xsa)
    output.parent.mkdir(parents=True, exist_ok=True)
    run(["petalinux-create", "-t", "project", "--template", "zynq", "--name", output.name], output.parent)
    metadata = (output / ".petalinux/metadata").read_text(encoding="utf-8")
    if "PETALINUX_VER=2019.2" not in metadata.splitlines():
        raise ValueError("The created project is not PetaLinux 2019.2. Choose the correct tool environment.")
    source_paths = {}
    (output / "sources").mkdir()
    for dependency in lock["dependencies"]:
        target = output / "sources" / dependency["name"]
        checkout_dependency(dependency, target, local_sources)
        source_paths[dependency["name"]] = target
    hashes = stage_snapshot(SOURCE, output, APPLICATION, lock, source_paths, xsa)
    run(["petalinux-config", "--get-hw-description=" + str(output / "hardware"), "--silentconfig"], output)
    # Reapply the custom layer and current application after hardware generation.
    dependencies = {item["name"]: item for item in lock["dependencies"]}
    hashes = restore_user_layer(SOURCE, output, APPLICATION, dependencies, source_paths)
    run(["petalinux-config", "--silentconfig"], output)
    config = (output / "project-spec/configs/config").read_text(encoding="utf-8")
    if KERNEL_KEY + '="' + source_paths["linux"].as_posix() + '"' not in config:
        raise ValueError("PetaLinux changed the kernel source selection; inspect the generated configuration.")
    report = {"petalinux_version": "2019.2", "xsa_sha256": sha256(xsa),
              "used_original_baseline_xsa": args.use_baseline_xsa,
              "dependencies": lock["dependencies"], "application_sha256": hashes,
              "status": "prepared; build and board boot still required"}
    write_text(output / "prepared-inputs.json", json.dumps(report, indent=2) + "\n")
    print("Prepared: " + str(output))
    print("Next: cd {} && petalinux-build".format(output))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile, ET.ParseError) as error:
        print("ERROR: " + str(error), file=sys.stderr)
        sys.exit(1)
