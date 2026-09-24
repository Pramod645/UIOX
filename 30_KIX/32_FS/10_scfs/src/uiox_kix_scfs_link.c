#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm link, verbatim — no new inode is allocated.
 * Two names point at one inode; i_nlink counts how many.
 */
int uiox_kix_scfs_link(const char *oldpath, const char *newpath)
{
    InCoreInode *ip;
    InCoreInode *dir;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    int          rc;

    if (!oldpath || !newpath) return SCFS_EFAULT;

    ip = namei(oldpath, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    if (ip->nlink >= 32767u) { iput(ip); return SCFS_EMLINK; }
    if (SCFS_IS_DIR(ip->mode)) { iput(ip); return SCFS_EPERM; }

    ip->nlink++;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    ip->locked = false;

    rc = scfs_path_split(newpath, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) {
        ip->nlink--; iupdate(ip); iput(ip); return rc;
    }

    dir = namei(parent, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!dir) {
        ip->nlink--; iupdate(ip); iput(ip); return SCFS_ENOENT;
    }

    if (dir_lookup(dir, name) != 0u) {
        dir->locked = false; iput(dir);
        ip->nlink--; iupdate(ip); iput(ip);
        return SCFS_EEXIST;
    }

    rc = dir_add(dir, name, ip->ino);
    if (rc != 0) {
        dir->locked = false; iput(dir);
        ip->nlink--; iupdate(ip); iput(ip);
        return SCFS_EIO;
    }

    dir->locked = false;
    iput(dir);
    iput(ip);
    return SCFS_OK;
}
