#!/usr/bin/env python3
"""Check PetaLinux import inputs without installing PetaLinux; Python 3.6+."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


SOURCE = Path(__file__).resolve().parents[1]
APP = SOURCE.parent / "sdr_app"
KEY = "CONFIG_SUBSYSTEM_COMPONENT_LINUX__KERNEL_NAME_EXT_LOCAL_SRC_PATH"


def digest(path, text=False):
    data = path.read_bytes()
    if text:
        data = data.replace(b"\r\n", b"\n")
    return hashlib.sha256(data).hexdigest()


def check(prepared=None):
    errors = []
    lock = json.loads((SOURCE / "dependencies.lock.json").read_text(encoding="utf-8"))
    dependencies = {item["name"]: item for item in lock["dependencies"]}
    if set(dependencies) != {"linux", "libiio", "libad9361-iio"}:
        errors.append("Expected all three locked ADI dependencies")
    for item in dependencies.values():
        if not re.fullmatch(r"[0-9a-f]{40}", item["commit"]):
            errors.append("Invalid locked commit: " + item["name"])
        if item["local_source_patches"]:
            errors.append("Unexpected source patches: owner confirmed configuration-only changes")
    marker = (SOURCE / ".petalinux/metadata").read_text(encoding="utf-8")
    if marker.strip() != "PETALINUX_VER=2019.2":
        errors.append("Missing PetaLinux 2019.2 version marker")
    config = (SOURCE / "project-spec/configs/config").read_text(encoding="utf-8")
    if KEY + '="@ADI_LINUX_SOURCE@"' not in config:
        errors.append("Expected relocatable kernel source placeholder")
    rootfs = (SOURCE / "project-spec/configs/rootfs_config").read_text(encoding="utf-8")
    for name in ("sdr-app", "rndis", "libiio", "libad9361-iio"):
        if "CONFIG_" + name + "=y" not in rootfs.splitlines():
            errors.append("Required rootfs package is disabled: " + name)
    baseline = SOURCE / lock["baseline_hardware"]["path"]
    if digest(baseline) != lock["baseline_hardware"]["sha256"]:
        errors.append("Original hardware snapshot checksum differs")
    audit = json.loads((SOURCE / "docs/import-audit.json").read_text(encoding="utf-8"))
    for item in audit["preserved_inputs"]:
        path = SOURCE / item["path"]
        if not path.is_file() or digest(path, item["normalize_lf"]) != item["sha256"]:
            errors.append("Preserved input differs: " + item["path"])

    app_recipe = SOURCE / "project-spec/meta-user/recipes-apps/sdr-app"
    make_sources = set(re.findall(r"src/[A-Za-z0-9_]+\.cpp", (APP / "Makefile").read_text(encoding="utf-8")))
    recipe_sources = set(re.findall(r"src/[A-Za-z0-9_]+\.cpp", (app_recipe / "sdr-app-sources.inc").read_text(encoding="utf-8")))
    if not make_sources or make_sources != recipe_sources:
        errors.append("PetaLinux app source list differs from sdr_app/Makefile")
    for relative in recipe_sources:
        if not (APP / relative).is_file():
            errors.append("Missing application source: " + relative)
    if (app_recipe / "files/sdr_app").exists():
        errors.append("Obsolete precompiled application must not be an import input")

    recipes = list((SOURCE / "project-spec/meta-user").rglob("*.bb"))
    recipes += list((SOURCE / "project-spec/meta-user").rglob("*.bbappend"))
    for path in recipes:
        text = path.read_text(encoding="utf-8")
        if "${AUTOREV}" in text or "/home/rgc/work/" in text:
            errors.append("Unpinned or host-specific recipe: " + str(path))
        blocks = re.findall(r'(?ms)^SRC_URI(?:_[A-Za-z0-9]+)?\s*(?:\+=|=)\s*"([^"]*)"', text)
        for block in blocks:
            for relative in re.findall(r"file://([^\s;\\]+)", block):
                if "${" in relative:
                    continue
                if path.parent == app_recipe and relative in ("src", "include"):
                    continue  # Staged from the sibling application by prepare_project.py.
                if not (path.parent / "files" / relative).exists():
                    errors.append("Missing recipe input: {} -> {}".format(path.name, relative))
    for name, relative in (
            ("libiio", "libiio/libiio_git.bbappend"),
            ("libad9361-iio", "libad9361-iio/libad9361-iio_git.bb")):
        text = (SOURCE / "project-spec/meta-user/recipes-support" / relative).read_text(encoding="utf-8")
        if 'SRCREV = "' + dependencies[name]["commit"] + '"' not in text:
            errors.append("Recipe revision differs from lock: " + name)

    if prepared:
        report = json.loads((prepared / "prepared-inputs.json").read_text(encoding="utf-8"))
        config = (prepared / "project-spec/configs/config").read_text(encoding="utf-8")
        for item in dependencies.values():
            path = prepared / "sources" / item["name"]
            head = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"]).decode().strip()
            if head != item["commit"]:
                errors.append("Prepared dependency revision differs: " + item["name"])
        if KEY + '="' + (prepared / "sources/linux").as_posix() + '"' not in config:
            errors.append("Prepared kernel source path differs")
        for relative, expected in report["application_sha256"].items():
            target = prepared / "project-spec/meta-user/recipes-apps/sdr-app/files" / relative
            if not target.is_file() or digest(target) != expected:
                errors.append("Staged application source differs: " + relative)
        if digest(prepared / "hardware/system.xsa") != report["xsa_sha256"]:
            errors.append("Prepared hardware snapshot differs")
        original = SOURCE / "project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi"
        restored = prepared / original.relative_to(SOURCE)
        if digest(original, True) != digest(restored, True):
            errors.append("Prepared device tree differs from the custom source")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepared", type=Path, help="Also check a prepared Ubuntu workspace")
    args = parser.parse_args()
    errors = check(args.prepared.resolve() if args.prepared else None)
    for error in errors:
        print("ERROR: " + error, file=sys.stderr)
    if errors:
        return 1
    print("PASS: PetaLinux inputs, preserved configuration, recipe files and pinned ADI revisions.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
