/*
 * 50_UIX/00_libs/00_uixlibs/dev/kbd/uiox_dev_kbd.c
 *
 * KBD on the EVENT shape.  Reaches the kernel only through sys_open,
 * sys_read, sys_close and uix_poll — no 30_KIX header.
 *
 * The mirrored event struct is passed straight through as the read
 * buffer: the kernel's uiox_kbd_event_read() pops one record into
 * whatever address it is given, and this layout is that record.
 */
#include "uiox_dev_kbd.h"
#include "uix_sys.h"               /* sys_open, sys_read, sys_close */

uiox_dev_err_t uiox_dev_kbd_open(uiox_dev_kbd_t *k, int unit, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;

    if (!k) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(UIOX_DEV_KBD_DEVPATH, unit, path, sizeof path)
        != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;

    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;

    k->h.fd   = fd;
    k->h.unit = unit;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_kbd_close(uiox_dev_kbd_t *k)
{
    if (!k || k->h.fd < 0) return UIOX_DEV_ENODEV;
    sys_close(k->h.fd);
    k->h.fd = -1;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_kbd_wait(uiox_dev_kbd_t *k, int timeout_ms)
{
    uix_pollfd_t pf;
    int r;

    if (!k || k->h.fd < 0) return UIOX_DEV_ENODEV;

    pf.fd      = k->h.fd;
    pf.events  = UIX_POLLIN;
    pf.revents = 0;

    r = uix_poll(&pf, 1u, timeout_ms);
    if (r == 0)                    return UIOX_DEV_EIO;   /* timed out */
    if (r < 0)                     return UIOX_DEV_EIO;
    if (pf.revents & UIX_POLLNVAL) return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_kbd_read(uiox_dev_kbd_t *k, uiox_dev_kbd_event_t *out)
{
    uix_ssize_t n;

    if (!k || !out || k->h.fd < 0) return UIOX_DEV_EBADNAME;

    /* One record per read.  A short read means the kernel had nothing
     * to pop, which is the caller's cue to wait() rather than to retry
     * blindly — the same read-and-compare discipline the netfs mount
     * uses, since nothing here can sleep on a deadline. */
    n = sys_read(k->h.fd, out, (uix_size_t)sizeof *out);
    if (n < (uix_ssize_t)sizeof *out) return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}
