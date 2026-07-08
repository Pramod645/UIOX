/**
 * @file  uix_posix_fs.c
 * @brief UIOX POSIX — filesystem syscall implementations.
 */

 #include "uix_posix_fs.h"
 #include "uix_posix_io.h"
 
 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 /* ── Stat ────────────────────────────────────────────────────── */
 
 int uix_stat(const char *path, struct uix_stat *st)
 {
     return (int)_ret(uix_syscall2(SYS_STAT,(long)path,(long)st));
 }
 
 int uix_fstat(int fd, struct uix_stat *st)
 {
     return (int)_ret(uix_syscall2(SYS_FSTAT,(long)fd,(long)st));
 }
 
 int uix_lstat(const char *path, struct uix_stat *st)
 {
     return (int)_ret(uix_syscall2(SYS_LSTAT,(long)path,(long)st));
 }
 
 int uix_fstatat(int dirfd, const char *path,
                  struct uix_stat *st, int flags)
 {
     return (int)_ret(uix_syscall4(SYS_FSTATAT,
                       (long)dirfd,(long)path,(long)st,(long)flags));
 }
 
 /* ── Directories ─────────────────────────────────────────────── */
 
 int uix_mkdir(const char *path, mode_t mode)
 {
     return (int)_ret(uix_syscall2(SYS_MKDIR,(long)path,(long)mode));
 }
 
 int uix_mkdirat(int dirfd, const char *path, mode_t mode)
 {
     return (int)_ret(uix_syscall3(SYS_MKDIRAT,
                                    (long)dirfd,(long)path,(long)mode));
 }
 
 int uix_rmdir(const char *path)
 {
     return (int)_ret(uix_syscall1(SYS_RMDIR,(long)path));
 }
 
 /* ── File operations ─────────────────────────────────────────── */
 
 int uix_rename(const char *oldpath, const char *newpath)
 {
     return (int)_ret(uix_syscall2(SYS_RENAME,(long)oldpath,(long)newpath));
 }
 
 int uix_renameat(int olddirfd, const char *oldpath,
                   int newdirfd, const char *newpath)
 {
     return (int)_ret(uix_syscall4(SYS_RENAMEAT,
                       (long)olddirfd,(long)oldpath,
                       (long)newdirfd,(long)newpath));
 }
 
 int uix_link(const char *oldpath, const char *newpath)
 {
     return (int)_ret(uix_syscall2(SYS_LINK,(long)oldpath,(long)newpath));
 }
 
 int uix_linkat(int olddirfd, const char *oldpath,
                 int newdirfd, const char *newpath, int flags)
 {
     return (int)_ret(uix_syscall5(SYS_LINKAT,
                       (long)olddirfd,(long)oldpath,
                       (long)newdirfd,(long)newpath,(long)flags));
 }
 
 int uix_unlink(const char *path)
 {
     return (int)_ret(uix_syscall1(SYS_UNLINK,(long)path));
 }
 
 int uix_unlinkat(int dirfd, const char *path, int flags)
 {
     return (int)_ret(uix_syscall3(SYS_UNLINKAT,
                                    (long)dirfd,(long)path,(long)flags));
 }
 
 int uix_symlink(const char *target, const char *linkpath)
 {
     return (int)_ret(uix_syscall2(SYS_SYMLINK,(long)target,(long)linkpath));
 }
 
 int uix_symlinkat(const char *target, int newdirfd, const char *linkpath)
 {
     return (int)_ret(uix_syscall3(SYS_SYMLINKAT,
                                    (long)target,(long)newdirfd,
                                    (long)linkpath));
 }
 
 ssize_t uix_readlink(const char *path, char *buf, size_t bufsiz)
 {
     return (ssize_t)_ret(uix_syscall3(SYS_READLINK,
                                        (long)path,(long)buf,(long)bufsiz));
 }
 
 ssize_t uix_readlinkat(int dirfd, const char *path,
                         char *buf, size_t bufsiz)
 {
     return (ssize_t)_ret(uix_syscall4(SYS_READLINKAT,
                           (long)dirfd,(long)path,(long)buf,(long)bufsiz));
 }
 
 /* ── Permissions ─────────────────────────────────────────────── */
 
 int uix_chmod(const char *path, mode_t mode)
 {
     return (int)_ret(uix_syscall2(SYS_CHMOD,(long)path,(long)mode));
 }
 
 int uix_fchmod(int fd, mode_t mode)
 {
     return (int)_ret(uix_syscall2(SYS_FCHMOD,(long)fd,(long)mode));
 }
 
 int uix_fchmodat(int dirfd, const char *path, mode_t mode, int flags)
 {
     return (int)_ret(uix_syscall4(SYS_FCHMODAT,
                                    (long)dirfd,(long)path,
                                    (long)mode,(long)flags));
 }
 
 int uix_chown(const char *path, uid_t owner, gid_t group)
 {
     return (int)_ret(uix_syscall3(SYS_CHOWN,
                                    (long)path,(long)owner,(long)group));
 }
 
 int uix_fchown(int fd, uid_t owner, gid_t group)
 {
     return (int)_ret(uix_syscall3(SYS_FCHOWN,
                                    (long)fd,(long)owner,(long)group));
 }
 
 int uix_lchown(const char *path, uid_t owner, gid_t group)
 {
     return (int)_ret(uix_syscall3(SYS_LCHOWN,
                                    (long)path,(long)owner,(long)group));
 }
 
 int uix_fchownat(int dirfd, const char *path,
                   uid_t owner, gid_t group, int flags)
 {
     return (int)_ret(uix_syscall5(SYS_FCHOWNAT,
                       (long)dirfd,(long)path,
                       (long)owner,(long)group,(long)flags));
 }
 
 int uix_access(const char *path, int mode)
 {
     return (int)_ret(uix_syscall2(SYS_ACCESS,(long)path,(long)mode));
 }
 
 int uix_faccessat(int dirfd, const char *path, int mode, int flags)
 {
     return (int)_ret(uix_syscall4(SYS_FACCESSAT,
                                    (long)dirfd,(long)path,
                                    (long)mode,(long)flags));
 }
 
 int uix_mknod(const char *path, mode_t mode, dev_t dev)
 {
     return (int)_ret(uix_syscall3(SYS_MKNOD,
                                    (long)path,(long)mode,(long)dev));
 }
 
 /* ── Directory traversal (user-space DIR layer over getdents) ── */
 
 #define UIX_DENTS_BUF_SIZE  2048u
 
 struct uix_DIR {
     int           fd;
     int           buf_pos;
     int           buf_len;
     char          buf[UIX_DENTS_BUF_SIZE];
 };
 
 static struct uix_DIR s_dir_pool[8];  /* small fixed pool */
 static int            s_dir_used[8];
 
 static struct uix_DIR *dir_alloc(void)
 {
     for (int i = 0; i < 8; i++) {
         if (!s_dir_used[i]) { s_dir_used[i] = 1; return &s_dir_pool[i]; }
     }
     return NULL;
 }
 
 static void dir_free(struct uix_DIR *d)
 {
     for (int i = 0; i < 8; i++) {
         if (&s_dir_pool[i] == d) { s_dir_used[i] = 0; return; }
     }
 }
 
 UIX_DIR *uix_opendir(const char *name)
 {
     int fd = uix_open(name, O_RDONLY | O_DIRECTORY, 0);
     if (fd < 0) return NULL;
     struct uix_DIR *d = dir_alloc();
     if (!d) { uix_close(fd); uix_errno = ENOMEM; return NULL; }
     d->fd = fd; d->buf_pos = 0; d->buf_len = 0;
     return d;
 }
 
 UIX_DIR *uix_fdopendir(int fd)
 {
     struct uix_DIR *d = dir_alloc();
     if (!d) { uix_errno = ENOMEM; return NULL; }
     d->fd = fd; d->buf_pos = 0; d->buf_len = 0;
     return d;
 }
 
 struct uix_dirent *uix_readdir(UIX_DIR *dirp)
 {
     if (!dirp) return NULL;
     if (dirp->buf_pos >= dirp->buf_len) {
         long n = uix_syscall3(SYS_GETDENTS,
                                (long)dirp->fd,
                                (long)dirp->buf,
                                (long)UIX_DENTS_BUF_SIZE);
         if (n <= 0) return NULL;
         dirp->buf_len = (int)n;
         dirp->buf_pos = 0;
     }
     struct uix_dirent *de = (struct uix_dirent *)(dirp->buf + dirp->buf_pos);
     dirp->buf_pos += de->d_reclen;
     return de;
 }
 
 void uix_rewinddir(UIX_DIR *dirp)
 {
     if (!dirp) return;
     uix_lseek(dirp->fd, 0, SEEK_SET);
     dirp->buf_pos = dirp->buf_len = 0;
 }
 
 int uix_closedir(UIX_DIR *dirp)
 {
     if (!dirp) return -1;
     int r = uix_close(dirp->fd);
     dir_free(dirp);
     return r;
 }
 
 int uix_dirfd(UIX_DIR *dirp)
 {
     return dirp ? dirp->fd : -1;
 }
 