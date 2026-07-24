#ifndef FS_H
#define FS_H

#include "inode.h"
#include "file.h"
#include "mount.h"
#include "buf.h"
#include "../../33_PCS/include/uiox_klibc.h"  

/* Stat structure */
typedef struct stat {
    uint16_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_size;
    time_t   st_atime;
    time_t   st_mtime;
    time_t   st_ctime;
    uint8_t  st_major;
    uint8_t  st_minor;
} stat_t;

/* Directory entry */
#define MAXNAMLEN   14
typedef struct dirent {
    uint32_t d_ino;
    char     d_name[MAXNAMLEN];
} dirent_t;

/* System call prototypes */
int  fs_open   (const char *path, int flags, uint16_t mode);
int  fs_read   (int fd, char *buf, uint32_t count);
int  fs_write  (int fd, const char *buf, uint32_t count);
int  fs_lseek  (int fd, int32_t offset, int whence);
int  fs_close  (int fd);
int  fs_creat  (const char *path, uint16_t mode);
int  fs_mknod  (const char *path, uint16_t mode,
                uint8_t major, uint8_t minor);
int  fs_chdir  (const char *path);
int  fs_chroot (const char *path);
int  fs_chown  (const char *path, uint16_t uid, uint16_t gid);
int  fs_chmod  (const char *path, uint16_t mode);
int  fs_stat   (const char *path, stat_t *st);
int  fs_fstat  (int fd, stat_t *st);
int  fs_pipe   (int fd[2]);
int  fs_dup    (int fd);
int  fs_link   (const char *oldpath, const char *newpath);
int  fs_unlink (const char *path);
int  fs_mount  (const char *special, const char *dir, int flags);
int  fs_umount (const char *special);

/* Error codes */
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

/* Seek whence values */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif /* FS_H */
