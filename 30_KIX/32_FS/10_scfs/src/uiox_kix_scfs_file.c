/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_file.c
 *
 * truncate / ftruncate / fallocate
 *
 * Bach Ch.4 — itrunc() frees every block past the new end when a file
 * shrinks; bmap_alloc() grows the block map for fallocate without writing
 * data.  The backend owns both; SCFS resolves the i-node and delegates.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* truncate() — set the file size by path. */
long uiox_kix_scfs_truncate(uiox_reg_t uptr, uiox_reg_t len,
                            uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode) return -SCFS_ENOENT;

    if (inode->i_op && inode->i_op->truncate)
        return inode->i_op->truncate(inode, (uint64_t)len);
    return -SCFS_ENOSYS;
}

/* ftruncate() — set the file size by descriptor. */
long uiox_kix_scfs_ftruncate(uiox_reg_t fd, uiox_reg_t len,
                             uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode) return -SCFS_EBADF;

    if (f->f_inode->i_op && f->f_inode->i_op->truncate)
        return f->f_inode->i_op->truncate(f->f_inode, (uint64_t)len);
    return -SCFS_ENOSYS;
}

/* fallocate() — pre-allocate blocks (bmap_alloc) without writing data.
 * mode == 0 is a plain allocation; any mode the backend cannot honour is
 * reported as a capability gap rather than silently succeeding. */
long uiox_kix_scfs_fallocate(uiox_reg_t fd, uiox_reg_t mode, uiox_reg_t offset,
                             uiox_reg_t len, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode || !f->f_inode->i_op) return -SCFS_EBADF;

    if (f->f_inode->i_op->fallocate)
        return f->f_inode->i_op->fallocate(f->f_inode, (uint32_t)mode,
                                           (uint64_t)offset, (uint64_t)len);
    return ((uint32_t)mode == 0u) ? SCFS_OK : -SCFS_ENOSYS;
}
