/*
 * 50_UIX/00_libs/00_uixlibs/dev/mouse/uiox_dev_mouse.c
 *
 * MOUSE on the EVENT shape.  Reaches the kernel only through sys_open,
 * sys_read, sys_close and uix_poll — no 30_KIX header.
 */
#include "uiox_dev_mouse.h"
#include "uix_sys.h"               /* sys_open, sys_read, sys_close */

uiox_dev_err_t uiox_dev_mouse_open(uiox_dev_mouse_t *m, int unit, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;

    if (!m) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(UIOX_DEV_MOUSE_DEVPATH, unit, path, sizeof path)
        != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;

    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;

    m->h.fd     = fd;
    m->h.unit   = unit;
    m->last_x   = 0;
    m->last_y   = 0;
    m->have_pos = 0;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_mouse_close(uiox_dev_mouse_t *m)
{
    if (!m || m->h.fd < 0) return UIOX_DEV_ENODEV;
    sys_close(m->h.fd);
    m->h.fd = -1;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_mouse_wait(uiox_dev_mouse_t *m, int timeout_ms)
{
    uix_pollfd_t pf;
    int r;

    if (!m || m->h.fd < 0) return UIOX_DEV_ENODEV;

    pf.fd      = m->h.fd;
    pf.events  = UIX_POLLIN;
    pf.revents = 0;

    r = uix_poll(&pf, 1u, timeout_ms);
    if (r == 0)                    return UIOX_DEV_EIO;   /* timed out */
    if (r < 0)                     return UIOX_DEV_EIO;
    if (pf.revents & UIX_POLLNVAL) return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_mouse_read(uiox_dev_mouse_t *m,
                                   uiox_dev_mouse_event_t *out)
{
    uix_ssize_t n;

    if (!m || !out || m->h.fd < 0) return UIOX_DEV_EBADNAME;

    /* The driver's event_read returns bytes written or a negative
     * errno — never 0 (uiox_mouse_if.h:67-68).  So anything short of a
     * full record is a fault, and it is reported here as one; an app
     * that treated 0 as "queue empty" would spin on a broken fd. */
    n = sys_read(m->h.fd, out, (uix_size_t)sizeof *out);
    if (n <= 0)                     return UIOX_DEV_EIO;
    if (n < (uix_ssize_t)sizeof *out) return UIOX_DEV_EIO;

    /* Remember the cursor from the event that just arrived. */
    m->last_x   = out->x;
    m->last_y   = out->y;
    m->have_pos = 1;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_mouse_cursor(const uiox_dev_mouse_t *m,
                                     int *x, int *y)
{
    if (!m || !x || !y) return UIOX_DEV_EBADNAME;
    if (!m->have_pos)   return UIOX_DEV_EIO;   /* no event yet */
    *x = m->last_x;
    *y = m->last_y;
    return UIOX_DEV_OK;
}
