"""Verify relocation and current app staging without running PetaLinux."""

from contextlib import contextmanager
import importlib.util
import json
from pathlib import Path
import shutil
import unittest
import uuid
import zipfile


SOURCE = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("prepare_project", str(SOURCE / "scripts/prepare_project.py"))
PREPARE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PREPARE)


@contextmanager
def scratch():
    base = SOURCE.parent / "build/petalinux-staging-tests"
    base.mkdir(parents=True, exist_ok=True)
    folder = base / uuid.uuid4().hex
    folder.mkdir()
    try:
        yield folder
    finally:
        folder.resolve().relative_to(base.resolve())
        shutil.rmtree(str(folder))


class PrepareProjectTests(unittest.TestCase):
    def test_settings_replacement_requires_exactly_one_key(self):
        self.assertEqual(PREPARE.set_config("A=1\nB=2\n", "A", '"new"'), 'A="new"\nB=2\n')
        with self.assertRaises(ValueError):
            PREPARE.set_config("B=2\n", "A", "1")
        with self.assertRaises(ValueError):
            PREPARE.set_config("A=1\nA=2\n", "A", "3")

    def test_relocation_stages_current_application_and_preserves_custom_dts(self):
        lock = json.loads((SOURCE / "dependencies.lock.json").read_text(encoding="utf-8"))
        config_path = SOURCE / "project-spec/configs/config"
        original_config = config_path.read_bytes()
        dts_rel = Path("project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi")
        original_dts = (SOURCE / dts_rel).read_bytes()
        with scratch() as folder:
            for name in ("first-workspace", "relocated-workspace"):
                output = folder / name
                paths = {item["name"]: output / "sources" / item["name"] for item in lock["dependencies"]}
                hashes = PREPARE.stage_snapshot(SOURCE, output, SOURCE.parent / "sdr_app", lock,
                                               paths, SOURCE / "hardware/system.xsa")
                config = (output / "project-spec/configs/config").read_text(encoding="utf-8")
                expected = config_path.read_text(encoding="utf-8").replace(
                    "@ADI_LINUX_SOURCE@", paths["linux"].as_posix()).replace(
                        "CONFIG_SUBSYSTEM_COPY_TO_TFTPBOOT=y",
                        "# CONFIG_SUBSYSTEM_COPY_TO_TFTPBOOT is not set")
                self.assertEqual(config, expected)
                self.assertNotIn("@ADI_LINUX_SOURCE@", config)
                self.assertIn(paths["linux"].as_posix(), config)
                self.assertIn("# CONFIG_SUBSYSTEM_COPY_TO_TFTPBOOT is not set", config)
                self.assertIn('CONFIG_SUBSYSTEM_SDROOT_DEV="/dev/mmcblk0p2"', config)
                self.assertEqual((output / dts_rel).read_bytes(), original_dts)
                for relative, expected in hashes.items():
                    self.assertEqual(PREPARE.sha256(output / PREPARE.APP_RECIPE / "files" / relative), expected)
                self.assertIn("src/audio_handler.cpp", hashes)
                recipe = output / "project-spec/meta-user/recipes-support/libiio/libiio_git.bbappend"
                self.assertIn("protocol=file", recipe.read_text(encoding="utf-8"))
                self.assertIn(lock["dependencies"][1]["commit"], recipe.read_text(encoding="utf-8"))
                # Simulate PetaLinux regenerating a DTS file during hardware import.
                (output / dts_rel).write_text("/ {};\n", encoding="utf-8")
                PREPARE.restore_user_layer(SOURCE, output, SOURCE.parent / "sdr_app",
                                           {d["name"]: d for d in lock["dependencies"]}, paths)
                self.assertEqual((output / dts_rel).read_bytes(), original_dts)
        self.assertEqual(config_path.read_bytes(), original_config)
        self.assertEqual((SOURCE / dts_rel).read_bytes(), original_dts)

    def test_recipe_paths_reject_delimiters_and_whitespace(self):
        for value in ("/tmp/project name", "/tmp/project;branch=wrong", "/tmp/$other"):
            with self.assertRaises(ValueError):
                PREPARE.valid_build_path(Path(value))
        PREPARE.valid_build_path(Path("/home/rgc/sdr-workspace"))

    def test_hardware_handoff_rejects_wrong_device_or_version(self):
        PREPARE.check_xsa(SOURCE / "hardware/system.xsa")
        with scratch() as folder:
            for version, device in (("2021.2", "7z020"), ("2019.2", "7z035")):
                path = folder / "wrong.xsa"
                xml = '<EDKSYSTEM VIVADOVERSION="{}"><SYSTEMINFO ARCH="zynq" DEVICE="{}" PACKAGE="clg400"/></EDKSYSTEM>'.format(version, device)
                with zipfile.ZipFile(str(path), "w") as archive:
                    archive.writestr("system.hwh", xml)
                with self.assertRaises(ValueError):
                    PREPARE.check_xsa(path)


if __name__ == "__main__":
    unittest.main()
