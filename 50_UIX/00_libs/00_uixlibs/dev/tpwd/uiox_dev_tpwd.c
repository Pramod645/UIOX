/*
 * 50_UIX/00_libs/00_uixlibs/dev/tpwd/uiox_dev_tpwd.c
 *
 * TPWD on the EVENT shape.  Reaches the kernel only through sys_open,
 * sys_read, sys_close and uix_poll — no 30_KIX header.
 */
#include "uiox_dev_tpwd.h"
#include "uix_sys.h"               /* sys_open, sys_read, sys_close */

uiox_dev_err_t uiox_dev_tpwd_open(uiox_dev_tpwd_t *t, int unit, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;

    if (!t) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(UIOX_DEV_TPWD_DEVPATH, unit, path, sizeof path)
        != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;

    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;

    t->h.fd       = fd;
    t->h.unit     = unit;
    t->auth_state = UIOX_DEV_TPWD_STATE_UNKNOWN;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_tpwd_close(uiox_dev_tpwd_t *t)
{
    if (!t || t->h.fd < 0) return UIOX_DEV_ENODEV;
    sys_close(t->h.fd);
    t->h.fd = -1;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_tpwd_wait(uiox_dev_tpwd_t *t, int timeout_ms)
{
    uix_pollfd_t pf;
    int r;

    if (!t || t->h.fd < 0) return UIOX_DEV_ENODEV;

    pf.fd      = t->h.fd;
    pf.events  = UIX_POLLIN;
    pf.revents = 0;

    r = uix_poll(&pf, 1u, timeout_ms);
    if (r == 0)                    return UIOX_DEV_EIO;   /* timed out */
    if (r < 0)                     return UIOX_DEV_EIO;
    if (pf.revents & UIX_POLLNVAL) return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_tpwd_read(uiox_dev_tpwd_t *t,
                                  uiox_dev_tpwd_raw_evt_t *out)
{
    uix_ssize_t n;

    if (!t || !out || t->h.fd < 0) return UIOX_DEV_EBADNAME;

    /* uiox_tpwd_if.h:151 — event_read returns bytes written or a
     * negative errno, NEVER 0.  Anything short of a full record is a
     * fault, and is reported as one here. */
    n = sys_read(t->h.fd, out, (uix_size_t)sizeof *out);
    if (n <= 0)                       return UIOX_DEV_EIO;
    if (n < (uix_ssize_t)sizeof *out) return UIOX_DEV_EIO;
    return UIOX_DEV_OK;
}

uiox_dev_tpwd_state_t uiox_dev_tpwd_state(const uiox_dev_tpwd_t *t)
{
    if (!t) return UIOX_DEV_TPWD_STATE_UNKNOWN;
    return t->auth_state;
}
