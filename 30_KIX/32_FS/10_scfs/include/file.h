#ifndef FILE_H
#define FILE_H

#include "inode.h"
#include "../../33_PCS/include/uiox_klibc.h"  

#define NFILE       100     /* max open files system-wide */
#define NOFILE      20      /* max open files per process */

/* File open flags */
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0x0040
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

/* File table entry */
typedef struct file {
    uint16_t  f_flag;       /* read/write/append flags */
    uint16_t  f_count;      /* reference count */
    inode_t  *f_inode;      /* pointer to inode */
    uint32_t  f_offset;     /* current read/write offset */
} file_t;

/* Per-process user file descriptor table */
typedef struct ufd {
    file_t   *ufd_file[NOFILE];   /* open file pointers */
} ufd_t;

/* u area (simplified) */
typedef struct u_area {
    ufd_t     u_ofile;            /* open file table */
    inode_t  *u_cdir;             /* current directory inode */
    inode_t  *u_rdir;             /* root directory inode */
    uint16_t  u_uid;              /* user id */
    uint16_t  u_gid;              /* group id */
    uint16_t  u_umask;            /* file creation mask */

    /* I/O parameters set before read/write */
    char     *u_base;             /* user buffer address */
    uint32_t  u_count;            /* byte count for I/O */
    uint32_t  u_offset;           /* current file offset */
    int       u_segflg;           /* 0 = user space, 1 = kernel space */
    int       u_error;            /* error code */
} u_area_t;

extern file_t  file_table[NFILE];
extern u_area_t u;

/* File table operations */
file_t *falloc(void);
void    f_close(file_t *fp);
int     ufalloc(void);

#endif /* FILE_H */
