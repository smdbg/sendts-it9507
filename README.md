# sendts-it9507

Standalone Linux userspace transmitter for the ITE IT9507 / UT-100C USB DVB-T
modulator. It reads raw 188-byte MPEG-TS packets from stdin, programs the RF
parameters and sends the stream through **libusb**.

This is a modified fork of [fabled/sendts-it9507](https://github.com/fabled/sendts-it9507)
by **Timo Teräs**, based on commit `136e4b9`. It retains the original Git history
and author notices. The integrated ITE initialization API comes from the
UT-100 CD Linux SDK. See [CHANGELOG.md](CHANGELOG.md) and [oem/README.md](oem/README.md).

No custom kernel driver, FFmpeg, TSDuck, Python runtime or container is needed.
Python 3 is used only by the optional tests. Hardware control and initialization
are linked into this single executable; there is no helper daemon.

## Supported hardware

- USB VID:PID **048d:9507**.
- Transmission validated on **UT-100C DB-01-01 v04, board type 11**.
- Other board types are explicitly rejected by the initialization bridge.
- A matching USB ID or successful firmware probe alone does not establish
  compatibility with other IT9507 products.
- DVB-T: 6/7/8 MHz bandwidth, 2K/8K, QPSK/16-QAM/64-QAM, FEC 1/2, 2/3, 3/4,
  5/6 or 7/8, guard 1/4, 1/8, 1/16 or 1/32.

The validated initialization uses LINK firmware `ff270300` and OFDM firmware
`ff090c00`. Upstream originally embedded `ff270200` / `ff090800`. The original
`firmware.h` is retained unchanged; active initialization uses `oem/`.

## Build and install

On Debian / Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y git build-essential pkg-config libusb-1.0-0-dev python3
git clone https://github.com/smdbg/sendts-it9507.git
cd sendts-it9507
make -j"$(nproc)"
make test
sudo make install
```

`make test` uses a mocked USB device and CLI checks. It never enables RF and
does not need a connected modulator. Building requires a C compiler, make,
pkg-config and libusb development headers; the installed binary needs libusb.

The default installation prefix is `/usr/local`. To install for one user:

```bash
make install PREFIX="$HOME/.local"
```

`DESTDIR` is also supported for packaging. To run without installing, use
`./sendts-it9507` from the build directory.

## USB permissions

For access as a normal user, install the supplied rule and join the `video`
group:

```bash
sudo install -m 0644 contrib/99-it9507.rules /etc/udev/rules.d/99-it9507.rules
sudo usermod -aG video "$USER"
sudo udevadm control --reload-rules
```

Log out and in again, then unplug and reconnect the modulator. The rule gives
the `video` group read/write access only to devices with VID:PID `048d:9507`.
It is not installed automatically by `make install`.

Use only one transmitter process per device. If a kernel driver or another
application owns the interface, stop that owner first. This program reports
the conflict instead of detaching an active kernel driver.

## Discover and inspect

```bash
sendts-it9507 --list
sendts-it9507 --probe --serial YOUR_USB_SERIAL
```

Use the **USB dongle serial number** shown by `sendts-it9507 --list` as the
value of `--serial`.

`--list` reads USB descriptors without initializing the transmitter.
`--probe` claims interface 0 and queries LINK/OFDM firmware versions only. It
does not upload firmware, reset the USB device, change modulation registers
or alter RF state. Prefer `--serial` over `--device N`, since enumeration
indices can change after unplugging a device or adding a second one.

## Calculate DVB-T capacity

```bash
sendts-it9507 --muxrate -b 8000 -t 8k -C qam64 -r 3/4 -G 1/32
# 27144384 (bits per second; approximately 27.144 Mbps)
```

This calculation does not access USB. The input's useful bitrate, including
tables and bitrate peaks, must fit the selected DVB-T payload capacity.

## Transmit a prepared transport-stream file

Replace `YOUR_USB_SERIAL` and use a prepared multiplex whose signaling matches
the intended service/network settings:

```bash
sendts-it9507 --transmit --serial YOUR_USB_SERIAL \
  -f 474000 -b 8000 -t 8k -C qam64 -r 3/4 -G 1/32 -g -10 < prepared.ts
```

`--transmit` and an explicit frequency are mandatory. Frequency and bandwidth
are in **kHz**: `474000` means 474 MHz, `8000` means 8 MHz. Gain is a relative
adjustment in dB, not absolute RF output power. This build accepts -30 through
0 dB; 0 dB is the highest supported software setting, not RF off.

Input must consist of synchronized 188-byte TS packets. File input is clocked
by USB flow control; there is no PCR-based software pacing or restamping. The
tool does not generate DVB tables, remultiplex services or adapt an oversized
CBR input to a lower RF capacity.

## Transmit live UDP channels through DVB-T Mux

The transmitter reads stdin; it does not open UDP sockets. Use the separate
[dvbt-mux](https://github.com/smdbg/dvbt-mux) program to receive UDP/RTP services
and create the DVB-T multiplex. Build it, install it and edit its supplied
`examples/two-channels.json` as `mux.json`, then run in Bash:

```bash
set -o pipefail
dvb-mux --config mux.json --check-config
dvb-mux --config mux.json --status-file /tmp/dvbt-mux-status.json |
  sendts-it9507 --transmit --serial YOUR_USB_SERIAL \
    -f 474000 -b 8000 -t 8k -C qam64 -r 3/4 -G 1/32 -g -10
```

Keep the mux JSON's frequency, bandwidth, modulation, FEC, guard and FFT mode
consistent with the transmitter command. `dvb-mux` generates the matching DVB
tables; `sendts-it9507` programs the RF hardware. Neither program uses FFmpeg.

The tested UT-100C accepts VBR without software NULL stuffing. The transmitter
retains every supplied packet, including EIT, ECM, EMM and any NULL packets.
`dvb-mux` removes incoming NULLs before transmission. Do not feed a padded
38/44 Mbps multiplex into a 27.144 Mbps configuration directly.

Writes are batched up to 65424 bytes; partial batches of complete TS packets
are flushed within 10 ms. Partial read fragments are carried into the next
batch. There are no per-packet sleeps or software codec operations.

## Command reference

| Option | Meaning |
| --- | --- |
| `--list` | List matching devices without initialization |
| `--probe` | Query firmware versions without changing RF state |
| `--transmit` | Initialize, configure RF and send stdin |
| `--muxrate` / `-m` | Print TS capacity in bits/s without USB access |
| `--serial SERIAL` | USB dongle serial number; shown by `--list` |
| `--device N` / `-d N` | Zero-based enumeration index; default 0 |
| `--frequency KHZ` / `-f KHZ` | Required for transmission; 174000..862000 |
| `--bandwidth KHZ` / `-b KHZ` | 6000, 7000 or 8000; default 8000 |
| `--transmission-mode MODE` / `-t MODE` | `2k` or `8k`; default `8k` |
| `--constellation MOD` / `-C MOD` | `qpsk`, `qam16`, `qam64`; default `qam64` |
| `--code-rate FEC` / `-r FEC` | 1/2, 2/3, 3/4, 5/6 or 7/8; default 2/3 |
| `--guard-interval GI` / `-G GI` | 1/4, 1/8, 1/16 or 1/32; default 1/4 |
| `--gain DB` / `-g DB` | Relative -30..0 dB; default -10 |
| `--cell-id N` / `-i N` | 0..65535; default 0 |
| `--verbose` / `-v` | Additional diagnostics |
| `--help` / `-h` | Show usage |

Select exactly one operating mode (`--list`, `--probe`, `--transmit` or
`--muxrate`). Explicitly set FEC and guard in examples; their CLI defaults
are different from the 3/4 and 1/32 used in the validated setup.

## Stop and troubleshoot

EOF, SIGINT and SIGTERM attempt to turn RF off before releasing USB. Ctrl+C
stops a foreground pipeline. A supervisor should stop the transmitter first,
then its input producer. There is no daemon or automatic restart in this tool.
SIGKILL, unplugging USB or loss of the host cannot guarantee that an RF-off
command reaches the hardware.

- **Access denied:** check group membership and the udev rule, then reconnect.
- **Busy interface:** stop the other process using that modulator.
- **Unsupported board:** this integration currently accepts board type 11 only.
- **Five seconds without input:** the transmitter exits with an error. Check
  the upstream source; the tool does not invent replacement packets.
- **Short USB write, USB timeout or TS misalignment:** the process exits and
  logs the cause. Check USB connection, input framing and capacity.
- **No services on the receiver:** this tool does not create PAT/PMT/SDT/NIT;
  inspect the upstream multiplex and its RF metadata.

The CLI and mocked USB tests cover malformed responses, checksum/sequence
validation, firmware queries, partial TS blocks, short writes and cancellation.
They do not replace a real RF/receiver test. The integrated data path was
previously receiver-tested on the UT-100C with a live 11-service VBR multiplex.

## License and credits

`sendts-it9507.c` is **GPL-2.0-or-later**, originally by Timo Teräs, derived from
the ITE driver and Dave Chapman's cleaned driver. [LICENSE](LICENSE) retains
the upstream GPL v2 text. Original notices are preserved.

The ITE API subset and firmware provenance are recorded in
[oem/README.md](oem/README.md) and [oem/SOURCE.json](oem/SOURCE.json). The source
driver declares GPL; the supplied SDK has no separate firmware license notice.
These records describe the supplied material and do not relicense the firmware.
