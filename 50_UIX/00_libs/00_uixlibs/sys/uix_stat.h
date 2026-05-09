#ifndef __SYS_UIX_STAT__H
#define __SYS_UIX_STAT__H
/*
sys/stat.h in simplified form.

*/
/* This is for only POXIS */

#include "uix_features.h" //?

#include "uix_types.h"

#define UIX_S_IFMT   0170000
#define UIX_S_IFSOCK 0140000
#define UIX_S_IFLNK  0120000   //Symbolic link type bit
#define UIX_S_IFREG  0100000   // Regular file type bit
#define UIX_S_IFBLK  0060000   // Block device
#define UIX_S_IFDIR  0040000  // Directory type bit
#define UIX_S_IFCHR  0020000  // Character device
#define UIX_S_IFIFO  0010000  // FIFO/named pipe

#define UIX_S_ISREG(m)  (((m)&UIX_S_IFMT)==UIX_S_IFREG)  // Tests if mode is regular file
#define UIX_S_ISDIR(m)  (((m)&UIX_S_IFMT)==UIX_S_IFDIR)   // Tests if mode is directory
#define UIX_S_ISCHR(m)  (((m)&UIX_S_IFMT)==UIX_S_IFCHR)
#define UIX_S_ISBLK(m)  (((m)&UIX_S_IFMT)==UIX_S_IFBLK)
#define UIX_S_ISFIFO(m) (((m)&UIX_S_IFMT)==UIX_S_IFIFO)
#define UIX_S_ISLNK(m)  (((m)&UIX_S_IFMT)==UIX_S_IFLNK)
#define UIX_S_ISSOCK(m) (((m)&UIX_S_IFMT)==UIX_S_IFSOCK)

#define UIX_S_ISUID 04000
#define UIX_S_ISGID 02000
#define UIX_S_ISVTX 01000
#define UIX_S_IRWXU 00700
#define UIX_S_IRUSR 00400   // User read permission
#define UIX_S_IWUSR 00200   // User write permission
#define UIX_S_IXUSR 00100   // User execute permission
#define UIX_S_IRWXG 00070
#define UIX_S_IRGRP 00040
#define UIX_S_IWGRP 00020
#define UIX_S_IXGRP 00010
#define UIX_S_IRWXO 00007
#define UIX_S_IROTH 00004
#define UIX_S_IWOTH 00002
#define UIX_S_IXOTH 00001

typedef struct uix_stat {
    uix_dev_t     st_dev;   // Device ID containing file
    uix_ino_t     st_ino;   // Inode number
    uix_mode_t    st_mode;   // File type and permissions
    uix_nlink_t   st_nlink;  // Number of hard links
    uix_uid_t     st_uid;
    uix_gid_t     st_gid;
    uix_dev_t     st_rdev;
    uix_off_t     st_size;  // File size in bytes
    uix_time_t    st_atime;
    uix_time_t    st_mtime;
    uix_time_t    st_ctime;
    uix_blksize_t st_blksize;
    uix_blkcnt_t  st_blocks;
} uix_stat_t;

int        uix_stat  (const char *path, uix_stat_t *buf);  // Gets file info, follows symlinks
int        uix_fstat (int fd, uix_stat_t *buf);  // Gets file info by fd
int        uix_lstat (const char *path, uix_stat_t *buf);  // Gets file info, does NOT follow symlinks
int        uix_chmod (const char *path, uix_mode_t mode);  // Changes file permissions
int        uix_fchmod(int fd, uix_mode_t mode);     
int        uix_chown (const char *path, uix_uid_t owner, uix_gid_t group);  // Changes file owner and group
int        uix_fchown(int fd, uix_uid_t owner, uix_gid_t group);
uix_mode_t uix_umask (uix_mode_t mask); // Sets file creation mode mask
int        uix_mkdir (const char *path, uix_mode_t mode);  // Creates directory
int        uix_mknod (const char *path, uix_mode_t mode, uix_dev_t dev);  // Creates special file



#endif /* End of __SYS_UIX_STAT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
