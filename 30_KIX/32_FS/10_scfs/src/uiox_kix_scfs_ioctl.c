/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_ioctl.c
 *
 * ioctl
 *
 * Bach Ch.7 — device- and filesystem-specific control is carried by the
 * file's own operation switch.  SCFS never interprets the request; it
 * resolves the fd and forwards it.  A file with no ioctl op reports ENOSYS.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* ioctl() — forward a control request to the file's own operation. */
long uiox_kix_scfs_ioctl(uiox_reg_t fd, uiox_reg_t req, uiox_reg_t arg,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->ioctl) return -SCFS_ENOSYS;
    return (long)f->f_op->ioctl(f, (uint32_t)req, (void *)arg);
}
