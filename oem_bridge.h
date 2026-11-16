/* Original ITE API initialization on an already claimed libusb device. */
#ifndef IT9507_OEM_BRIDGE_H
#define IT9507_OEM_BRIDGE_H
#include <libusb.h>
int it9507_oem_initialize(libusb_device_handle *handle);
#endif
