/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_inode.h   — v3.0.0
 *
 * UIOX i-node — Bach Ch.4 layout, modernised for 64-bit scale.
 *
 * GAP FIXES APPLIED (#1 4 GB ceiling, #8 xattr):
 *   #1  i_size widened uint32_t -> uint64_t
 *   #8  i_xattr points at an opaque xattr chain (UNFS xattr home)
 *   +   i_generation (stable file handles), i_seq (change counter)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_INODE_H
#define UIOX_KIX_SCFS_INODE_H

#include "uiox_klibc.h"

#define NBLOCK_DIRECT   10
#define NBLOCK_INDIRECT  3
#define MAX_LINKS       32767
#define INODE_FREE      0
#define INODE_USED      1

#define IFMT    0170000
#define IFREG   0100000
#define IFDIR   0040000
#define IFBLK   0060000
#define IFCHR   0020000
#define IFIFO   0010000
#define IFLNK   0120000

#define ILOCK       0x01
#define IUPD        0x02
#define IACC        0x04
#define IMOUNT      0x08
#define IWANT       0x10
#define ITEXT       0x20
#define IFASTSYM    0x40    /* symlink target stored inline in i_addr[]  */
#define IEXTENTS    0x80    /* backend uses extents, not i_addr[]        */

#define NINODE      100

struct uiox_inode_ops; struct uiox_file_ops;
struct uiox_pipe_buffer; struct uiox_xattr_node;

/* 64-bit time, explicitly.  32-bit time_t is GAP #7 (the 2038 problem). */
typedef int64_t uiox_time64_t;

typedef struct inode {
    uint16_t  i_flag;
    uint16_t  i_count;
    uint16_t  i_dev;
    uint32_t  i_number;
    uint16_t  i_mode;
    uint16_t  i_nlink;
    uint16_t  i_uid;
    uint16_t  i_gid;

    uint64_t  i_size;                        /* GAP #1 */
    uint32_t  i_addr[NBLOCK_DIRECT + NBLOCK_INDIRECT];

    uint8_t   i_major;
    uint8_t   i_minor;

    uiox_time64_t i_atime;                   /* GAP #7 */
    uiox_time64_t i_mtime;
    uiox_time64_t i_ctime;
    uiox_time64_t i_btime;

    const struct uiox_inode_ops *i_iop;
    const struct uiox_file_ops  *i_fop;
    struct uiox_pipe_buffer     *i_pipe;
    const char                  *i_path;
    struct uiox_xattr_node      *i_xattr;    /* GAP #8 */

    uint32_t  i_generation;
    uint32_t  i_seq;
    int16_t   i_pipe_readers;
    int16_t   i_pipe_writers;
} inode_t;

extern inode_t inode_table[NINODE];

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

#endif /* UIOX_KIX_SCFS_INODE_H */
