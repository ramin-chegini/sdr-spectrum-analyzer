# P201PRO local Vivado project

Target: PUZHI PZSDR P201PRO, `xc7z020clg400-2`, AD9361, Vivado **2019.2**.
The intended build host is Ubuntu 18.04 with Vivado 2019.2 installed.

**Validation:** bitstream generation from the local source bundle was confirmed
by the project owner in Ubuntu / Vivado 2019.2 on 2026-11-09.

Copy or clone **this entire P201Pro directory**. No sibling `Analogdevicesinc`
checkout, Git submodule, download, or `ADI_HDL_DIR` environment variable is
needed by the active project or its recreation scripts. Vivado and its Xilinx
IP catalog must still be installed, including any required Xilinx IP licenses.

## Local ADI sources

`third_party/adi_hdl` contains the exact required subset of the supplied
`hdl_2019_r2` working tree, not a newer upstream release:

- `axi_ad9361`, `axi_dmac`, and `util_cpack2` are used directly by the BD.
- `util_axis_fifo` and `util_cdc` are required subcores.
- FIFO bus and RTL abstraction definitions are included in `library/interfaces`.
- Shared HDL, headers, constraints, GUI/BD callbacks, and the AD9361 device
  information helper are included with their original relative layout.
- The four directly added ADI HDL files and the upstream license files are
  included too. Copyright notices remain in the sources.

`third_party/adi_hdl/manifest.json` records the upstream base commit, branch,
selected working tree changes, and SHA256 for all 113 copied files. One selected
file (`library/axi_ad9361/bd/bd.tcl`) was generated locally and untracked in the
supplied ADI checkout; it is preserved here as required by the packaged IP.
The snapshot is about 1.36 MB and contains no ADI build caches or nested Git repo.

## Verify on Ubuntu

From this directory, after loading your Vivado 2019.2 environment:

```bash
python3 scripts/check_adi_vendor.py
vivado -mode batch -source scripts/validate_adi.tcl
```

The Python check supports Python 3.6+ and needs no third party packages. It checks
the snapshot hashes, packaged file paths, subcore dependencies, interface
definitions, embedded BD parameter paths, the BD's ADI IPs, local project source
paths, custom IP package files, and filename case. Regression checks can be run
with `python3 -m unittest discover -s tests -v`.

The Vivado check rebuilds the local IP catalog, validates the existing BD, and
writes `adi_ip_status.rpt`. Review that report for locked or missing IPs. To
regenerate output products as well:

```bash
vivado -mode batch -source scripts/validate_adi.tcl -tclargs --generate
```

You can also open `pzp351.xpr` directly in Vivado 2019.2. Its two IP repository
paths now point to `third_party/adi_hdl/library` and `ip_repo` inside this folder.
Do not upgrade the IPs when checking this migration.

## Recreate and build without the old run caches

The authoritative BD is `pzp351.srcs/sources_1/bd/system/system.bd`. The active
`create_project.tcl` uses this current Z7020 design and creates a separate project
under `build/pzp351`, leaving the source BD intact:

```bash
vivado -mode batch -source create_project.tcl -tclargs --generate
```

For a full synthesis, implementation, and bitstream run, choose an empty output
directory (an existing nonempty directory is rejected):

```bash
vivado -mode batch -source create_project.tcl \
  -tclargs --output_dir build/full_check --build --jobs 4
```

`system_project.tcl` is a compatibility entry point for the same script.
`system_bd.tcl` imports the current BD into the new project. The old three scripts
are preserved as non-executable `.tcl.txt` records in `docs/legacy`. They include
historical external paths and an older ADI reference design; they are not build
inputs. In particular, the old `system_project.tcl` targeted a Z7035 and the old
exported `create_project.tcl` did not match the latest BD.

## Fixes after the first Ubuntu import

The first Ubuntu Vivado 2019.2 import exposed two omissions in the original
source-only bundle. Both are addressed in the v2 bundle:

- The ADI `fifo_rd` / `fifo_wr` bus and RTL abstraction XML definitions are now
  local. These are separate IP catalog entries, not file references in the IP's
  `component.xml`. Their absence caused `BD 41-51` / `BD 41-49` messages.
- `sys_ps7.PCW_IMPORT_BOARD_PRESET` contained the obsolete absolute path
  `/home/rgc/work/hw/ps7_init.tcl`. It is now `None` in the source BD and existing
  PS7 XCI, disabling re-import of that preset. The other **567 PS7 parameters**
  (including DDR, MIO, clocks and HP ports), connections, and address assignments
  are unchanged. The original BD/XCI are preserved as `.txt` records in
  `docs/legacy`. The old preset path caused `PS7-1` and the subsequent PS7 port
  and address-segment errors.

Extract the v2 source bundle to a new directory and recreate there. If reusing
an existing working directory, choose a fresh `--output_dir`, since the failed
first import already created `build/pzp351`. Existing output is not overwritten.
For the Vivado Tcl Console, from the updated P201Pro directory:

```tcl
close_project
set argv {--output_dir build/pzp351_v2 --generate}
source create_project.tcl
set argv {}
```

Use `close_project` only if a project is currently open. `BD 41-434` lines are
informational during recreation of missing generated XCI files; check the actual
ERROR / CRITICAL WARNING messages and the final validation result.

Keep `pzp351.srcs`, `ip_repo`, `coe`, `third_party`, the top-level HDL/XDC/XPR/Tcl
files, and `scripts` in Git. Some required coefficient and wrapper files are
currently under `pzp351.srcs`; do not remove or ignore that directory. The supplied
`.gitignore` excludes Vivado caches, run products, and bitstreams, without deleting
any existing files. Firmware in `Vitis` and handoff files in `XSA` remain separate
from this Vivado migration.

## Validation boundary

The project owner confirmed successful bitstream generation from the v2 local
source bundle on Ubuntu with Vivado 2019.2 on **2026-11-09**. The local static
checks, isolated source-copy check, and portability regression tests also passed.
The owner-reported bitstream result is separate from the Windows checks: Vivado
2019.2 is not installed on this Windows host. No new timing-closure or on-board
RF / Linux runtime validation is asserted by this source import.
