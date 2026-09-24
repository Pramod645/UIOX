#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm creat, verbatim.
 * input: file name, permission settings
 * output: file descriptor
 * {
 *   get inode for file name (algorithm namei);
 *   if (file already exists)
 *   {
 *       if (not permitted access) { release inode; return (error); }
 *   }
 *   else
 *   {
 *       assign free inode from file system (algorithm ialloc);
 *       create new directory entry in parent directory;
 *   }
 *   allocate file table entry for inode, initialize count;
 *   if (file did exist at time of create)
 *       free all file blocks (algorithm free);
 *   unlock (inode);
 *   return (user file descriptor);
 * }
 */
int uiox_kix_scfs_creat(const char *path, uint16_t perm)
{
    InCoreInode *ip;
    scfs_file_t *f;
    int          existed;
    int          fd;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    existed = (ip != (InCoreInode *)0);

    if (existed) {
        if (!inode_access_ok(ip, scfs_uid_get(), scfs_gid_get(), 0, 1, 0)) {
            iput(ip);
            return SCFS_EACCES;
        }
        if (SCFS_IS_DIR(ip->mode)) {
            iput(ip);
            return SCFS_EISDIR;
        }
    } else {
        ip = scfs_create_node(path, scfs_apply_umask(perm));
        if (!ip) return SCFS_EACCES;
    }

    f = scfs_falloc(ip, O_WRONLY, 0777u);
    if (!f) { iput(ip); return SCFS_ENFILE; }

    fd = scfs_ufd_alloc(f);
    if (fd < 0) {
        scfs_fclose_entry(f);
        iput(ip);
        return SCFS_EMFILE;
    }

    if (existed && ip->size != 0u) {
        fs_free_inode_blocks(ip);
        iupdate(ip);
    }

    ip->locked = false;
    iput(ip);
    return fd;
}
