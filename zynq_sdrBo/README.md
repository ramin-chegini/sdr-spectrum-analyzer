# PetaLinux 2019.2 — P201PRO

Source inputs and workspace preparation for the PUZHI PZSDR P201PRO
(`xc7z020clg400-2`, AD9361), using **Ubuntu 18.04 / PetaLinux 2019.2**.

[راهنمای اجرای فارسی](README.fa.md)

**Status: prepared for Ubuntu validation.** The original board project previously
worked, and the hardware migration reached bitstream generation. This new
source-based PetaLinux workspace has not yet been built or booted. Static input
checks and staging tests are separate from a PetaLinux image build.

## What is stored here

- Original system/rootfs configuration and `.petalinux/metadata` version marker.
- Custom device tree, kernel configuration, U-Boot settings and application recipes.
- Recipe firmware inputs and their existing license notices.
- Original `hardware/system.xsa`, identified by checksum in the dependency lock.
- Exact revisions of Linux ADI, libiio and libad9361-iio.
- Scripts to create a separate workspace and stage the repository's current app.

No generated build trees, rootfs images, usage tokens, nested Git repositories,
or precompiled `sdr_app` executable are included in this component. The original
working project is not modified by the preparation command.

## Versioned dependencies

| Source | Branch | Locked revision |
| --- | --- | --- |
| Linux ADI 4.19.0 | `2019_R2` | `89a181de04e3bf49c4c06df417b71bc8dbb04ab9` |
| libiio | `2019_R2` | `fb64aca8adcaa607b482091b23b8edf10e5c1ad8` |
| libad9361-iio | `2019_R2` | `be7eb026436f6b68eb718a01cdd249acd85b05c5` |

The owner confirmed that changes were confined to configuration, device tree and
PetaLinux recipes. No manual kernel source patch is required. The preparation
script creates independent Git clones and checks out the locked commits. It can
use existing local Git repositories or fetch the original upstream repositories.
It does not copy the dirty Windows kernel worktree: that copy lost symlinks and
contains Windows filename/case conflicts.

## Prepare on Ubuntu

Extract/clone the complete repository, including sibling `sdr_app/`. Work on a
native Linux filesystem, with no spaces in the source/output paths. Source your
installed PetaLinux **2019.2** `settings.sh` first. Python 3.6+ and Git are required.

From the repository root:

```bash
python3 zynq_sdrBo/scripts/check_project.py
python3 -m unittest discover -s zynq_sdrBo/tests -v
python3 zynq_sdrBo/scripts/prepare_project.py \
  --output /home/rgc/work/sdr-clean/zynq_sdrBo \
  --local-sources /home/rgc/work/Analogdevicesinc \
  --xsa /home/rgc/Desktop/p201pro-current.xsa
```

`--output` must be a **new** directory, not the original PetaLinux project.
`--local-sources` names the existing parent of `linux`, `libiio` and
`libad9361-iio` Git repositories. Omit it to fetch those repositories from GitHub.
Local source repositories must contain the locked commits and `2019_R2` branch;
their worktrees are not reset or edited. Fresh clones occupy additional disk space.

The script:

1. Creates a Zynq project and checks its PetaLinux 2019.2 version marker.
2. Checks out all three locked dependencies under the new project's `sources/`.
3. Resolves the kernel source placeholder in the generated system configuration.
4. Uses the new local Git clones for the generated library recipes.
5. Imports the selected XSA, restores the custom layer, and stages current C++
   sources/headers into the generated `sdr-app` recipe.
6. Runs silent configuration and writes `prepared-inputs.json` with the hardware
   and application input hashes.

SD boot settings are retained. Automatic host-side deployment to `/tftpboot` is
disabled in the generated workspace, avoiding a machine-specific host directory.
Other PetaLinux/Yocto downloads and caches are still required as usual; local ADI
clones alone do not make a full first build offline.

## Choose the hardware handoff explicitly

Use `--xsa` for a handoff exported from the actual FPGA design to boot. In Vivado
2019.2, with the successfully built project open, a new export can be made with:

```tcl
open_run [current_run -implementation]
write_hw_platform -fixed -include_bit /home/rgc/Desktop/p201pro-current.xsa
```

The script checks that the XSA contains a Vivado 2019.2 Zynq-7020 / clg400 handoff.
It does not prove every register, interrupt or pin mapping matches the custom DTS;
that remains part of Ubuntu/board validation.

To reproduce the **original PetaLinux hardware snapshot**, explicitly replace
`--xsa ...` with `--use-baseline-xsa`. Its embedded bitstream is named
`Handoff_Ver1.1.bit`. Its correspondence to the newly rebuilt FPGA is unconfirmed;
the baseline is kept to preserve the original input, not presented as a new export.

## Build and package

After preparation succeeds:

```bash
python3 zynq_sdrBo/scripts/check_project.py \
  --prepared /home/rgc/work/sdr-clean/zynq_sdrBo
cd /home/rgc/work/sdr-clean/zynq_sdrBo
petalinux-build
petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf \
  --fpga images/linux/system.bit --u-boot --force
```

The existing U-Boot customization loads **`zImage` and `system.dtb`** from SD
partition 1 and uses `/dev/mmcblk0p2` for the root filesystem. Boot files include
`BOOT.BIN`, `zImage` and `system.dtb`; install the generated `rootfs.tar.gz` contents
on the Linux root partition using the board's established SD preparation process.
The presence of an `image.ub` output does not change the custom U-Boot command.

On the board, verify the AD9361 IIO devices, UIO names, DMA reservations, networking,
and application before marking the import boot-tested. Useful checks include:

```bash
dmesg | grep -Ei 'ad9361|iio|uio|dma'
ls /sys/bus/iio/devices
ls /sys/class/uio
sdr_app ad9361 status
/etc/init.d/sdr-app status
```

## App integration and preserved settings

The `sdr-app` recipe now compiles the same eight C++ translation units as
`sdr_app/Makefile`, with the PetaLinux compiler/sysroot and built libiio dependency.
`src/` and `include/` are staged from the repository snapshot; there is no second
permanent copy of application source in Git. Rerun preparation into a new workspace
after changing the application to stage the new revision. The existing startup
helper and its recipe license metadata are retained; this import assigns no new
repository-wide license to the application.

The original custom DTS is restored after hardware import. Current settings include:

| Setting | Preserved value |
| --- | --- |
| Reserved DMA memory | `0x3F000000`, 16 MiB, `no-map` |
| PSD / audio DMA | `0x40400000` / `0x40410000` |
| ADI receive DMA | `0x7C400000` |
| AD9361 interface | `0x79020000` |
| Ethernet configuration | `192.168.1.10/24` |
| RNDIS USB address | `192.168.2.10/24`, MTU 9000 |
| Root device | `/dev/mmcblk0p2` |

Both original defconfig files are retained in their existing locations. The recipe's
`file://defconfig` resolves to `recipes-kernel/linux/files/defconfig`.
`docs/original-final-kernel.config` preserves the previous full Linux/arm 4.19.0
configuration as a reference; it is not silently substituted as a new defconfig.
Compare the effective new kernel configuration during the Ubuntu build.

`docs/import-audit.json` records checksums for 57 preserved inputs. Generated
configuration is kept separate so host paths and build state do not enter Git.

## References

- [PetaLinux UG1144 v2019.2](https://docs.amd.com/api/khub/documents/skTpbGY85XboY_5ulnLMpw/content): version control and project/hardware configuration.
- [Vivado UG835 v2019.2](https://docs.amd.com/api/khub/documents/0UEc0Pr7HVthl2nugR8iNA/content): `write_hw_platform`.
