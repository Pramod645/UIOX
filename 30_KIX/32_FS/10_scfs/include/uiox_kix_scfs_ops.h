/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_ops.h   — v3.0.0
 *
 * UIOX — VFS operation tables (modern ops-table design).
 *
 * GAP FIXES APPLIED (#1 64-bit, #2 groups, #4 locks, #5 ordering,
 *                     #6 TRIM, #8 xattr)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_OPS_H
#define UIOX_KIX_SCFS_OPS_H

#include "uiox_klibc.h"
#include "uiox_kix_scfs_inode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uiox_pipe_buffer uiox_pipe_buffer_t;
typedef struct uiox_xattr_node  uiox_xattr_node_t;

#define UIOX_ORDER_LAZY    0
#define UIOX_ORDER_ORDERED 1
#define UIOX_ORDER_FULL    2

typedef struct uiox_file_ops {
    int  (*read)    (file_t *fp, void *kbuf, uint32_t count);
    int  (*write)   (file_t *fp, const void *kbuf, uint32_t count);
    int  (*pread)   (file_t *fp, void *kbuf, uint32_t count, uint64_t off);
    int  (*pwrite)  (file_t *fp, const void *kbuf, uint32_t count, uint64_t off);
    int  (*seek)    (file_t *fp, int64_t off, int whence);
    int  (*readdir) (file_t *fp, void *dbuf, uint32_t count);
    int  (*ioctl)   (file_t *fp, uint32_t req, void *arg);
    int  (*fsync)   (file_t *fp, int mode);          /* GAP #5 */
    int  (*close)   (file_t *fp);
    int  (*lock)    (file_t *fp, int exclusive);     /* GAP #4 */
    int  (*unlock)  (file_t *fp);                    /* GAP #4 */
} uiox_file_ops_t;

typedef struct uiox_inode_ops {
    inode_t *(*iget)       (uint16_t dev, uint32_t inum);
    void     (*iput)       (inode_t *ip);
    int      (*write_inode)(inode_t *ip, int order); /* GAP #5 */
    int      (*free_inode) (inode_t *ip);

    int  (*lookup)  (inode_t *dir, const char *name, inode_t **out);
    int  (*create)  (inode_t *dir, const char *name, uint16_t mode, inode_t **out);
    int  (*mkdir)   (inode_t *dir, const char *name, uint16_t mode);
    int  (*rmdir)   (inode_t *dir);
    int  (*link)    (inode_t *dir, inode_t *target, const char *name);
    int  (*unlink)  (inode_t *dir, const char *name);
    int  (*mknod)   (inode_t *dir, const char *name, uint16_t mode,
                     uint32_t dev, inode_t **out);
    int  (*symlink) (inode_t *dir, const char *target, const char *name);
    int  (*readlink)(inode_t *ip, char *buf, uint32_t len);
    int  (*rename)  (inode_t *olddir, const char *oldname,
                     inode_t *newdir, const char *newname);

    int  (*truncate)(inode_t *ip, uint64_t size);    /* GAP #1 */
    int  (*fallocate)(inode_t *ip, uint32_t mode, uint64_t off, uint64_t len);

    uint32_t (*alloc_block)(inode_t *ip, uint32_t grp_hint);  /* GAP #2 */
    void     (*free_block) (inode_t *ip, uint32_t blkno);     /* GAP #2 */

    int  (*discard) (inode_t *ip, uint64_t off, uint64_t len); /* GAP #6 */

    void (*ilock)   (inode_t *ip, int exclusive);    /* GAP #4 */
    void (*iunlock) (inode_t *ip);                   /* GAP #4 */

    int  (*setxattr)   (inode_t *ip, const char *name,
                        const void *val, uint32_t len, int flags); /* GAP #8 */
    int  (*getxattr)   (inode_t *ip, const char *name,
                        void *val, uint32_t len);
    int  (*listxattr)  (inode_t *ip, char *list, uint32_t size);
    int  (*removexattr)(inode_t *ip, const char *name);

    const char *fs_name;
} uiox_inode_ops_t;

static inline const uiox_file_ops_t *scfs_fop(const inode_t *ip)
{
    return ip ? ip->i_fop : (const uiox_file_ops_t *)0;
}
static inline const uiox_inode_ops_t *scfs_iop(const inode_t *ip)
{
    return ip ? ip->i_iop : (const uiox_inode_ops_t *)0;
}

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_SCFS_OPS_H */
