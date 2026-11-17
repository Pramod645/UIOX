/*
 * 50_UIX/00_libs/00_uixlibs/dev/uiox_dev_block.c
 *
 * Shape 2 — BLOCK.  Generic: no device's control numbers appear here.
 * Reaches the kernel only through 40_SCIX (sys_*) and the sys/ wrapper
 * uix_ioctl.  No 30_KIX header is included.
 */
#include "uiox_dev_block.h"
#include "uix_ioctl.h"          /* uix_ioctl — forwards to sys_ioctl first */
#include "uix_sys.h"            /* sys_open/read/write/lseek/close          */
#include "../PoStd/uix_unistd.h"/* UIX_SEEK_SET (0)                         */

/* ── open / close ─────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_block_open(uiox_dev_handle_t *h, const char *name, int flags)
{
    char path[UIOX_DEV_PATH_MAX];
    int  fd;

    if (!h || !name) return UIOX_DEV_EBADNAME;
    if (uiox_dev_make_path(name, 0, path, sizeof path) != UIOX_DEV_OK)
        return UIOX_DEV_EBADNAME;

    fd = sys_open(path, flags);
    if (fd < 0) return UIOX_DEV_ENODEV;

    h->fd   = fd;
    h->unit = 0;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_block_close(uiox_dev_handle_t *h)
{
    if (!h || h->fd < 0) return UIOX_DEV_ENODEV;
    sys_close(h->fd);
    h->fd = -1;
    return UIOX_DEV_OK;
}

/* ── sequential transfer ─────────────────────────────────────────── */

uix_ssize_t uiox_dev_block_read(uiox_dev_handle_t *h, void *buf, uix_size_t n)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    return sys_read(h->fd, buf, n);
}

uix_ssize_t uiox_dev_block_write(uiox_dev_handle_t *h, const void *buf, uix_size_t n)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    return sys_write(h->fd, buf, n);
}

/* ── positioned transfer ─────────────────────────────────────────────
 * SYS_PREAD 169 / SYS_PWRITE 170 have numbers but sit at null slots in
 * uiox_kix_scfs_dispatch.c, so a call returns ENOSYS today.  Seek+read
 * is the form that works now — plain sys_read() with no lseek, as in
 * the sequential case, since a read is a read whatever the position.
 * Swap the body for sys_pread() when the dispatch slot is filled.
 * Not atomic for concurrent callers, which is fine while block access
 * is single-threaded. */

uix_ssize_t uiox_dev_block_pread(uiox_dev_handle_t *h, void *buf, uix_size_t n,
                                 unsigned long long offset)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    if (sys_lseek(h->fd, (uix_off_t)offset, UIX_SEEK_SET) < 0)
        return (uix_ssize_t)-1;
    return sys_read(h->fd, buf, n);
}

uix_ssize_t uiox_dev_block_pwrite(uiox_dev_handle_t *h, const void *buf, uix_size_t n,
                                  unsigned long long offset)
{
    if (!h || h->fd < 0) return (uix_ssize_t)-1;
    if (sys_lseek(h->fd, (uix_off_t)offset, UIX_SEEK_SET) < 0)
        return (uix_ssize_t)-1;
    return sys_write(h->fd, buf, n);
}

/* ── escape hatch ─────────────────────────────────────────────────── */

int uiox_dev_block_ioctl(uiox_dev_handle_t *h, unsigned long request, void *arg)
{
    if (!h || h->fd < 0) return -1;
    return uix_ioctl(h->fd, request, arg);
}
