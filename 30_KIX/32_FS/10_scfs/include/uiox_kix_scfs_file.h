/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_file.h   — modernised
 *
 * UIOX file table + per-process descriptor table + u-area.
 * Bach Ch.7 layout, with the op-table pointer and the process-state fix.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_FILE_H
#define UIOX_KIX_SCFS_FILE_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_ops.h"
#include "uiox_klibc.h"

#define NFILE       100     /* max open files system-wide */
#define NOFILE      20      /* max open files per process */

/* File open flags (match the O_* values in uiox_vfs.h) */
#define FREAD       0x0001
#define FWRITE      0x0002
#define FAPPEND     0x0004

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

/* File-table entry (Bach Ch.7) */
typedef struct file {
    uint16_t                  f_flag;    /* FREAD / FWRITE / FAPPEND     */
    uint16_t                  f_count;   /* reference count (dup ++)     */
    inode_t                  *f_inode;   /* i-node this entry names      */
    uint32_t                  f_offset;  /* current read/write offset    */
    const uiox_file_ops_t    *f_op;      /* data ops (copied from i_fop) */
    struct file              *f_next;    /* free-list link               */
} file_t;

/* Per-process user file descriptor table */
typedef struct ufd {
    file_t   *ufd_file[NOFILE];          /* open file pointers           */
} ufd_t;

/* u area — PROCESS STATE (Bach Ch.7).  One per process, not a global. */
typedef struct u_area {
    ufd_t     u_ofile;                   /* open file table              */
    inode_t  *u_cdir;                    /* current directory inode      */
    inode_t  *u_rdir;                    /* root directory inode         */
    uint16_t  u_uid;
    uint16_t  u_gid;
    uint16_t  u_umask;
    uint32_t  u_offset;                  /* current file offset          */
    int       u_segflg;                  /* 0 = user, 1 = kernel         */
    int       u_error;                   /* error code                   */
} u_area_t;

/* System-wide file table (Bach NFILE) */
extern file_t file_table[NFILE];

/* Process u-area accessor — supplied by 34_PCS, one per process. */
extern u_area_t *u_area(void);

/* File-table operations (Bach Ch.7) */
file_t *falloc(void);
void    f_close(file_t *fp);
int     ufalloc(void);

#endif /* UIOX_KIX_SCFS_FILE_H */
