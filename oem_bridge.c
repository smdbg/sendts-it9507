/* Userspace glue for the operator-supplied ITE GPL driver API.
 * The caller owns USB interface 0. Initialization never enables RF. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include "oem_bridge.h"
#include "oem/modulatorType.h"
/* Keep firmware arrays private to the vendor API translation unit. */
extern Dword EagleUser_getDeviceType(Modulator *, Byte *);
extern Dword EagleUser_getSystemConfig(Modulator *, Byte, SystemConfig *);
extern Dword EagleUser_setSystemConfig(Modulator *, SystemConfig);
extern Dword EagleUser_getTsInputType(Modulator *, TsInterface *);
extern Dword IT9507_initialize(Modulator *, TsInterface, Byte, Byte);
extern Dword IT9507_getFirmwareVersion(Modulator *, Processor, Dword *);

static Dword transfer(Modulator *m, unsigned char ep, Dword n, Byte *bytes)
{
    if (!n || n > INT_MAX) return 1;
    int actual = 0;
    int result = libusb_bulk_transfer(m->userData, ep, bytes, (int)n, &actual, 1000);
    if (result || actual != (int)n) {
        fprintf(stderr, "OEM USB endpoint %02x: %s, %d/%lu bytes\n",
                ep, libusb_error_name(result), actual, n);
        return 1;
    }
    return 0;
}
Dword Usb2_writeControlBus(Modulator *m, Dword n, Byte *b) { return transfer(m, 0x02, n, b); }
Dword Usb2_readControlBus(Modulator *m, Dword n, Byte *b) { return transfer(m, 0x81, n, b); }

int it9507_oem_initialize(libusb_device_handle *handle)
{
    Modulator m = {0};
    SystemConfig cfg = {0};
    TsInterface stream = INTERFACE_UNUSE;
    Byte type = 0;
    Dword r, link = 0, ofdm = 0;
    m.userData = handle;
    m.busId = Bus_USB;
    if ((r = EagleUser_getDeviceType(&m, &type))) goto failed;
    /* The transmit configuration below has been verified on this UT-100C board. */
    if (type != 11) {
        fprintf(stderr, "Unsupported IT9507 board type %u (validated: 11)\n", type);
        return -ENOTSUP;
    }
    if ((r = EagleUser_getSystemConfig(&m, type, &cfg))) goto failed;
    if ((r = EagleUser_setSystemConfig(&m, cfg))) goto failed;
    if ((r = EagleUser_getTsInputType(&m, &stream))) goto failed;
    m.tsInterfaceType = stream;
    if ((r = IT9507_initialize(&m, (TsInterface)StreamType_DVBT_DATAGRAM, Bus_USB, 0))) goto failed;
    if ((r = IT9507_getFirmwareVersion(&m, Processor_LINK, &link))) goto failed;
    if ((r = IT9507_getFirmwareVersion(&m, Processor_OFDM, &ofdm))) goto failed;
    fprintf(stderr, "OEM initialized board %u, LINK %08lx, OFDM %08lx; RF off\n", type, link, ofdm);
    return 0;
failed:
    fprintf(stderr, "OEM initialization failed: 0x%lx\n", r);
    return -EIO;
}
