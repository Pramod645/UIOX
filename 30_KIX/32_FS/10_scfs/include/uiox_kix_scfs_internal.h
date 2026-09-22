/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_internal.h   — v2.0.0
 *
 * Shared prologue for every src/*.c unit, rewritten against the modern
 * inode_t / file_t / ops-table headers.  The old vfs_* extern block is gone;
 * the bridge now runs through the two op tables plus the Bach core calls.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_INTERNAL_H
#define UIOX_KIX_SCFS_INTERNAL_H

#include "uiox_kix_scfs_fs.h"       /* brings inode/file/mount/buf/ops + FS_* */

/* ── errno — one set.  SCFS aliases the FS_* codes. ─────────────────── */
#define SCFS_OK      FS_OK
#define SCFS_ENOENT  FS_ENOENT
#define SCFS_EACCES  FS_EACCES
#define SCFS_EEXIST  FS_EEXIST
#define SCFS_EBADF   FS_EBADF
#define SCFS_ENOTDIR FS_ENOTDIR
#define SCFS_EISDIR  FS_EISDIR
#define SCFS_EPERM   FS_EPERM
#define SCFS_EBUSY   FS_EBUSY
#define SCFS_EROFS   FS_EROFS
#define SCFS_EXDEV   FS_EXDEV
#define SCFS_EINVAL  FS_EINVAL
#define SCFS_ENOSYS  FS_ENOSYS
#define SCFS_EFAULT  FS_EFAULT
#define SCFS_ENOMEM  FS_ENOMEM
#define SCFS_ERANGE  FS_ERANGE

/* ── Bach core calls SCFS uses directly (from inode.h / buf.h) ──────── */
/* iget / iput / ialloc / ifree / namei / iupdate / itrunc / iaccess
 * bread / bwrite / brelse / getblk / balloc / bfree / bmap — declared in
 * their own headers, already included above. */

/* ── modern helpers the SCFS units call ─────────────────────────────── */
/* fd table — the per-process descriptor table lives in the u-area */
extern file_t   *u_fd_get(int fd);          /* u_area()->u_ofile lookup */
extern int       u_fd_alloc(file_t *fp);    /* lowest free fd           */
extern int       u_fd_free(int fd);
extern int       u_fd_dup(int fd);
extern int       u_fd_find_free(void);

/* path resolution — namei() plus the mount-point redirect (Bach Ch.9) */
extern inode_t  *namei_at(inode_t *dir, const char *path);
extern inode_t  *namei_parent(const char *path, char **lastname);

/* mount table */
extern int       vfs_mount_root(const char *fstype);
extern int       vfs_mount_busy(const char *dir);

/* block-device read used by the SCFS-ramfs/UNFS backends */
extern int       bdev_read(uint16_t dev, uint32_t blkno, void *buf);
extern int       bdev_write(uint16_t dev, uint32_t blkno, const void *buf);

/* ── shared fd lookup (every unit uses this) ────────────────────────── */
static inline file_t *scfs_fd(int fd)
{
    return (fd < 0 || fd >= NOFILE) ? (file_t *)0 : u_fd_get(fd);
}

#endif /* UIOX_KIX_SCFS_INTERNAL_H */
