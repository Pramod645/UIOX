/* dev/uiox_dev_event.h */
#ifndef UIOX_DEV_EVENT_H
#define UIOX_DEV_EVENT_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"
#include "../PoStd/uix_poll.h"

/* ── shape 3: EVENT ────────────────────────────────────────────────
 * Family 3 (kbd, mouse, tp): a device that pushes fixed-size records
 * at you and must be waited on.  No seek, no position, no geometry —
 * the operation is "block until something happened, then take one". */

uiox_dev_err_t uiox_dev_event_open (uiox_dev_handle_t *h, const char *name, int flags);
uiox_dev_err_t uiox_dev_event_close(uiox_dev_handle_t *h);

/* Blocking or timed wait; see 5.4 of the IPC design on why this is
 * read-and-compare, never sleep-until-deadline. */
uiox_dev_err_t uiox_dev_event_wait(uiox_dev_handle_t *h, int timeout_ms);

/* Takes one record.  `rec` is opaque: the device file includes the
 * kernel header for the real event type and passes its address. */
uix_ssize_t    uiox_dev_event_read(uiox_dev_handle_t *h, void *rec, uix_size_t n);

/* Escape hatch for a device's own commands. */
int uiox_dev_event_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg);

#endif
