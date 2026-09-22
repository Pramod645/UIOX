/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mkdir.c
 *
 * mkdir / rmdir / mknod / dirname
 *
 * Bach Ch.4 — ialloc() allocates an i-node, ifree() frees it; Ch.5 — a new
 * directory is linked into its parent by name.  mknod creates a special
 * i-node whose device number sits in the i-node.  dirname() is defined ONCE
 * here, as a global, and every other unit calls it through the shared header.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* dirname() — split a path into its directory component (Bach Ch.5).
 * SINGLE GLOBAL DEFINITION — mkdir.c owns it. */
int uiox_kix_scfs_dirname(const char *path, char *out, size_t outlen)
{
    if (!path || !out || outlen == 0u) return -SCFS_EINVAL;

    size_t n = 0u;
    while (path[n]) n++;                       /* strlen, freestanding-safe */

    size_t end = n;
    while (end > 1u && path[end - 1u] == '/') end--;

    size_t cut = end;
    while (cut > 0u && path[cut - 1u] != '/') cut--;

    if (cut == 0u) {                           /* no slash — current dir */
        out[0] = '.';
        out[1] = '\0';
        return 1;
    }

    if (cut >= outlen) return -SCFS_EINVAL;
    for (size_t i = 0u; i < cut; i++) out[i] = path[i];
    out[cut] = '\0';
    return (int)cut;
}

/* mkdir() — allocate an i-node, link "." and "..", enter the name in the
 * parent directory. */
long uiox_kix_scfs_mkdir(uiox_reg_t uptr, uiox_reg_t mode,
                         uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *parent = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &parent, UIOX_VFS_PARENT);
    if (rc < 0) return rc;
    if (!parent || !parent->i_op || !parent->i_op->mkdir)
        return -SCFS_ENOSYS;

    return parent->i_op->mkdir(parent, (const char *)uptr,
                               (uint32_t)mode & 0x0FFFu);
}

/* rmdir() — remove an empty directory: unlink it from the parent, then
 * ifree() the i-node and fs_free() its block (Bach Ch.4). */
long uiox_kix_scfs_rmdir(uiox_reg_t uptr,
                         uiox_reg_t a1, uiox_reg_t a2, uiox_reg_t a3,
                         uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode) return -SCFS_ENOENT;
    if (!(inode->i_mode & UIOX_S_IFDIR)) return -SCFS_ENOTDIR;
    if (inode->i_size != 0u) return -SCFS_EBUSY;   /* not empty */

    return (inode->i_op && inode->i_op->rmdir)
         ? inode->i_op->rmdir(inode)
         : -SCFS_ENOSYS;
}

/* mknod() — create a character device, block device, or FIFO.  Bach Ch.4:
 * the device number is stored in the i-node; the node has no data blocks. */
long uiox_kix_scfs_mknod(uiox_reg_t uptr, uiox_reg_t mode, uiox_reg_t dev,
                         uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a3; (void)a4; (void)a5;
    if (uptr == 0u) return -SCFS_EFAULT;

    uint32_t type = (uint32_t)mode & UIOX_S_IFMT;
    if (type != UIOX_S_IFCHR && type != UIOX_S_IFBLK && type != UIOX_S_IFIFO)
        return -SCFS_EINVAL;                    /* mknod makes specials only */

    char dir[256];
    if (uiox_kix_scfs_dirname((const char *)uptr, dir, sizeof(dir)) < 0)
        return -SCFS_EINVAL;

    uiox_inode_t *parent = (uiox_inode_t *)0;
    long rc = vfs_path_lookup(dir, &parent, 0u);
    if (rc < 0) return rc;
    if (!parent || !parent->i_op || !parent->i_op->mknod)
        return -SCFS_ENOSYS;

    uiox_inode_t *node = (uiox_inode_t *)0;
    rc = parent->i_op->mknod(parent, (const char *)uptr, (uint32_t)mode,
                             (uint32_t)dev, &node);
    if (rc < 0) return rc;

    if (node) {
        node->i_rdev = (uint32_t)dev;
        node->i_size = 0u;
    }
    return SCFS_OK;
}
