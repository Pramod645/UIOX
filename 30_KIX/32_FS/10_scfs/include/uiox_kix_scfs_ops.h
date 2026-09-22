/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_ops.h
 *
 * UIOX — VFS operation tables (modern ops-table design).
 *
 * Bach's v7 filesystem dispatched on i_mode from inside fs_* functions,
 * because it had exactly one filesystem compiled in.  UIOX supports
 * pluggable backends (UNFS today, SCFS-ramfs and netfs later), so the
 * dispatch is carried by two operation tables instead:
 *
 *   uiox_inode_ops_t — directory mutations and i-node lifecycle
 *   uiox_file_ops_t  — per-open-file data operations
 *
 * A backend fills these structs and registers them at mount time.  SCFS
 * calls through the tables and never knows which backend is underneath.
 *
 *   arch trap -> scfs_dispatch() -> file_ops / inode_ops -> backend -> buf.h
 *
 * @version 1.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_OPS_H
#define UIOX_KIX_SCFS_OPS_H

#include "uiox_klibc.h"
#include "uiox_kix_scfs_inode.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque pipe buffer — owned by the backend, hung off inode_t::i_pipe. */
typedef struct uiox_pipe_buffer uiox_pipe_buffer_t;

/* ── file operations — one table per open file ──────────────────────── */
typedef struct uiox_file_ops {
    /* data path: returns bytes transferred, or a negative FS_E* code.
     * The offset is advanced in place (Bach u.u_offset). */
    int  (*read)    (file_t *fp, void *kbuf, uint32_t count);
    int  (*write)   (file_t *fp, const void *kbuf, uint32_t count);

    /* positioned variants must NOT disturb fp->f_offset */
    int  (*pread)   (file_t *fp, void *kbuf, uint32_t count, uint32_t off);
    int  (*pwrite)  (file_t *fp, const void *kbuf, uint32_t count, uint32_t off);

    int  (*seek)    (file_t *fp, int32_t off, int whence);
    int  (*readdir) (file_t *fp, void *dbuf, uint32_t count);
    int  (*ioctl)   (file_t *fp, uint32_t req, void *arg);
    int  (*fsync)   (file_t *fp);
    int  (*close)   (file_t *fp);   /* last close; iput() is inode_ops */
} uiox_file_ops_t;

/* ── inode operations — one table per filesystem type ───────────────── */
typedef struct uiox_inode_ops {
    /* lifecycle (Bach iget / iput / iupdate) */
    inode_t *(*iget)       (uint16_t dev, uint32_t inum);
    void     (*iput)       (inode_t *ip);
    int      (*write_inode)(inode_t *ip);   /* iupdate — flush i-node    */
    int      (*free_inode) (inode_t *ip);   /* ifree — links == 0        */

    /* namespace mutation (Bach namei helpers / dir_add / dir_remove) */
    int  (*lookup)  (inode_t *dir, const char *name, inode_t **out);
    int  (*create)  (inode_t *dir, const char *name, uint16_t mode,
                     inode_t **out);
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

    /* data (Bach itrunc / bmap_alloc) */
    int  (*truncate)(inode_t *ip, uint32_t size);
    int  (*fallocate)(inode_t *ip, uint32_t mode, uint32_t off, uint32_t len);

    /* filesystem registration */
    const char *fs_name;
} uiox_inode_ops_t;

/* ── which ops does an i-node use? ──────────────────────────────────── */
/* i_fop / i_iop are set by iget() from the filesystem's registration. */
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
