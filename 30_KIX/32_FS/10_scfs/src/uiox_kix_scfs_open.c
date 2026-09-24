#include "uiox_kix_scfs_internal.h"

/*
 * Bach's Algorithm open.
 *
 * input:  path name, open flags, permission
 * output: user file descriptor
 * {
 *     get inode for file name (algorithm namei);
 *     if (file does not exist and file is not being created)
 *         return (error);
 *     ... five checks ...
 *     allocate file table entry (algorithm falloc);
 *     allocate user file descriptor entry;
 *     ...
 * }
 */
int uiox_kix_scfs_open(const char *path, int flags, uint16_t perm)
{
    InCoreInode *ip;
    scfs_file_t *f;
    int          fd;

    if (!path) return SCFS_EFAULT;

    /* O_CREAT with a path that does not exist goes to creat's body. */
    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) {
        if (flags & O_CREAT) return uiox_kix_scfs_creat(path, perm);
        return SCFS_ENOENT;
    }

    /* a directory may be opened read-only, for getdents */
    if (SCFS_IS_DIR(ip->mode) && ((flags & O_ACCMODE) != O_RDONLY)) {
        iput(ip);
        return SCFS_EISDIR;
    }
    if (SCFS_IS_DIR(ip->mode) && (flags & O_TRUNC)) {
        iput(ip);
        return SCFS_EISDIR;
    }

    if (flags & O_TRUNC) {
        if ((flags & O_ACCMODE) == O_RDONLY) {
            iput(ip);
            return SCFS_EACCES;
        }
        fs_free_inode_blocks(ip);
        ip->size = 0;
        ip->flags |= IFLAG_CHANGED | IFLAG_MODIFIED;
        iupdate(ip);
    }

    if (!scfs_perm_test(ip, (flags & O_ACCMODE) == O_RDONLY ? SCFS_R_OK
                          : (flags & O_ACCMODE) == O_WRONLY ? SCFS_W_OK
                          : (SCFS_R_OK | SCFS_W_OK))) {
        iput(ip);
        return SCFS_EACCES;
    }

    f = scfs_falloc(ip, flags, 0u);
    if (!f) { iput(ip); return SCFS_ENFILE; }

    fd = scfs_ufd_alloc(f);
    if (fd < 0) { scfs_fclose_entry(f); return fd; }

    iput(ip);
    return fd;
}
