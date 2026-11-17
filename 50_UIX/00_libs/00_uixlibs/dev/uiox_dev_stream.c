/*
 * 50_UIX/00_libs/00_uixlibs/dev/uiox_dev_stream.c
 *
 * Shape 1 — STREAM.  Generic: no device's control numbers appear here.
 * Reaches the kernel only through 40_SCIX (sys_*) and the sys/ wrapper
 * uix_poll.  No 30_KIX header is included.
 */
#include "uiox_dev_stream.h"
#include "uix_sys.h"            /* sys_open/read/write/close */
#include "../PoStd/uix_poll.h"  /* uix_poll, uix_pollfd_t, UIX_POLL* */

/* ── open ─────────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_stream_open(uiox_dev_handle_t *h,
                                    const char *prefix, int unit, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;

    if (!h || !prefix) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(prefix, unit, path, sizeof path) != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;

    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;

    h->fd   = fd;
    h->unit = unit;
    return UIOX_DEV_OK;
}

/* "eth1" and ("eth", 1) must reach the same node, so a trailing decimal
 * is split off the name and passed as the unit.  devparse.c accepts both
 * forms in the kernel; this keeps the two sides agreeing. */
uiox_dev_err_t uiox_dev_stream_open_name(uiox_dev_handle_t *h,
                                         const char *name, int flags)
{
    char buf[UIOX_DEV_PATH_MAX];
    unsigned long i = 0;
    int unit = 0, digits = 0, mul = 1, k;

    if (!h || !name) return UIOX_DEV_EBADNAME;
    while (name[i] && i + 1u < sizeof buf) { buf[i] = name[i]; i++; }
    buf[i] = '\0';

    while (i > 0u && buf[i - 1u] >= '0' && buf[i - 1u] <= '9') {
        unit += (buf[i - 1u] - '0') * mul;
        for (k = 0; k < digits; k++) mul *= 10;
        digits++;
        i--;
    }
    buf[i] = '\0';

    return uiox_dev_stream_open(h, buf, digits ? unit : 0, flags);
}

/* ── transfer ─────────────────────────────────────────────────────── */

uix_ssize_t uiox_dev_stream_read(uiox_dev_handle_t *h, void *buf, uix_size_t n)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    return sys_read(h->fd, buf, n);
}

uix_ssize_t uiox_dev_stream_write(uiox_dev_handle_t *h, const void *buf, uix_size_t n)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    return sys_write(h->fd, buf, n);
}

/* ── poll ─────────────────────────────────────────────────────────────
 * One element, so the caller's `events` mask selects what to wait on.
 * UIX_POLLNVAL says the fd is not one the kernel serves — the same
 * failure open() would have reported, reported at wait time instead. */

uiox_dev_err_t uiox_dev_stream_poll(uiox_dev_handle_t *h, short events, int timeout_ms)
{
    uix_pollfd_t pf;
    int r;

    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;

    pf.fd      = h->fd;
    pf.events  = events ? events : UIX_POLLIN;
    pf.revents = 0;

    r = uix_poll(&pf, 1u, timeout_ms);
    if (r == 0)                      return UIOX_DEV_EIO;     /* timed out */
    if (r < 0)                       return UIOX_DEV_EIO;
    if (pf.revents & UIX_POLLNVAL)   return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}

/* ── close ────────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_stream_close(uiox_dev_handle_t *h)
{
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    sys_close(h->fd);
    h->fd = -1;
    return UIOX_DEV_OK;
}
