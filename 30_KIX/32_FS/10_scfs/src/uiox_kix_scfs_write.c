/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_write.c
 *
 * write
 *
 * Bach Ch.7 — sequential write through the file's own write operation,
 * advancing f_pos as bytes are written.  The buffered write path (page
 * cache, delayed write, bmap_alloc on growth) lives below in the backend;
 * SCFS resolves the fd and hands the call down.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* write() — sequential write from the user buffer, advancing f_pos. */
long uiox_kix_scfs_write(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->write) return -SCFS_ENOSYS;
    return (long)f->f_op->write(f, (const void *)ubuf, (size_t)count, &f->f_pos);
}
