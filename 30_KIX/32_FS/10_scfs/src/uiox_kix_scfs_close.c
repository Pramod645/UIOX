/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_close.c
 *
 * close / fclose
 *
 * Bach Ch.7 close() — drop the descriptor, decrement the file-table
 * reference count, and iput() the i-node only when no other descriptor
 * still shares the entry.  A deleted-but-open file keeps working for
 * exactly this reason.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* fclose() — flush, release the file-table reference, iput() on last use. */
long uiox_kix_scfs_fclose(uiox_reg_t fd,
                          uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                          uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;

    if (f->f_op && f->f_op->close) f->f_op->close(f);

    if (f->f_count > 0u) f->f_count--;

    if (f->f_count == 0u && f->f_inode &&
        f->f_inode->i_op && f->f_inode->i_op->iput)
        f->f_inode->i_op->iput(f->f_inode);

    return vfs_fd_free((uint32_t)fd);
}

/* close() — the SCFS_NR_close entry; identical to fclose(). */
long uiox_kix_scfs_close(uiox_reg_t fd,
                         uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                         uiox_reg_t a4, uiox_reg_t a5)
{
    return uiox_kix_scfs_fclose(fd, a1, a2, a3, a4, a5);
}
