/* dev/uiox_dev_bus.c */
#include "uiox_dev_bus.h"
#include "uix_ioctl.h"
#include "uix_sys.h"

uiox_dev_err_t uiox_dev_bus_open(uiox_dev_handle_t *h, const char *name, int flags)
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

int uiox_dev_bus_enumerate(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}

int uiox_dev_bus_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}

uiox_dev_err_t uiox_dev_bus_close(uiox_dev_handle_t *h)
{
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    sys_close(h->fd); h->fd = -1;
    return UIOX_DEV_OK;
}
