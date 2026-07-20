#ifndef MOUNT_H
#define MOUNT_H

#include "inode.h"
#include "buf.h"
#include <stdint.h>

#define NMOUNT  20

/* Super block structure */
typedef struct super_block {
    uint32_t s_isize;       /* size of inode list */
    uint32_t s_fsize;       /* size of file system */
    uint16_t s_nfree;       /* number of free blocks */
    uint32_t s_free[50];    /* free block list */
    uint16_t s_ninode;      /* number of free inodes */
    uint32_t s_inode[100];  /* free inode list */
    uint8_t  s_flock;       /* free list lock */
    uint8_t  s_ilock;       /* inode list lock */
    uint8_t  s_fmod;        /* super block modified flag */
    uint8_t  s_ronly;       /* read-only flag */
    time_t   s_time;        /* last update time */
} super_block_t;

/* Mount table entry */
typedef struct mount {
    uint16_t      m_dev;        /* device number */
    buf_t        *m_bufp;       /* buffer with super block */
    inode_t      *m_inodp;      /* inode of mounted-on directory */
    inode_t      *m_mount_root; /* root inode of mounted fs */
    super_block_t m_sb;         /* super block */
    int           m_flags;      /* mount flags */
} mount_t;

#define MNT_RDONLY  0x01

extern mount_t mount_table[NMOUNT];

mount_t *getmount(uint16_t dev);
mount_t *mount_alloc(void);
void     mount_free(mount_t *mp);

#endif /* MOUNT_H */
