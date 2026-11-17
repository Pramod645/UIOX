/* dev/uiox_dev_reg.h */
#ifndef UIOX_DEV_REG_H
#define UIOX_DEV_REG_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"

/* ── shape 4: REG ──────────────────────────────────────────────────
 * Family 4 (als, therm, bms, chg, pmic, rtc, fan): a register device.
 * There is no stream and no position — you ask for one named value.
 * So the whole shape is open, ioctl, close; every operation is a
 * command, and the commands belong to the device file. */

uiox_dev_err_t uiox_dev_reg_open (uiox_dev_handle_t *h, const char *name, int flags);
uiox_dev_err_t uiox_dev_reg_close(uiox_dev_handle_t *h);

/* The shape's only real operation.  `arg` is opaque: the device file
 * includes the kernel header for the real type. */
int uiox_dev_reg_get(uiox_dev_handle_t *h, unsigned long request, void *arg);
int uiox_dev_reg_set(uiox_dev_handle_t *h, unsigned long request, void *arg);

#endif
