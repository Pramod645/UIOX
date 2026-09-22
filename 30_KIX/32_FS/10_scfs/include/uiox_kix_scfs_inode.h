/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_inode.h   — modernised
 *
 * UIOX i-node — Bach Ch.4 layout, plus the two op-table pointers that make
 * the backend pluggable, and the pipe-buffer pointer pipe() needs.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_INODE_H
#define UIOX_KIX_SCFS_INODE_H

#include "uiox_klibc.h"

#define NBLOCK_DIRECT   10
#define NBLOCK_INDIRECT  3
#define MAX_LINKS       32767
#define INODE_FREE      0
#define INODE_USED      1

/* File type flags (Bach 0170000-classic; UIOX_S_* aliases in uiox_vfs.h) */
#define IFMT    0170000   /* type mask          */
#define IFREG   0100000   /* regular file       */
#define IFDIR   0040000   /* directory          */
#define IFBLK   0060000   /* block special      */
#define IFCHR   0020000   /* character special  */
#define IFIFO   0010000   /* named pipe / FIFO  */
#define IFLNK   0120000   /* symbolic link      */

/* Inode flags */
#define ILOCK       0x01    /* inode is locked                     */
#define IUPD        0x02    /* inode has been modified             */
#define IACC        0x04    /* inode access time update needed     */
#define IMOUNT      0x08    /* inode is a mount point              */
#define IWANT       0x10    /* process waiting for inode           */
#define ITEXT       0x20    /* inode is a shared text file         */

#define NINODE      100     /* max inodes in the in-core table     */

/* forward decls — the ops tables and the pipe buffer live in ops.h */
struct uiox_inode_ops;
struct uiox_file_ops;
struct uiox_pipe_buffer;

typedef struct inode {
    uint16_t  i_flag;
    uint16_t  i_count;                      /* reference count (Bach)     */
    uint16_t  i_dev;
    uint32_t  i_number;                     /* i-node number on device    */
    uint16_t  i_mode;                       /* file type + permissions    */
    uint16_t  i_nlink;                      /* number of hard links       */
    uint16_t  i_uid;
    uint16_t  i_gid;
    uint32_t  i_size;                       /* file size in bytes         */
    uint32_t  i_addr[NBLOCK_DIRECT +
                     NBLOCK_INDIRECT];      /* block addresses (Bach)     */
    uint8_t   i_major;
    uint8_t   i_minor;
    time_t    i_atime;
    time_t    i_mtime;
    time_t    i_ctime;

    /* ── modern additions ──────────────────────────────────────────── */
    const struct uiox_inode_ops *i_iop;     /* directory/lifecycle ops    */
    const struct uiox_file_ops  *i_fop;     /* data ops for this i-node   */
    struct uiox_pipe_buffer     *i_pipe;    /* pipe buffer (NULL otherwise)*/
    const char                  *i_path;    /* dentry path, or NULL       */
    int16_t                     i_pipe_readers;
    int16_t                     i_pipe_writers;
} inode_t;

/* In-core inode table (Bach NINODE) */
extern inode_t inode_table[NINODE];

/* Core inode algorithms (Bach Ch.4) */
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
