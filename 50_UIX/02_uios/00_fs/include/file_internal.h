#ifndef UIOX_FILE_INTERNAL_H
#define UIOX_FILE_INTERNAL_H

/*
 * include/file_internal.h
 *
 * Internal data structures for file I/O implementation.
 * Represents the kernel's three-table structure:
 *   1. Process table (per-process fd table)
 *   2. File table (system-wide open file table)
 *   3. V-node/inode table
 */

#include "file_io.h"

/* =============================================================
 * File system constants
 * ============================================================= */
#define FILE_TABLE_SIZE     512     /* global file table entries   */
#define VNODE_CACHE_SIZE    256     /* in-memory vnode cache       */
#define PATH_MAX            4096    /* maximum pathname length     */
#define NAME_MAX            255     /* maximum filename length     */
#define FILE_MODE           0644    /* default file permissions    */

/* =============================================================
 * File types (from stat.h mode field)
 * ============================================================= */
#define S_IFMT              0170000 /* file type mask              */
#define S_IFREG             0100000 /* regular file                */
#define S_IFDIR             0040000 /* directory                   */
#define S_IFCHR             0020000 /* character special           */
#define S_IFBLK             0060000 /* block special               */
#define S_IFIFO             0010000 /* FIFO                        */
#define S_IFLNK             0120000 /* symbolic link               */
#define S_IFSOCK            0140000 /* socket                      */

#define S_ISREG(m)          (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)          (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)          (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)          (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)         (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)          (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)         (((m) & S_IFMT) == S_IFSOCK)   

/* =============================================================
 * Permission bits
 * ============================================================= */
#define S_IRWXU             0000700 /* owner: read, write, execute */
#define S_IRUSR             0000400 /* owner: read permission      */
#define S_IWUSR             0000200 /* owner: write permission     */
#define S_IXUSR             0000100 /* owner: execute permission   */
#define S_IRWXG             0000070 /* group: read, write, execute */
#define S_IRGRP             0000040 /* group: read permission      */
#define S_IWGRP             0000020 /* group: write permission     */
#define S_IXGRP             0000010 /* group: execute permission   */
#define S_IRWXO             0000007 /* other: read, write, execute */
#define S_IROTH             0000004 /* other: read permission      */
#define S_IWOTH             0000002 /* other: write permission     */
#define S_IXOTH             0000001 /* other: execute permission   */

/* =============================================================
 * Simulated disk storage and inode structure
 * ============================================================= */
#define BLOCK_SIZE          4096
#define MAX_BLOCKS          1024
#define MAX_INODES          256
#define DIRECT_BLOCKS       12
#define INDIRECT_BLOCKS     1
#define DBL_INDIRECT        1

typedef struct disk_inode {
    uint16_t    di_mode;            /* file mode and type          */
    uint16_t    di_nlink;           /* number of links             */
    uint16_t    di_uid;             /* owner user ID               */
    uint16_t    di_gid;             /* owner group ID              */
    uint32_t    di_size;            /* file size in bytes          */
    uint32_t    di_blocks;          /* blocks allocated            */
    time_t      di_atime;           /* access time                 */
    time_t      di_mtime;           /* modify time                 */
    time_t      di_ctime;           /* change time                 */
    uint32_t    di_addr[DIRECT_BLOCKS + INDIRECT_BLOCKS + DBL_INDIRECT];
} disk_inode_t;

/* =============================================================
 * Global tables
 * ============================================================= */
extern file_entry_t     file_table[FILE_TABLE_SIZE];
extern vnode_t          vnode_cache[VNODE_CACHE_SIZE];
extern disk_inode_t     inode_table[MAX_INODES];
extern uint8_t          disk_blocks[MAX_BLOCKS][BLOCK_SIZE];

/* =============================================================
 * Internal function prototypes
 * ============================================================= */

/* File table management */
file_entry_t *file_table_get(int index);
int file_table_find_free(void);

/* Vnode cache management */
vnode_t *vnode_get(uint32_t ino);
vnode_t *vnode_alloc(void);
void vnode_free(vnode_t *vp);

/* Path resolution */
vnode_t *path_lookup(const char *path, proc_fd_table_t *pft);
vnode_t *path_walk(const char *path, vnode_t *start_dir);

/* Block I/O */
int block_read(uint32_t blkno, void *buf);
int block_write(uint32_t blkno, const void *buf);

/* Inode I/O */
int inode_read(uint32_t ino, disk_inode_t *dip);
int inode_write(uint32_t ino, const disk_inode_t *dip);

/* File data I/O */
ssize_t file_data_read(vnode_t *vp, void *buf, size_t count, off_t offset);
ssize_t file_data_write(vnode_t *vp, const void *buf, size_t count, off_t offset);

/* Validation */
bool valid_fd(int fd, proc_fd_table_t *pft);
bool valid_flags(int oflag);
bool valid_mode(mode_t mode);

#endif /* UIOX_FILE_INTERNAL_H */
