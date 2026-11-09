"""Regressions found while importing the source-only bundle in Vivado 2019.2."""

import importlib.util
import json
from pathlib import Path
import shutil
import unittest
from unittest.mock import patch
import uuid
from contextlib import contextmanager


PROJECT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "check_adi_vendor", str(PROJECT / "scripts/check_adi_vendor.py"))
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


@contextmanager
def temporary_vendor():
    # Use normal mkdir permissions on shared Windows/Linux workspaces.
    temp_root = PROJECT / "build/portability_tests"
    temp_root.mkdir(parents=True, exist_ok=True)
    folder = temp_root / uuid.uuid4().hex
    folder.mkdir()
    try:
        yield folder
    finally:
        folder.resolve().relative_to(temp_root.resolve())
        shutil.rmtree(str(folder))


class PortabilityChecks(unittest.TestCase):
    def test_fifo_interfaces_include_both_bus_and_abstraction_definitions(self):
        with temporary_vendor() as temp:
            vendor = Path(temp)
            paths = ["library/axi_dmac/component.xml"] + [
                "library/interfaces/" + name + ".xml"
                for name in ("fifo_rd", "fifo_rd_rtl", "fifo_wr", "fifo_wr_rtl")]
            for name in paths:
                target = vendor / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(str(CHECK.VENDOR / name), str(target))
            tracked = {(vendor / name).resolve() for name in paths}
            with patch.object(CHECK, "VENDOR", vendor):
                errors = []
                self.assertEqual(CHECK.check_interface_definitions(errors, tracked), 4)
                self.assertEqual(errors, [])
                for name, kind in (("fifo_wr", "busDefinition"),
                                   ("fifo_wr_rtl", "abstractionDefinition")):
                    target = vendor / "library/interfaces" / (name + ".xml")
                    original = target.read_bytes()
                    target.unlink()
                    try:
                        errors = []
                        CHECK.check_interface_definitions(errors, tracked)
                        self.assertTrue(any("Missing " + kind in e and
                                            "analog.com:interface:" + name + ":1.0" in e
                                            for e in errors), errors)
                    finally:
                        target.write_bytes(original)

    def test_embedded_old_ps7_preset_path_is_rejected(self):
        bd = PROJECT / "pzp351.srcs/sources_1/bd/system/system.bd"
        design = json.loads(bd.read_text(encoding="utf-8"))["design"]
        errors = []
        CHECK.check_bd_parameters(errors, design)
        self.assertEqual(errors, [])
        params = design["components"]["sys_ps7"]["parameters"]
        self.assertEqual(params["PCW_IMPORT_BOARD_PRESET"]["value"], "None")
        for path in ("/home/rgc/work/hw/ps7_init.tcl", r"C:\old\ps7.xml"):
            params["PCW_IMPORT_BOARD_PRESET"]["value"] = path
            errors = []
            CHECK.check_bd_parameters(errors, design)
            self.assertEqual(len(errors), 1)
            self.assertIn("sys_ps7.PCW_IMPORT_BOARD_PRESET", errors[0])


if __name__ == "__main__":
    unittest.main()
