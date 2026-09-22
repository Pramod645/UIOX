/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_file.h   — v3.0.0
 *
 * UIOX file table + per-process descriptor table + u-area (Bach Ch.7).
 *
 * GAP FIXES APPLIED (#3 table size, #4 concurrency)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_FILE_H
#define UIOX_KIX_SCFS_FILE_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_ops.h"
#include "uiox_klibc.h"

#ifndef NFILE
#define NFILE       4096          /* GAP #3: tunable, was 100 */
#endif
#ifndef NOFILE
#define NOFILE      256           /* GAP #3: tunable, was 20  */
#endif

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
#define O_DIRECT    0x0800
#define O_NONBLOCK  0x1000

#define UIOX_ATIME_ALWAYS   0
#define UIOX_ATIME_RELATIME 1
#define UIOX_ATIME_NOATIME  2

/* GAP #4: per-open-file lock */
typedef struct file_lock {
    int16_t   l_excl;
    int16_t   l_readers;
    void     *l_waitq;
} file_lock_t;

typedef struct file {
    uint16_t               f_flag;
    uint16_t               f_count;
    inode_t               *f_inode;
    uint64_t               f_offset;    /* 64-bit (was uint32) */
    const uiox_file_ops_t *f_op;
    struct file           *f_next;
    file_lock_t            f_lock;      /* GAP #4 */
    uint32_t               f_seq;
} file_t;

typedef struct ufd {
    file_t   *ufd_file[NOFILE];
} ufd_t;

typedef struct u_area {
    ufd_t     u_ofile;
    inode_t  *u_cdir;
    inode_t  *u_rdir;
    uint16_t  u_uid;
    uint16_t  u_gid;
    uint16_t  u_umask;
    uint64_t  u_offset;
    int       u_segflg;
    int       u_error;
    uint8_t   u_atime_policy;
} u_area_t;

extern file_t file_table[NFILE];
extern u_area_t *u_area(void);

file_t *falloc(void);
void    f_close(file_t *fp);
int     ufalloc(void);

#endif /* UIOX_KIX_SCFS_FILE_H */
