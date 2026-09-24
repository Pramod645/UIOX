#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm unlink, verbatim.
 * The name goes away immediately; the BLOCKS do not.  iput() frees them
 * only when i_nlink is 0 AND the inode's own reference count is 0 — which
 * is why a file removed while open keeps working.
 */
int uiox_kix_scfs_unlink(const char *path)
{
    InCoreInode *dir;
    InCoreInode *ip;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    uint32_t     ino;
    int          rc;

    if (!path) return SCFS_EFAULT;

    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    if (name[0] == '.' && (name[1] == '\0' ||
        (name[1] == '.' && name[2] == '\0'))) {
        return SCFS_EINVAL;
    }

    dir = namei(parent, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!dir) return SCFS_ENOENT;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, scfs_uid_get(), scfs_gid_get(), 0, 1, 1)) {
        iput(dir);
        return SCFS_EACCES;
    }

    ino = dir_lookup(dir, name);
    if (ino == 0u) { iput(dir); return SCFS_ENOENT; }

    ip = iget(ino);
    if (!ip) { iput(dir); return SCFS_EIO; }

    if (SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        iput(dir);
        return SCFS_EISDIR;
    }

    rc = dir_remove(dir, name);
    if (rc != 0) {
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    iput(dir);

    if (ip->nlink > 0u) ip->nlink--;
    ip->flags |= IFLAG_CHANGED;

    iupdate(ip);
    iput(ip);
    return SCFS_OK;
}
