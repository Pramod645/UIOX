/*
 * 50_UIX/00_libs/00_uixlibs/dev/uiox_dev_cmd.c
 *
 * Shape 5 — CMD.  Generic: no device's control numbers appear here.
 * Reaches the kernel only through 40_SCIX (sys_open, sys_mmap,
 * sys_close) and the sys/ wrapper uix_ioctl.  No 30_KIX header.
 */
#include "uiox_dev_cmd.h"
#include "uix_ioctl.h"
#include "uix_sys.h"            /* sys_open, sys_mmap, sys_close */

uiox_dev_err_t uiox_dev_cmd_open(uiox_dev_handle_t *h, const char *name, int flags)
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

void *uiox_dev_cmd_map(uiox_dev_handle_t *h, unsigned long length,
                       int prot, unsigned long long offset)
{
    if (!h || h->fd < 0) return 0;
    return sys_mmap(0, length, prot, UIOX_DEV_MAP_SHARED,
                    h->fd, (uix_off_t)offset);
}

int uiox_dev_cmd_submit(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}

int uiox_dev_cmd_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}

uiox_dev_err_t uiox_dev_cmd_wait(uiox_dev_handle_t *h, int timeout_ms)
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

uiox_dev_err_t uiox_dev_cmd_close(uiox_dev_handle_t *h)
{
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    sys_close(h->fd); h->fd = -1;
    return UIOX_DEV_OK;
}
