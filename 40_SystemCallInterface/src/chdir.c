#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"

/*
 * Algorithm chdir
 * input : new directory name
 * output: none (0 on success)
 */
int fs_chdir(const char *path)
{
    /* Get inode for new directory name (algorithm namei) */
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;

    /* Must be a directory and accessible */
    if ((ip->i_mode & IFMT) != IFDIR) {
        iput(ip);
        return FS_ENOTDIR;
    }
    if (!iaccess(ip, 1)) {
        iput(ip);
        return FS_EACCES;
    }

    iunlock(ip);

    /* Release old current directory inode (algorithm iput) */
    if (u.u_cdir)
        iput(u.u_cdir);

    /* Place new inode into current directory slot in u area */
    u.u_cdir = ip;
    return FS_OK;
}

/*
 * Algorithm chroot
 * input : new root directory name
 * output: none (0 on success)
 */
int fs_chroot(const char *path)
{
    if (u.u_uid != 0)           /* super user only */
        return FS_EPERM;

    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;

    if ((ip->i_mode & IFMT) != IFDIR) {
        iput(ip);
        return FS_ENOTDIR;
    }

    iunlock(ip);

    if (u.u_rdir)
        iput(u.u_rdir);

    u.u_rdir = ip;
    return FS_OK;
}

/*
 * Algorithm chown
 * input : file name, new uid, new gid
 * output: none (0 on success)
 */
int fs_chown(const char *path, uint16_t uid, uint16_t gid)
{
    if (u.u_uid != 0)
        return FS_EPERM;

    /* Convert file name to inode (algorithm namei) */
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;

    ip->i_uid   = uid;
    ip->i_gid   = gid;
    ip->i_flag |= IUPD;

    /* Release inode (algorithm iput) */
    iput(ip);
    return FS_OK;
}

/*
 * Algorithm chmod
 * input : file name, new permission mode
 * output: none (0 on success)
 */
int fs_chmod(const char *path, uint16_t mode)
{
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;

    if (u.u_uid != 0 && u.u_uid != ip->i_uid) {
        iput(ip);
        return FS_EPERM;
    }

    /* Change permission bits in inode (keep file type bits) */
    ip->i_mode  = (ip->i_mode & IFMT) | (mode & 07777);
    ip->i_flag |= IUPD;

    iput(ip);
    return FS_OK;
}

/*
 * Algorithm stat
 * input : file name, stat buffer
 * output: 0 on success
 */
int fs_stat(const char *path, stat_t *st)
{
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;

    st->st_dev   = ip->i_dev;
    st->st_ino   = ip->i_number;
    st->st_mode  = ip->i_mode;
    st->st_nlink = ip->i_nlink;
    st->st_uid   = ip->i_uid;
    st->st_gid   = ip->i_gid;
    st->st_size  = ip->i_size;
    st->st_atime = ip->i_atime;
    st->st_mtime = ip->i_mtime;
    st->st_ctime = ip->i_ctime;
    st->st_major = ip->i_major;
    st->st_minor = ip->i_minor;

    iput(ip);
    return FS_OK;
}

/*
 * Algorithm fstat
 * input : file descriptor, stat buffer
 * output: 0 on success
 */
int fs_fstat(int fd, stat_t *st)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t *fp = u.u_ofile.ufd_file[fd];
    if (!fp || !fp->f_inode) return FS_EBADF;

    inode_t *ip = fp->f_inode;

    st->st_dev   = ip->i_dev;
    st->st_ino   = ip->i_number;
    st->st_mode  = ip->i_mode;
    st->st_nlink = ip->i_nlink;
    st->st_uid   = ip->i_uid;
    st->st_gid   = ip->i_gid;
    st->st_size  = ip->i_size;
    st->st_atime = ip->i_atime;
    st->st_mtime = ip->i_mtime;
    st->st_ctime = ip->i_ctime;
    st->st_major = ip->i_major;
    st->st_minor = ip->i_minor;

    return FS_OK;
}
