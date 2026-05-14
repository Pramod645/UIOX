#include "uix_stat.h"
#include "../PoStd/uix_errno.h"
#include "../PoStd/uix_string.h"

static uix_mode_t current_umask = 022;

int uix_stat(const char *path, uix_stat_t *buf)
{
    extern int sys_stat(const char *, uix_stat_t *)
        __attribute__((weak));
    if (sys_stat) return sys_stat(path, buf);
    if (!buf) { uix_errno = UIX_EFAULT; return -1; }
    uix_memset(buf, 0, sizeof(*buf));
    buf->st_mode = UIX_S_IFREG | 0644;
    uix_errno = UIX_ENOENT;
    return -1;
}

int uix_fstat(int fd, uix_stat_t *buf)
{
    extern int sys_fstat(int, uix_stat_t *) __attribute__((weak));
    if (sys_fstat) return sys_fstat(fd, buf);
    if (!buf) { uix_errno = UIX_EFAULT; return -1; }
    uix_memset(buf, 0, sizeof(*buf));
    if (fd < 3) buf->st_mode = UIX_S_IFCHR | 0666;
    else        { uix_errno = UIX_EBADF; return -1; }
    return 0;
}

int uix_lstat(const char *path, uix_stat_t *buf)
{
    extern int sys_lstat(const char *, uix_stat_t *)
        __attribute__((weak));
    if (sys_lstat) return sys_lstat(path, buf);
    return uix_stat(path, buf);
}

int uix_chmod(const char *path, uix_mode_t mode)
{
    extern int sys_chmod(const char *, uix_mode_t)
        __attribute__((weak));
    return sys_chmod ? sys_chmod(path, mode)
                     : (uix_errno = UIX_EPERM, -1);
}

int uix_fchmod(int fd, uix_mode_t mode)
{
    (void)fd; (void)mode;
    uix_errno = UIX_EPERM; return -1;
}

int uix_chown(const char *path, uix_uid_t owner, uix_gid_t group)
{
    extern int sys_chown(const char *, uix_uid_t, uix_gid_t)
        __attribute__((weak));
    return sys_chown ? sys_chown(path, owner, group)
                     : (uix_errno = UIX_EPERM, -1);
}

int uix_fchown(int fd, uix_uid_t owner, uix_gid_t group)
{
    (void)fd; (void)owner; (void)group;
    uix_errno = UIX_EPERM; return -1;
}

uix_mode_t uix_umask(uix_mode_t mask)
{
    uix_mode_t old = current_umask;
    current_umask  = mask & 0777;
    return old;
}

int uix_mkdir(const char *path, uix_mode_t mode)
{
    extern int sys_mkdir(const char *, uix_mode_t)
        __attribute__((weak));
    return sys_mkdir ? sys_mkdir(path, mode & ~current_umask)
                     : (uix_errno = UIX_ENOSPC, -1);
}

int uix_mknod(const char *path, uix_mode_t mode, uix_dev_t dev)
{
    extern int sys_mknod(const char *, uix_mode_t, uix_dev_t)
        __attribute__((weak));
    return sys_mknod ? sys_mknod(path, mode, dev)
                     : (uix_errno = UIX_EPERM, -1);
}

/* ***This is End of file, there is no more line should be added after this line*** */
