# Changes from upstream

## Standalone integration — 2026-11-16

Based on Timo Teräs' `fabled/sendts-it9507` revision
`136e4b97f147b447c7e38b9d78aaa2f332870988`. The original Git history, copyright
notices, GPL license and original `firmware.h` are retained.

The integration was developed in the headend project at
https://git.promet.ad/mitev/tsdecrypt, revision
`525d6cca6a23db65213ad6929ca664059de74a11`, in
`third_party/sendts-it9507`, then packaged here as an independent program.

### CLI and hardware control

- Explicit `--transmit` and frequency are required to enable RF.
- Added `--list`, non-transmitting `--probe` and stable `--serial` selection.
- Validate mode combinations and DVB-T parameters; fix bandwidth units,
  cell-ID parsing and the getopt option-table terminator.
- Validate USB response length, sequence and checksum; remove unaligned
  checksum reads and signed firmware-version shifts.
- Check initialization failures before streaming; set gain before enabling RF.
- Fix calibration fallback register, interpolation bounds and zero divisors.
- Capacity calculation retains precision by multiplying before division.

### Streaming

- Replace the leaking asynchronous transfer ring with one bounded 65424-byte
  synchronous USB buffer and hardware flow control.
- Coalesce complete 188-byte TS packets, preserve partial read fragments and
  flush partial batches within 10 ms. Do not add NULL packets or filter PIDs.
- Handle TS misalignment, short USB writes, transfer failure and input timeout.
- Attempt RF shutdown on EOF, SIGINT and SIGTERM before releasing USB.

### ITE initialization

- Link the initialization API from the UT-100 CD v2.4_20221221_withDEC Linux SDK
  into this executable, with userspace portability changes.
- `oem_bridge.c` supplies libusb transport and board initialization.
- Only board type 11 (tested UT-100C DB-01-01 v04) is currently accepted for
  transmission. Other boards fail explicitly rather than assume compatibility.
- Active RAM firmware: LINK `ff270300`, OFDM `ff090c00`.
- See `oem/README.md` and `oem/SOURCE.json` for source paths and SDK hashes.

### Standalone packaging

- Document installation, USB permissions, file input and a VBR mux pipeline.
- Include hardware-independent CLI and mocked USB protocol/streaming tests.
- Include a narrowly scoped udev rule for VID:PID 048d:9507.
- Remove the old `mux.sh` example (it used the obsolete `-c`/implicit transmit
  CLI) and the unrelated, unbuilt `mpegts-switch.c` utility from the working
  tree. Both remain in the original Git history.
