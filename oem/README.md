# ITE initialization API

This subset comes from the operator-supplied **UT-100 CD v2.4_20221221_withDEC**,
`SDK_Linux/it950x_linux_v18.04.16.2/it950x_driver`. The driver identifies itself
as v18.04.16.1, TX API 1.3.20160929.0. Copyright ITE Corporation; the containing
Linux driver declares `MODULE_LICENSE("GPL")`. Original source notices are
retained. The distribution includes no separate license notice for the firmware.
`SOURCE.json` records original file paths and SHA-256 digests.

Only the original API and its required headers are retained, not the Linux
kernel driver. Local portability changes replace Linux-only includes and
`msleep` with userspace headers and `usleep`; standard allocation declarations
are included. Line endings and whitespace are normalized. `URB_BUFSIZE_RX` is a build constant
for the API's existing interface setup. The original API types are retained.

`../oem_bridge.c` supplies synchronous libusb control transfers to an already
claimed handle and calls the same initialization sequence used in the successful
hardware trial. It reads board type, configures board GPIO, determines the TS
interface, initializes firmware/API, then queries LINK/OFDM versions. It does
not write EEPROM or enable RF. Validated board type is 11, UT-100C DB-01-01 v04.
The embedded RAM firmware is LINK ff270300 / OFDM ff090c00.

The API is linked into **sendts-it9507**. There is no extra initialization daemon,
helper executable or kernel module. `--list` and `--probe` do not call it; only
explicit `--transmit` initializes hardware.
