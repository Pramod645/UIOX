#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm chroot — the same shape as chdir, with one addition:
 * it is privileged, because it changes what "/" means for everything the
 * process does afterwards.
 *
 *   input:  new root directory name
 *   output: none
 *   {
 *       if (not super user) return (error);
 *       get inode for the new root directory name (algorithm namei);
 *       if (inode not that of directory)
 *       { release inode (algorithm iput); return (error); }
 *       unlock (inode);
 *       release "old" root directory inode (algorithm iput);
 *       place new inode into root directory slot in u area;
 *   }
 */
int uiox_kix_scfs_chroot(const char *path)
{
    InCoreInode *ip;
    InCoreInode *old;

    if (!path) return SCFS_EFAULT;

    if (!scfs_is_super()) return SCFS_EPERM;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    if (!SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        return SCFS_ENOTDIR;
    }

    ip->locked = false;

    old = scfs_root_get();
    if (old && old != ip) iput(old);

    scfs_root_set(ip);
    return SCFS_OK;
}
