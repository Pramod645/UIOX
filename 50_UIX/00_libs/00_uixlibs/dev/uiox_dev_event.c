/* dev/uiox_dev_event.c */
#include "uiox_dev_event.h"
#include "uix_ioctl.h"
#include "uix_sys.h"

uiox_dev_err_t uiox_dev_event_open(uiox_dev_handle_t *h, const char *name, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;
    if (!h || !name) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(name, 0, path, sizeof path) != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;
    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;
    h->fd = fd; h->unit = 0;
    return UIOX_DEV_OK;
}

/* An event device is read-blocking, so the wait and the take are the
 * same syscall on most drivers.  uix_poll first because it can be given
 * a timeout and read() cannot. */
uiox_dev_err_t uiox_dev_event_wait(uiox_dev_handle_t *h, int timeout_ms)
{
    uix_pollfd_t pf;
    int r;
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    pf.fd = h->fd; pf.events = UIX_POLLIN; pf.revents = 0;
    r = uix_poll(&pf, 1u, timeout_ms);
    if (r <= 0)                    return UIOX_DEV_EIO;
    if (pf.revents & UIX_POLLNVAL) return UIOX_DEV_ENODEV;
    return UIOX_DEV_OK;
}

uix_ssize_t uiox_dev_event_read(uiox_dev_handle_t *h, void *rec, uix_size_t n)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    return sys_read(h->fd, rec, n);
}

int uiox_dev_event_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}

uiox_dev_err_t uiox_dev_event_close(uiox_dev_handle_t *h)
{
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    sys_close(h->fd); h->fd = -1;
    return UIOX_DEV_OK;
}
