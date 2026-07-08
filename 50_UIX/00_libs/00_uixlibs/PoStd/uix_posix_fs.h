/**
 * @file  uix_posix_fs.h
 * @brief UIOX POSIX — filesystem syscall wrappers.
 *        stat, fstat, lstat, mkdir, rmdir, rename, link, unlink,
 *        symlink, readlink, chmod, chown, access, opendir, readdir.
 */

 #ifndef UIX_POSIX_FS_H
 #define UIX_POSIX_FS_H
 
 #include "uix_syscall.h"
 #include "../sys/uix_stat.h"
 #include "uix_dirent.h"
 #include "../sys/uix_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Stat family
  * ====================================================================== */
 
 int  uix_stat      (const char *path, struct uix_stat *st);
 int  uix_fstat     (int fd,           struct uix_stat *st);
 int  uix_lstat     (const char *path, struct uix_stat *st);
 int  uix_fstatat   (int dirfd, const char *path,
                      struct uix_stat *st, int flags);
 
 /* =========================================================================
  * Directory operations
  * ====================================================================== */
 
 int  uix_mkdir     (const char *path, mode_t mode);
 int  uix_mkdirat   (int dirfd, const char *path, mode_t mode);
 int  uix_rmdir     (const char *path);
 
 /* =========================================================================
  * File operations
  * ====================================================================== */
 
 int  uix_rename    (const char *oldpath, const char *newpath);
 int  uix_renameat  (int olddirfd, const char *oldpath,
                      int newdirfd, const char *newpath);
 int  uix_link      (const char *oldpath, const char *newpath);
 int  uix_linkat    (int olddirfd, const char *oldpath,
                      int newdirfd, const char *newpath, int flags);
 int  uix_unlink    (const char *path);
 int  uix_unlinkat  (int dirfd, const char *path, int flags);
 int  uix_symlink   (const char *target, const char *linkpath);
 int  uix_symlinkat (const char *target, int newdirfd,
                      const char *linkpath);
 ssize_t uix_readlink(const char *path, char *buf, size_t bufsiz);
 ssize_t uix_readlinkat(int dirfd, const char *path,
                         char *buf, size_t bufsiz);
 
 /* =========================================================================
  * Permissions
  * ====================================================================== */
 
 int  uix_chmod     (const char *path, mode_t mode);
 int  uix_fchmod    (int fd,           mode_t mode);
 int  uix_fchmodat  (int dirfd, const char *path, mode_t mode, int flags);
 int  uix_chown     (const char *path, uid_t owner, gid_t group);
 int  uix_fchown    (int fd,           uid_t owner, gid_t group);
 int  uix_lchown    (const char *path, uid_t owner, gid_t group);
 int  uix_fchownat  (int dirfd, const char *path,
                      uid_t owner, gid_t group, int flags);
 int  uix_access    (const char *path, int mode);
 int  uix_faccessat (int dirfd, const char *path, int mode, int flags);
 
 /* =========================================================================
  * Directory traversal (user-space layer over getdents)
  * ====================================================================== */
 
 struct uix_DIR;
 typedef struct uix_DIR UIX_DIR;
 
 UIX_DIR        *uix_opendir  (const char *name);
 UIX_DIR        *uix_fdopendir(int fd);
 struct uix_dirent *uix_readdir(UIX_DIR *dirp);
 void            uix_rewinddir(UIX_DIR *dirp);
 int             uix_closedir (UIX_DIR *dirp);
 int             uix_dirfd    (UIX_DIR *dirp);
 
 /* =========================================================================
  * mknod / device files
  * ====================================================================== */
 
 int  uix_mknod     (const char *path, mode_t mode, dev_t dev);
 int  uix_mknodat   (int dirfd, const char *path,
                      mode_t mode, dev_t dev);
 
 /* POSIX aliases */
 #define stat        uix_stat
 #define fstat       uix_fstat
 #define lstat       uix_lstat
 #define mkdir       uix_mkdir
 #define rmdir       uix_rmdir
 #define rename      uix_rename
 #define link        uix_link
 #define unlink      uix_unlink
 #define symlink     uix_symlink
 #define readlink    uix_readlink
 #define chmod       uix_chmod
 #define fchmod      uix_fchmod
 #define chown       uix_chown
 #define fchown      uix_fchown
 #define lchown      uix_lchown
 #define access      uix_access
 #define opendir     uix_opendir
 #define readdir     uix_readdir
 #define closedir    uix_closedir
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIX_POSIX_FS_H */
 