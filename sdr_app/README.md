# sdr_app

C++17 embedded Linux application for the P201PRO Zynq-7020 / AD9361 design.
The files in `src/` and `include/` are the current application working sources,
including continuous audio streaming, timed WAVE capture and SINAD measurement.

## Requirements

- Ubuntu build environment with GNU Make and `arm-linux-gnueabihf-g++`.
- Linux ARM hard-float target with matching FPGA bitstream and device tree.
- AD9361 IIO devices and compatible libiio runtime.
- `/dev/mem` access, DMA buffers reserved in Linux, and the expected UIO mappings.
- Raw Layer 2 Ethernet access on the selected interface.

The supplied `third_party/libiio` contains the public header and ARM shared
library from the existing project. Its unversioned `libiio.so` linker name is
included so `make` works after a fresh checkout; `.so.0` and `.so.0.21` are also
retained. These are ARM binaries, not host x86 libraries. License texts were
extracted from the project's original libiio source archive.

## Build

```bash
cd sdr_app
make
```

Output: `bin/sdr_app`. To select another compatible compiler:

```bash
make CXX=arm-linux-gnueabihf-g++
```

The Makefile uses `-std=c++17 -pthread` and links against the bundled libiio.
This import preserves the application sources; it does not establish a new
cross-compile or runtime result. Use the existing Ubuntu / board environment
to validate this application revision with the rebuilt FPGA design.

## Deploy

`bin/deploy.sh` currently targets `root@192.168.2.10` and installs
`/usr/bin/sdr_app`. Edit its target before using it on another board.
It stops `sdr-app`, transfers the executable through SCP, sets its mode and
starts the service again:

```bash
bash bin/deploy.sh
```

The target must already provide the `sdr-app` service command and compatible
runtime libraries. `docs/sdr-app` contains the service helper used by the
project; its log is `/var/log/sdr_app.log`. Installing/configuring the Linux
image and service is part of the separate PetaLinux environment.

## Run on the board

```bash
sdr_app --help
sdr_app ad9361 status
sdr_app status
sdr_app
```

With no arguments, the application initializes the SDR and starts the Layer 2
command listener. Run a single service/listener instance when using the control
GUI. Commands that access FPGA registers, DMA or raw sockets normally run as
root on the embedded target.

Selected control commands:

```bash
sdr_app ad9361 rx frequency 100       # MHz
sdr_app ad9361 rx bandwidth 200000    # Hz
sdr_app ad9361 rx sample-rate 2083340 # samples/s
sdr_app mode-select 1                # current demodulation selection
sdr_app bandwidth 1                  # FPGA filter selection, not a Hz value
sdr_app mod-enable 1                 # enables processing and starts audio stream
sdr_app mod-enable 0                 # disables processing and stops audio stream
```

Use `--help` and the source for the complete command set. `bandwidth` controls
the FPGA filter selection, while `ad9361 rx bandwidth` changes the RF device's
bandwidth in Hz.

## Audio recording and SINAD

```bash
sdr_app audio capture 5 /tmp/test.wave
sdr_app audio sinad /tmp/test.wave
```

The capture duration is in seconds. If no name is supplied, the capture command
uses `/tmp/audio_capture.wave`; a `.wav` suffix is changed to `.wave`.
The SINAD implementation searches for a fundamental from 900 to 1100 Hz, then
refines it near the strongest peak. Use a test tone near **1 kHz**; this command
does not perform a general search across all possible input frequencies.

## Hardware and network settings

| Setting | Current value | Source |
| --- | --- | --- |
| FPGA register base | `0x43C00000` | `include/RegisterMap.h` |
| PSD DMA base | `0x40400000` | `include/dma.h` |
| Audio DMA base | `0x40410000` | `include/dma.h` |
| PSD buffer used by capture commands | `0x3F000000` | `src/main.cpp` |
| Audio buffer | `0x3F002000` | `src/audio_handler.cpp` |
| Audio UIO device | `/dev/uio1` | `src/audio_handler.cpp` |
| Network interface | `eth0` | `include/network_config.h` |

These addresses and the UIO numbering must match the booted device tree and
hardware handoff. Select exactly one of `SDR_NET_ETH0` or `SDR_NET_USB0` before
building. The actual PetaLinux configuration will be imported separately.

## Layer 2 protocol

| Transmit stream | EtherType | Custom ID |
| --- | --- | --- |
| PSD | `0x9410` | `0x0200` |
| IQ | `0x9411` | `0x0208` |
| Audio | `0x8400` | `0x0213` |

Transmit frames contain the 14-byte Ethernet header, a 2-byte custom ID,
payload and an 8-byte footer. The payload limit is 8192 bytes; the network
path must support the frame sizes in use. The receive command format differs:
the 27-byte command frame has length `0x000D`, FPGA ID `0x0282`, and an 11-byte
command. Details are in `include/l2_protocol.h` and `include/CommandDispatcher.h`.

`docs/Radio_OrangePi_Log` contains earlier packet captures/screenshots.
`docs/wireshark_filter.txt` contains the existing frame-length filter. These
are reference artifacts from development, not new validation results.
