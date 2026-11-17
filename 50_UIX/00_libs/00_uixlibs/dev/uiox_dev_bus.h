/* dev/uiox_dev_bus.h */
#ifndef UIOX_DEV_BUS_H
#define UIOX_DEV_BUS_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"

/* ── shape 6: BUS ──────────────────────────────────────────────────
 * Family 6 (tb4, usbhc): a bus controller, not a data device.  It
 * moves no payload of its own — you enumerate what is attached to it
 * and manage it.  The whole shape is open, ioctl, close. */

uiox_dev_err_t uiox_dev_bus_open (uiox_dev_handle_t *h, const char *name, int flags);
uiox_dev_err_t uiox_dev_bus_close(uiox_dev_handle_t *h);

/* `arg` is opaque: the device file passes the real enumeration struct. */
int uiox_dev_bus_enumerate(uiox_dev_handle_t *h, unsigned long request, void *arg);
int uiox_dev_bus_ioctl    (uiox_dev_handle_t *h, unsigned long request, void *arg);

#endif
