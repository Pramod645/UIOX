#ifndef INODE_H
#define INODE_H

#include "../../33_PCS/include/uiox_klibc.h"  

#define NBLOCK_DIRECT   10
#define NBLOCK_INDIRECT  3
#define MAX_LINKS       32767
#define INODE_FREE      0
#define INODE_USED      1

/* File type flags */
#define IFMT    0170000   /* type mask */
#define IFREG   0100000   /* regular file */
#define IFDIR   0040000   /* directory */
#define IFBLK   0060000   /* block special */
#define IFCHR   0020000   /* character special */
#define IFIFO   0010000   /* named pipe / FIFO */

/* Permission bits */
#define ISUID   04000
#define ISGID   02000
#define ISVTX   01000
#define IRWXU   0700
#define IRUSR   0400
#define IWUSR   0200
#define IXUSR   0100
#define IRWXG   0070
#define IRWXO   0007

/* Inode flags */
#define ILOCK       0x01    /* inode is locked */
#define IUPD        0x02    /* inode has been updated */
#define IACC        0x04    /* inode access time updated */
#define IMOUNT      0x08    /* inode is a mount point */
#define IWANT       0x10    /* process waiting for inode */
#define ITEXT       0x20    /* inode is a shared text file */

#define NINODE      100     /* max inodes in inode table */
#define BLOCK_SIZE  512

typedef struct inode {
    uint16_t  i_flag;                       /* state flags */
    uint16_t  i_count;                      /* reference count */
    uint16_t  i_dev;                        /* device where inode lives */
    uint32_t  i_number;                     /* inode number */
    uint16_t  i_mode;                       /* file type + permissions */
    uint16_t  i_nlink;                      /* number of hard links */
    uint16_t  i_uid;                        /* owner user id */
    uint16_t  i_gid;                        /* owner group id */
    uint32_t  i_size;                       /* file size in bytes */
    uint32_t  i_addr[NBLOCK_DIRECT +
                     NBLOCK_INDIRECT];      /* block addresses */
    uint8_t   i_major;                      /* major device number */
    uint8_t   i_minor;                      /* minor device number */
    time_t    i_atime;                      /* last access time */
    time_t    i_mtime;                      /* last modification time */
    time_t    i_ctime;                      /* last inode change time */
    int       i_pipe_readers;               /* pipe: number of readers */
    int       i_pipe_writers;               /* pipe: number of writers */
} inode_t;

/* Inode table (in-memory) */
extern inode_t inode_table[NINODE];

/* Core inode algorithms */
inode_t *iget(uint16_t dev, uint32_t inum);
void     iput(inode_t *ip);
void     ilock(inode_t *ip);
void     iunlock(inode_t *ip);
inode_t *ialloc(uint16_t dev);
void     ifree(uint16_t dev, uint32_t inum);
inode_t *namei(const char *path);
int      iupdate(inode_t *ip);
int      itrunc(inode_t *ip);
int      iaccess(inode_t *ip, int mode);

#endif /* INODE_H */
