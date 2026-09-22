/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_fs.h   — v3.0.0
 *
 * UIOX filesystem front end.  Bach's fs_*() syscall bodies, dispatched
 * through the ops tables.
 *
 * GAP FIXES APPLIED (#1 64-bit sizes, #5 write-ordering, #6 TRIM,
 *                     #7 64-bit time, #8 xattr)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_FS_H
#define UIOX_KIX_SCFS_FS_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_file.h"
#include "uiox_kix_scfs_mount.h"
#include "uiox_kix_scfs_buf.h"
#include "uiox_kix_scfs_ops.h"
#include "uiox_klibc.h"

typedef struct stat {
    uint32_t       st_dev;
    uint32_t       st_ino;
    uint16_t       st_mode;
    uint16_t       st_nlink;
    uint16_t       st_uid;
    uint16_t       st_gid;
    uint64_t       st_size;          /* GAP #1 */
    uiox_time64_t  st_atime;         /* GAP #7 */
    uiox_time64_t  st_mtime;
    uiox_time64_t  st_ctime;
    uiox_time64_t  st_btime;
    uint8_t        st_major;
    uint8_t        st_minor;
} stat_t;

#define MAXNAMLEN   255
typedef struct dirent {
    uint64_t d_ino;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[MAXNAMLEN + 1];
} dirent_t;

#define DT_UNKNOWN 0
#define DT_REG     1
#define DT_DIR     2
#define DT_CHR     3
#define DT_BLK     4
#define DT_FIFO    5
#define DT_LNK     6

/* error codes — THE canonical set */
#define FS_OK        0
#define FS_ENOENT   -1
#define FS_EACCES   -2
#define FS_EEXIST   -3
#define FS_ENFILE   -4
#define FS_EBADF    -5
#define FS_ENOTDIR  -6
#define FS_EISDIR   -7
#define FS_EMLINK   -8
#define FS_EPERM    -9
#define FS_EBUSY   -10
#define FS_EROFS   -11
#define FS_EXDEV   -12
#define FS_EINVAL  -13
#define FS_ENOSYS  -14
#define FS_EFAULT  -15
#define FS_ENOMEM  -16
#define FS_ERANGE  -17
#define FS_ENOSPC  -18   /* no free blocks (GAP #2) */
#define FS_EFBIG   -19   /* file exceeds backend max  */

/* sync modes (GAP #5) */
#define UIOX_SYNC_LAZY    0
#define UIOX_SYNC_ORDERED 1
#define UIOX_SYNC_FULL    2

int  fs_open   (const char *path, int flags, uint16_t mode);
int  fs_read   (int fd, char *buf, uint32_t count);
int  fs_write  (int fd, const char *buf, uint32_t count);
int  fs_pread  (int fd, char *buf, uint32_t count, uint64_t off);
int  fs_pwrite (int fd, const char *buf, uint32_t count, uint64_t off);
int  fs_lseek  (int fd, int64_t offset, int whence);
int  fs_close  (int fd);
int  fs_dup    (int fd);
int  fs_fcntl  (int fd, int cmd, int arg);
int  fs_creat  (const char *path, uint16_t mode);
int  fs_mknod  (const char *path, uint16_t mode, uint8_t major, uint8_t minor);
int  fs_mkdir  (const char *path, uint16_t mode);
int  fs_rmdir  (const char *path);
int  fs_chdir  (const char *path);
int  fs_fchdir (int fd);
int  fs_chroot (const char *path);
int  fs_getcwd (char *buf, uint32_t size);
int  fs_stat   (const char *path, stat_t *buf);
int  fs_fstat  (int fd, stat_t *buf);
int  fs_lstat  (const char *path, stat_t *buf);
int  fs_link   (const char *old, const char *new);
int  fs_unlink (const char *path);
int  fs_symlink(const char *target, const char *link);
int  fs_readlink(const char *path, char *buf, uint32_t len);
int  fs_rename (const char *old, const char *new);
int  fs_access (const char *path, int mode);
int  fs_chmod  (const char *path, uint16_t mode);
int  fs_truncate(const char *path, uint64_t size);   /* GAP #1 */
int  fs_ftruncate(int fd, uint64_t size);            /* GAP #1 */
int  fs_getdents64(int fd, void *buf, uint32_t count);
int  fs_fsync  (int fd);
int  fs_sync   (void);
int  fs_sync_mode(int mode);                          /* GAP #5 */
int  fs_discard(uint16_t dev, uint64_t offset, uint64_t len); /* GAP #6 */
int  fs_mount  (const char *dev, const char *dir, int flags);
int  fs_umount (const char *dir, int flags);
int  fs_statfs (const char *path, void *buf);
int  fs_statvfs(const char *path, void *buf);
int  fs_ioctl  (int fd, uint32_t req, void *arg);
int  fs_mmap   (int fd, uint32_t length, uint32_t prot, uint32_t flags,
                uint64_t offset);                     /* GAP #1 */

/* xattr (GAP #8) */
int  fs_setxattr(const char *path, const char *name,
                 const void *val, uint32_t len, int flags);
int  fs_getxattr(const char *path, const char *name, void *val, uint32_t len);
int  fs_listxattr(const char *path, char *list, uint32_t size);
int  fs_removexattr(const char *path, const char *name);

int  fs_register_fs(const char *name,
                    const uiox_inode_ops_t *iop,
                    const uiox_file_ops_t  *fop);

#endif /* UIOX_KIX_SCFS_FS_H */
