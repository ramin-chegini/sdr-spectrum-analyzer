#!/usr/bin/env python3
"""Check the frozen ADI IP repository without Vivado (Python 3.6+)."""

import hashlib
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


PROJECT = Path(__file__).resolve().parents[1]
VENDOR = PROJECT / "third_party" / "adi_hdl"
NS = {"s": "http://www.spiritconsortium.org/XMLSchema/SPIRIT/1685-2009",
      "x": "http://www.xilinx.com"}


def inside(path, root):
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def exact_case(path, root):
    """Detect Linux filename case errors even when this runs on Windows."""
    if not inside(path, root):
        return False
    relative = path.resolve().relative_to(root.resolve())
    folder = root.resolve()
    for part in relative.parts:
        if not folder.is_dir() or part not in {p.name for p in folder.iterdir()}:
            return False
        folder = folder / part
    return True


def check_interface_definitions(errors, tracked):
    definitions = {}
    trees = []
    for path in (VENDOR / "library").rglob("*.xml"):
        tree = ET.parse(str(path))
        trees.append((path, tree))
        kind = tree.getroot().tag.rsplit("}", 1)[-1]
        if kind not in ("busDefinition", "abstractionDefinition"):
            continue
        key = tuple(tree.findtext("s:" + name, namespaces=NS)
                    for name in ("vendor", "library", "name", "version"))
        if (kind, key) in definitions:
            errors.append("Duplicate interface definition: " + ":".join(key))
        definitions[kind, key] = path
        if path.resolve() not in tracked:
            errors.append("Interface definition absent from manifest: " + str(path))

    # Bus/abstraction definitions are independent catalog entries, not files
    # declared by the IP's component.xml. Include disabled interfaces as well.
    for path, tree in trees:
        for tag, kind in (("busType", "busDefinition"),
                          ("extends", "busDefinition"),
                          ("abstractionType", "abstractionDefinition")):
            for node in tree.findall(".//s:" + tag, NS):
                key = tuple(node.get("{" + NS["s"] + "}" + name)
                            for name in ("vendor", "library", "name", "version"))
                if key[0] == "analog.com" and (kind, key) not in definitions:
                    errors.append("Missing {}: {} (used by {})".format(
                        kind, ":".join(key), path.name))
    return len(definitions)


def check_bd_parameters(errors, design):
    for instance, component in design["components"].items():
        for name, setting in component.get("parameters", {}).items():
            value = setting.get("value") if isinstance(setting, dict) else setting
            if isinstance(value, str) and re.match(r"^(?:/|[A-Za-z]:[\\/]|\\\\)", value):
                errors.append("External BD parameter: {}.{} = {}".format(instance, name, value))


def main():
    errors = []
    manifest = json.loads((VENDOR / "manifest.json").read_text(encoding="utf-8"))
    tracked = set()
    for item in manifest["files"]:
        path = VENDOR / item["path"]
        tracked.add(path.resolve())
        if not inside(path, VENDOR) or not path.is_file() or not exact_case(path, VENDOR):
            errors.append("Missing or outside vendor tree: " + item["path"])
        elif hashlib.sha256(path.read_bytes()).hexdigest() != item["sha256"]:
            errors.append("SHA256 differs: " + item["path"])

    cores = {}
    for path in (VENDOR / "library").rglob("component.xml"):
        tree = ET.parse(str(path))
        key = tuple(tree.findtext("s:" + name, namespaces=NS)
                    for name in ("vendor", "library", "name", "version"))
        if key in cores:
            errors.append("Duplicate core: " + ":".join(key))
        cores[key] = path
        for node in tree.findall(".//s:file/s:name", NS):
            target = (path.parent / node.text).resolve()
            if not inside(target, VENDOR) or not target.is_file() or not exact_case(target, VENDOR):
                errors.append("Broken IP file: {} -> {}".format(path, node.text))
            elif target not in tracked:
                errors.append("IP file absent from manifest: " + str(target))

    for key, path in cores.items():
        for node in ET.parse(str(path)).findall(".//x:subCoreRef/x:componentRef", NS):
            dep = tuple(node.get("{" + NS["x"] + "}" + name)
                        for name in ("vendor", "library", "name", "version"))
            if dep[0] == "analog.com" and dep not in cores:
                errors.append("Missing subcore: " + ":".join(dep))

    # This runtime helper is sourced by axi_ad9361/bd/bd.tcl rather than
    # declared in component.xml; keep it alongside the library directory.
    helper = VENDOR / "library/scripts/adi_xilinx_device_info_enc.tcl"
    if not helper.is_file() or helper.resolve() not in tracked:
        errors.append("Missing AD9361 device information helper")

    interface_count = check_interface_definitions(errors, tracked)
    bd = PROJECT / "pzp351.srcs/sources_1/bd/system/system.bd"
    design = json.loads(bd.read_text(encoding="utf-8"))["design"]
    check_bd_parameters(errors, design)
    for instance, component in design["components"].items():
        vlnv = component.get("vlnv", "")
        if vlnv.startswith("analog.com:") and tuple(vlnv.split(":")) not in cores:
            errors.append("BD IP missing from vendor tree: " + instance + " " + vlnv)

    xpr = ET.parse(str(PROJECT / "pzp351.xpr"))
    for node in xpr.findall(".//FileSets/FileSet/File"):
        value = node.get("Path")
        value = value.replace("$PSRCDIR", str(PROJECT / "pzp351.srcs"))
        value = value.replace("$PPRDIR", str(PROJECT))
        path = Path(value)
        if "$" in value or not inside(path, PROJECT) or not path.is_file() or not exact_case(path, PROJECT):
            errors.append("Broken or external XPR source: " + node.get("Path"))
    for node in xpr.findall(".//Configuration/Option[@Name='IPRepoPath']"):
        value = node.get("Val").replace("$PPRDIR", str(PROJECT))
        if not inside(Path(value), PROJECT) or not Path(value).is_dir():
            errors.append("Broken or external IP repository: " + node.get("Val"))

    # Check the other local IP packages as well: these are needed on a
    # clean checkout even though they are not part of the ADI snapshot.
    for path in (PROJECT / "ip_repo").rglob("component.xml"):
        for node in ET.parse(str(path)).findall(".//s:file/s:name", NS):
            target = path.parent / node.text
            if not inside(target, PROJECT) or not target.is_file() or not exact_case(target, PROJECT):
                errors.append("Broken local IP file: {} -> {}".format(path, node.text))

    if errors:
        for error in errors:
            print("ERROR: " + error, file=sys.stderr)
        return 1
    print("PASS: {} ADI cores, {} interface definitions, {} exact file hashes; "
          "IP dependencies, BD parameters and XPR sources are local.".format(
              len(cores), interface_count, len(manifest["files"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
