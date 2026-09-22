/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_lseek.c
 *
 * lseek
 *
 * Bach Ch.7 — reposition the per-file offset.  Defect fixed here: the ops
 * slot was `.seek = (void *)0`, so any lseek dereferenced a null pointer.
 * Three tiers now: the backend's own seek op, else the VFS seek helper,
 * else ENOSYS — never a fault.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* lseek() — reposition f_pos (SEEK_SET / SEEK_CUR / SEEK_END). */
long uiox_kix_scfs_lseek(uiox_reg_t fd, uiox_reg_t off, uiox_reg_t whence,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;

    if (f->f_op && f->f_op->seek)
        return (long)f->f_op->seek(f, (int64_t)off, (uint32_t)whence);

    if (!f->f_inode) return -SCFS_EBADF;
    return (long)vfs_seek_fallback(f, (int64_t)off, (uint32_t)whence);
}
