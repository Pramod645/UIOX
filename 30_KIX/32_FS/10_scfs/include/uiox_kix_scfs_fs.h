/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_fs.h   — modernised
 *
 * UIOX filesystem front end.  Bach's fs_*() prototypes are kept (they are
 * the syscall bodies); dispatch goes through the ops tables.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_FS_H
#define UIOX_KIX_SCFS_FS_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_file.h"
#include "uiox_kix_scfs_mount.h"
#include "uiox_kix_scfs_buf.h"
#include "uiox_kix_scfs_ops.h"
#include "uiox_klibc.h"

/* ── stat ───────────────────────────────────────────────────────────── */
typedef struct stat {
    uint32_t st_dev;        /* widened: device numbers exceed 16 bits  */
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint64_t st_size;       /* widened: files beyond 4 GB              */
    time_t   st_atime;
    time_t   st_mtime;
    time_t   st_ctime;
    uint8_t  st_major;
    uint8_t  st_minor;
} stat_t;

/* ── directory entry ────────────────────────────────────────────────── */
#define MAXNAMLEN   255     /* widened from v7's 14 */
typedef struct dirent {
    uint32_t d_ino;
    uint16_t d_reclen;      /* getdents64 needs a stride field */
    uint8_t  d_type;        /* DT_REG / DT_DIR / ... */
    char     d_name[MAXNAMLEN + 1];
} dirent_t;

/* d_type values */
#define DT_UNKNOWN 0
#define DT_REG     1
#define DT_DIR     2
#define DT_CHR     3
#define DT_BLK     4
#define DT_FIFO    5
#define DT_LNK     6

/* ── error codes — THE canonical set (SCFS aliases these) ───────────── */
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

/* ── syscall bodies (Bach Ch.5/7; called by scfs_dispatch) ──────────── */
int  fs_open   (const char *path, int flags, uint16_t mode);
int  fs_read   (int fd, char *buf, uint32_t count);
int  fs_write  (int fd, const char *buf, uint32_t count);
int  fs_pread  (int fd, char *buf, uint32_t count, uint32_t off);
int  fs_pwrite (int fd, const char *buf, uint32_t count, uint32_t off);
int  fs_lseek  (int fd, int32_t offset, int whence);
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
int  fs_truncate(const char *path, uint64_t size);
int  fs_ftruncate(int fd, uint64_t size);
int  fs_getdents64(int fd, void *buf, uint32_t count);
int  fs_fsync  (int fd);
int  fs_sync   (void);
int  fs_mount  (const char *dev, const char *dir, int flags);
int  fs_umount (const char *dir, int flags);
int  fs_statfs (const char *path, void *buf);
int  fs_ioctl  (int fd, uint32_t req, void *arg);
int  fs_mmap   (int fd, uint32_t length, uint32_t prot, uint32_t flags,
                uint32_t offset);

/* ── backend registration ───────────────────────────────────────────── */
/* A backend calls this to install its ops tables.  UNFS registers here. */
int  fs_register_fs(const char *name,
                    const uiox_inode_ops_t *iop,
                    const uiox_file_ops_t  *fop);

#endif /* UIOX_KIX_SCFS_FS_H */
