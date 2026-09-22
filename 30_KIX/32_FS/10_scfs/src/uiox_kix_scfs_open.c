/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_open.c
 *
 * open / creat
 *
 * Bach Ch.5 — namei() resolves the path; ialloc() creates a new i-node;
 * Ch.7 falloc() allocates the descriptor + file-table entry.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* open() — resolve the path, create when O_CREAT, truncate when O_TRUNC,
 * then allocate a descriptor and return it. */
long uiox_kix_scfs_open(uiox_reg_t uptr, uiox_reg_t flags, uiox_reg_t mode,
                        uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, (uint32_t)flags);

    if (rc < 0) {
        /* O_CREAT: the path does not exist — create it in its parent. */
        if (!((uint32_t)flags & UIOX_O_CREAT)) return rc;

        char dir[256];
        if (uiox_kix_scfs_dirname((const char *)uptr, dir, sizeof(dir)) < 0)
            return -SCFS_EINVAL;

        uiox_inode_t *parent = (uiox_inode_t *)0;
        rc = vfs_path_lookup(dir, &parent, 0u);
        if (rc < 0) return rc;
        if (!parent || !parent->i_op || !parent->i_op->create)
            return -SCFS_ENOSYS;

        uiox_inode_t *created = (uiox_inode_t *)0;
        rc = parent->i_op->create(parent, (const char *)uptr,
                                  (uint32_t)mode & 0x0FFFu, &created);
        if (rc < 0) return rc;
        inode = created;
    } else if ((uint32_t)flags & UIOX_O_TRUNC) {
        if (inode->i_op && inode->i_op->truncate) {
            rc = inode->i_op->truncate(inode, 0u);
            if (rc < 0) return rc;
        }
    }

    return (long)vfs_fd_alloc(inode, (uint32_t)flags, (uint32_t)mode & 0x0FFFu);
}

/* creat() — Bach Ch.5: equivalent to open(path, O_CREAT|O_WRONLY|O_TRUNC). */
long uiox_kix_scfs_creat(uiox_reg_t uptr, uiox_reg_t mode,
                         uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    return uiox_kix_scfs_open(uptr,
                              UIOX_O_CREAT | UIOX_O_WRONLY | UIOX_O_TRUNC,
                              mode, 0, 0, 0);
}
