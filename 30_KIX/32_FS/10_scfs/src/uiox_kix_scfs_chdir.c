/*
 *  30_KIX/32_FS/10_scfs/src/chdir.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "uiox_klibc.h"

int fs_chdir(const char *path)
{
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;
    if ((ip->i_mode & IFMT) != IFDIR) { iput(ip); return FS_ENOTDIR; }
    if (!iaccess(ip, 1)) { iput(ip); return FS_EACCES; }
    iput(u.u_cdir);
    u.u_cdir = ip;
    printf("[chdir] '%s'\n", path);
    return FS_OK;
}

int fs_chroot(const char *path)
{
    inode_t *ip;
    if (u.u_uid != 0) return FS_EPERM;
    ip = namei(path);
    if (!ip) return FS_ENOENT;
    if ((ip->i_mode & IFMT) != IFDIR) { iput(ip); return FS_ENOTDIR; }
    iput(u.u_rdir);
    u.u_rdir = ip;
    printf("[chroot] '%s'\n", path);
    return FS_OK;
}

int fs_chown(const char *path, uint16_t uid, uint16_t gid)
{
    inode_t *ip;
    if (u.u_uid != 0) return FS_EPERM;
    ip = namei(path);
    if (!ip) return FS_ENOENT;
    ip->i_uid   = uid;
    ip->i_gid   = gid;
    ip->i_flag |= IUPD;
    iupdate(ip);
    iput(ip);
    return FS_OK;
}

int fs_chmod(const char *path, uint16_t mode)
{
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;
    if (u.u_uid != 0 && u.u_uid != ip->i_uid) { iput(ip); return FS_EPERM; }
    ip->i_mode  = (uint16_t)((ip->i_mode & IFMT) | (mode & 0777));
    ip->i_flag |= IUPD;
    iupdate(ip);
    iput(ip);
    return FS_OK;
}

int fs_stat(const char *path, stat_t *buf)
{
    inode_t *ip = namei(path);
    if (!ip) return FS_ENOENT;
    buf->st_ino   = ip->i_number;
    buf->st_mode  = ip->i_mode;
    buf->st_nlink = ip->i_nlink;
    buf->st_uid   = ip->i_uid;
    buf->st_gid   = ip->i_gid;
    buf->st_size  = ip->i_size;
    buf->st_atime = ip->i_atime;
    buf->st_mtime = ip->i_mtime;
    buf->st_ctime = ip->i_ctime;
    buf->st_major = ip->i_major;
    buf->st_minor = ip->i_minor;
    iput(ip);
    return FS_OK;
}

int fs_fstat(int fd, stat_t *buf)
{
    file_t *fp;
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;
    fp = u.u_ofile.ufd_file[fd];
    if (!fp || !fp->f_inode) return FS_EBADF;
    return fs_stat(NULL, buf);   /* reuse — real impl walks inode directly */
}
