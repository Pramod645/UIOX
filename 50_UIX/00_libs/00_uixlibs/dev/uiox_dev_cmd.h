#ifndef UIOX_DEV_CMD_H
#define UIOX_DEV_CMD_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"     /* uix_ssize_t, uix_off_t */
#include "../PoStd/uix_poll.h"    /* uix_pollfd_t, UIX_POLLIN */

/* ── shape 5: CMD ──────────────────────────────────────────────────
 * Family 5 (gpu, hdmi, mon): a command device with a shared surface.
 * You map a buffer, hand it to the device, and wait for the device to
 * finish with it.
 *
 * NO DEVICE COMMANDS APPEAR HERE.  There is no generic "start" or
 * "stop": the monitor's enable/disable carry its own numbers
 * (UIOX_MON_IOC_ENABLE / _DISABLE), and a second family member will
 * carry its own.  The shape supplies the surface, the submission and
 * the wait; the device supplies the vocabulary. */

uiox_dev_err_t uiox_dev_cmd_open (uiox_dev_handle_t *h, const char *name, int flags);
uiox_dev_err_t uiox_dev_cmd_close(uiox_dev_handle_t *h);

/* Map the device's surface.  SYS_MMAP 49 dispatches to
 * uiox_kix_scfs_mmap (declared at uiox_kix_scfs.h:473); flags and prot
 * are the mirrored UIOX_DEV_MAP_* / UIOX_DEV_PROT_* values, passed
 * through uninterpreted. */
void          *uiox_dev_cmd_map(uiox_dev_handle_t *h, unsigned long length,
                                int prot, unsigned long long offset);

/* Submit work: `request` is the device's own command number (the
 * monitor's FLIP, a GPU's submit), `arg` its own argument struct. */
int            uiox_dev_cmd_submit(uiox_dev_handle_t *h,
                                   unsigned long request, void *arg);

/* Wait for the completion event — vblank, fence, done — via uix_poll. */
uiox_dev_err_t uiox_dev_cmd_wait(uiox_dev_handle_t *h, int timeout_ms);

/* Escape hatch for any other device command. */
int            uiox_dev_cmd_ioctl(uiox_dev_handle_t *h,
                                  unsigned long request, void *arg);

#endif
