#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm chdir, verbatim.
 * input: new directory name
 * output: none
 * {
 *   get inode for new directory name (algorithm namei);
 *   if (inode not that of directory or process not permitted access)
 *   {
 *       release inode (algorithm iput);
 *       return (error);
 *   }
 *   unlock (inode);
 *   release "old" current directory inode (algorithm iput);
 *   place new inode into current directory slot in u area;
 * }
 *
 * Bach's note: the u-area slot HOLDS a reference on the cwd, which is why
 * the old one is iput() here and the new one is not.
 */
int uiox_kix_scfs_chdir(const char *path)
{
    InCoreInode *ip;
    InCoreInode *old;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    if (!SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        return SCFS_ENOTDIR;
    }
    if (!inode_access_ok(ip, scfs_uid_get(), scfs_gid_get(), 0, 0, 1)) {
        iput(ip);
        return SCFS_EACCES;
    }

    ip->locked = false;

    old = scfs_cwd_get();
    if (old && old != ip) iput(old);

    scfs_cwd_set(ip);
    return SCFS_OK;
}
