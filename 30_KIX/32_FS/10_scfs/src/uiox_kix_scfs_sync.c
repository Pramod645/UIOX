/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_sync.c
 *
 * fsync / fdatasync / sync / msync
 *
 * Bach Ch.3 — the buffer cache holds dirty blocks; fsync/fdatasync write a
 * file's dirty buffers back to disk, sync() flushes the whole cache.  msync
 * does the same for a memory mapping, so it lives with the other durability
 * calls rather than beside mmap().
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* fsync() — flush the file's data and its i-node to stable storage. */
long uiox_kix_scfs_fsync(uiox_reg_t fd,
                         uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                         uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;

    if (f->f_op && f->f_op->fsync) { f->f_op->fsync(f); return SCFS_OK; }
    return vfs_sync_inode(f->f_inode);
}

/* fdatasync() — flush file data; the i-node write is optional when the size
 * did not change. */
long uiox_kix_scfs_fdatasync(uiox_reg_t fd,
                             uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                             uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    return vfs_sync_inode(f->f_inode);
}

/* sync() — flush every dirty buffer in the system. */
long uiox_kix_scfs_sync(uiox_reg_t a0, uiox_reg_t a1, uiox_reg_t a2,
                        uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return vfs_sync_all();
}

/* msync() — flush modified pages of a mapping back to backing store. */
long uiox_kix_scfs_msync(uiox_reg_t addr, uiox_reg_t length, uiox_reg_t flags,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if (addr == 0u || length == 0u) return -SCFS_EINVAL;
    return vfs_msync((void *)addr, (uint64_t)length, (uint32_t)flags);
}
