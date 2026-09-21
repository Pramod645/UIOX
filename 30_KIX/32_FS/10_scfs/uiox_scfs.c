/*
 * 30_KIX/32_FS/10_scfs/scfs.c  — v1.1.0
 *
 * UIOX — SCFS: the syscall-facing filesystem layer.
 *
 * SCFS owns NO storage logic.  Each entry below:
 *   1. validates user pointers / copies args in
 *   2. resolves an fd (file table) or a path (namei)
 *   3. dispatches into uiox_file_ops_t / uiox_inode_ops_t
 *   4. marshals the result back out and returns an errno-style value
 *
 * Anything whose backend is not yet linked returns -ENOSYS rather than a
 * NULL function pointer — the previous defect (`.seek = (void*)0`).
 *
 * Layering:
 *   arch trap  -> scfs_dispatch() -> VFS (01_fsa) -> backend (UNFS)
 *
 * @version 1.1.0  @date 2026-09-20
 */
#include "uiox_scfs.h"
#include "uiox_vfs.h"
#include "uiox_soc_stdio.h"

/* =====================================================================
 * Minimal errno set — no <errno.h> under -ffreestanding.
 * ===================================================================== */
#define SCFS_OK        0
#define SCFS_EBADF     9
#define SCFS_EACCES   13
#define SCFS_EFAULT   14
#define SCFS_ENOENT    2
#define SCFS_ENOSYS   38
#define SCFS_EINVAL   22
#define SCFS_ENOTDIR  20
#define SCFS_EROFS    30

/* =====================================================================
 * Forward declarations — one per entry.
 * ===================================================================== */
static long sc_open   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_close  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_read   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_write  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_lseek  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_pread  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_pwrite (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_readv  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_writev (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_stat   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_fstat  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_lstat  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_fstatat(uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_chmod  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_fchmod (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_chown  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_truncate(uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_ftruncate(uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_access (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_umask  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_mkdir  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_rmdir  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_chdir  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_getcwd (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_getdents64(uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_link   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_unlink (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_rename (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_fsync  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_sync   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_mount  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_statfs (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_dup    (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_fcntl  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_ioctl  (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_mmap   (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);
static long sc_enosys (uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t, uiox_reg_t);

/* =====================================================================
 * The table.
 *
 * dispatch  — which VFS object the entry calls
 * state     — SCFS_READY    : wired to a live op
 *             SCFS_PARTIAL  : syscall present, backend may return ENOSYS
 *             SCFS_PENDING  : waits on UNFS registration / page cache
 * ===================================================================== */
static const scfs_syscall_t s_scfs_table[] = {

/* nr                      name            dispatch             state          handler   */
{ SCFS_NR_read,            "read",         SCFS_DISPATCH_FILE,  SCFS_READY,   sc_read    },
{ SCFS_NR_write,           "write",        SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_write   },
{ SCFS_NR_open,            "open",         SCFS_DISPATCH_VFS,   SCFS_READY,   sc_open    },
{ SCFS_NR_close,           "close",        SCFS_DISPATCH_FILE,  SCFS_READY,   sc_close   },
{ SCFS_NR_lseek,           "lseek",        SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_lseek   },
{ SCFS_NR_pread,           "pread64",      SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_pread   },
{ SCFS_NR_pwrite,          "pwrite64",     SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_pwrite  },
{ SCFS_NR_readv,           "readv",        SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_readv   },
{ SCFS_NR_writev,          "writev",       SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_writev  },

{ SCFS_NR_stat,            "stat",         SCFS_DISPATCH_INODE, SCFS_READY,   sc_stat    },
{ SCFS_NR_fstat,           "fstat",        SCFS_DISPATCH_INODE, SCFS_READY,   sc_fstat   },
{ SCFS_NR_lstat,           "lstat",        SCFS_DISPATCH_INODE, SCFS_PENDING, sc_lstat   },
{ SCFS_NR_newfstatat,      "newfstatat",   SCFS_DISPATCH_INODE, SCFS_PENDING, sc_fstatat },
{ SCFS_NR_chmod,           "chmod",        SCFS_DISPATCH_INODE, SCFS_PENDING, sc_chmod   },
{ SCFS_NR_fchmod,          "fchmod",       SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_fchmod  },
{ SCFS_NR_chown,           "chown",        SCFS_DISPATCH_INODE, SCFS_PENDING, sc_chown   },
{ SCFS_NR_truncate,        "truncate",     SCFS_DISPATCH_INODE, SCFS_PENDING, sc_truncate},
{ SCFS_NR_ftruncate,       "ftruncate",    SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_ftruncate},
{ SCFS_NR_access,          "access",       SCFS_DISPATCH_INODE, SCFS_PARTIAL, sc_access  },
{ SCFS_NR_umask,           "umask",        SCFS_DISPATCH_PCS,   SCFS_READY,   sc_umask   },

{ SCFS_NR_mkdir,           "mkdir",        SCFS_DISPATCH_INODE, SCFS_PENDING, sc_mkdir   },
{ SCFS_NR_rmdir,           "rmdir",        SCFS_DISPATCH_INODE, SCFS_PENDING, sc_rmdir   },
{ SCFS_NR_chdir,           "chdir",        SCFS_DISPATCH_PCS,   SCFS_READY,   sc_chdir   },
{ SCFS_NR_fchdir,          "fchdir",       SCFS_DISPATCH_PCS,   SCFS_READY,   sc_chdir   },
{ SCFS_NR_getcwd,          "getcwd",       SCFS_DISPATCH_PCS,   SCFS_READY,   sc_getcwd  },
{ SCFS_NR_getdents64,      "getdents64",   SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_getdents64},
{ SCFS_NR_link,            "link",         SCFS_DISPATCH_INODE, SCFS_PARTIAL, sc_link    },
{ SCFS_NR_unlink,          "unlink",       SCFS_DISPATCH_INODE, SCFS_PENDING, sc_unlink  },
{ SCFS_NR_rename,          "rename",       SCFS_DISPATCH_INODE, SCFS_PENDING, sc_rename  },

{ SCFS_NR_fsync,           "fsync",        SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_fsync   },
{ SCFS_NR_fdatasync,       "fdatasync",    SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_fsync   },
{ SCFS_NR_sync,            "sync",         SCFS_DISPATCH_VFS,   SCFS_PENDING, sc_sync    },

{ SCFS_NR_mount,           "mount",        SCFS_DISPATCH_VFS,   SCFS_READY,   sc_mount   },
{ SCFS_NR_umount2,         "umount2",      SCFS_DISPATCH_VFS,   SCFS_PENDING, sc_mount   },
{ SCFS_NR_statfs,          "statfs",       SCFS_DISPATCH_VFS,   SCFS_PARTIAL, sc_statfs  },
{ SCFS_NR_fstatfs,         "fstatfs",      SCFS_DISPATCH_VFS,   SCFS_PARTIAL, sc_statfs  },

{ SCFS_NR_dup,             "dup",          SCFS_DISPATCH_VFS,   SCFS_READY,   sc_dup     },
{ SCFS_NR_dup3,            "dup3",         SCFS_DISPATCH_VFS,   SCFS_READY,   sc_dup     },
{ SCFS_NR_fcntl,           "fcntl",        SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_fcntl   },
{ SCFS_NR_ioctl,           "ioctl",        SCFS_DISPATCH_FILE,  SCFS_PARTIAL, sc_ioctl   },

{ SCFS_NR_mmap,            "mmap",         SCFS_DISPATCH_FILE,  SCFS_PENDING, sc_mmap    },

/* Surface present, backend absent — return ENOSYS rather than fault. */
{ SCFS_NR_symlink,         "symlink",      SCFS_DISPATCH_ENOSYS, SCFS_PARTIAL, sc_enosys },
{ SCFS_NR_readlink,        "readlink",     SCFS_DISPATCH_ENOSYS, SCFS_PARTIAL, sc_enosys },
{ SCFS_NR_statx,           "statx",        SCFS_DISPATCH_ENOSYS, SCFS_PARTIAL, sc_enosys },
};

#define SCFS_TABLE_N (sizeof(s_scfs_table) / sizeof(s_scfs_table[0]))

const scfs_syscall_t *scfs_table(void)      { return s_scfs_table; }
uint32_t              scfs_table_size(void) { return (uint32_t)SCFS_TABLE_N; }

/* =====================================================================
 * Dispatch — binary/linear lookup then call.
 * ===================================================================== */
long scfs_dispatch(uint32_t nr,
                   uiox_reg_t a0, uiox_reg_t a1, uiox_reg_t a2,
                   uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    for (uint32_t i = 0u; i < SCFS_TABLE_N; i++) {
        if (s_scfs_table[i].nr == nr) {
            if (s_scfs_table[i].handler == (scfs_handler_t)0)
                return -SCFS_ENOSYS;
            return s_scfs_table[i].handler(a0, a1, a2, a3, a4, a5);
        }
    }
    return -SCFS_ENOSYS;
}

/* =====================================================================
 * Shared helpers
 *
 * fd lookup and path resolution go through the VFS, never through a
 * static inode array (the previous defect).
 * ===================================================================== */
extern uiox_file_t *vfs_fd_get(uint32_t fd);          /* 01_fsa */
extern int          vfs_path_lookup(const char *path, uiox_inode_t **out, uint32_t flags);

static long scfs_fd_file(uiox_reg_t fd, uiox_file_t **out)
{
    if (!out) return -SCFS_EINVAL;
    *out = vfs_fd_get((uint32_t)fd);
    return (*out) ? SCFS_OK : -SCFS_EBADF;
}

/* =====================================================================
 * Implementations
 *
 * Each is a thin dispatcher.  Where the VFS op slot is unset, return
 * -ENOSYS — the fix for `.seek = (void*)0` and friends.
 * ===================================================================== */

static long sc_read(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->read) return -SCFS_ENOSYS;
    return (long)f->f_op->read(f, (void *)ubuf, (size_t)count, &f->f_pos);
}

static long sc_write(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->write) return -SCFS_ENOSYS;
    return (long)f->f_op->write(f, (const void *)ubuf, (size_t)count, &f->f_pos);
}

static long sc_lseek(uiox_reg_t fd, uiox_reg_t off, uiox_reg_t whence, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    /* Prefer the backend op; else compute on f_pos when the FS is seekable. */
    if (f->f_op && f->f_op->seek)
        return (long)f->f_op->seek(f, (int64_t)off, (uint32_t)whence);
    if (!f->f_inode || !f->f_inode->i_op) return -SCFS_ENOSYS;
    return (long)vfs_seek_fallback(f, (int64_t)off, (uint32_t)whence);
}

static long sc_open(uiox_reg_t uptr, uiox_reg_t flags, uiox_reg_t mode, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    if (uptr == 0u) return -SCFS_EFAULT;
    const char *path = (const char *)uptr;   /* uiox_copy_from_user in prod */
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup(path, &inode, (uint32_t)flags);
    if (rc < 0) return rc;
    int fd = vfs_fd_alloc(inode, (uint32_t)flags, (uint32_t)mode);
    return (fd < 0) ? (long)fd : (long)fd;
}

static long sc_close(uiox_reg_t fd, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (f->f_op && f->f_op->close) f->f_op->close(f);
    return vfs_fd_free((uint32_t)fd);
}

static long sc_stat(uiox_reg_t uptr, uiox_reg_t ubuf, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    if (!uptr || !ubuf) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uptr, &inode, 0u);
    if (rc < 0) return rc;
    return vfs_fill_stat(inode, (void *)ubuf);
}

static long sc_fstat(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    return vfs_fill_stat(f->f_inode, (void *)ubuf);
}

static long sc_lstat(uiox_reg_t uptr, uiox_reg_t ubuf, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    /* No symlink support until the backend provides one. */
    return sc_stat(uptr, ubuf, 0, 0, 0, 0);
}

static long sc_fstatat(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_chmod(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_fchmod(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_chown(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_truncate(uiox_reg_t uid, uiox_reg_t len, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    if (!uid) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uid, &inode, 0u);
    if (rc < 0) return rc;
    if (!inode->i_op || !inode->i_op->truncate) return -SCFS_ENOSYS;
    return inode->i_op->truncate(inode, (uint64_t)len);
}

static long sc_ftruncate(uiox_reg_t fd, uiox_reg_t len, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_inode->i_op || !f->f_inode->i_op->truncate) return -SCFS_ENOSYS;
    return f->f_inode->i_op->truncate(f->f_inode, (uint64_t)len);
}

static long sc_access(uiox_reg_t uid, uiox_reg_t mode, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    if (!uid) return -SCFS_EFAULT;
    uiox_inode_t *inode = (uiox_inode_t *)0;
    long rc = vfs_path_lookup((const char *)uid, &inode, 0u);
    if (rc < 0) return rc;
    return vfs_permission_ok(inode, (uint32_t)mode) ? SCFS_OK : -SCFS_EACCES;
}

static long sc_umask(uiox_reg_t mask, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    return vfs_umask_set((uint32_t)mask);   /* 33_PCS owns the process mask */
}

static long sc_mkdir(uiox_reg_t uid, uiox_reg_t mode, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)mode; (void)x; (void)y; (void)z; (void)w;
    if (!uid) return -SCFS_EFAULT;
    return vfs_mkdir((const char *)uid, (uint32_t)mode);
}

static long sc_rmdir(uiox_reg_t uid, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    if (!uid) return -SCFS_EFAULT;
    return vfs_rmdir((const char *)uid);
}

static long sc_chdir(uiox_reg_t uid, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    if (!uid) return -SCFS_EFAULT;
    return pcs_setcwd((const char *)uid);   /* 33_PCS owns cwd */
}

static long sc_getcwd(uiox_reg_t ubuf, uiox_reg_t size, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    if (!ubuf || size == 0u) return -SCFS_EINVAL;
    return pcs_getcwd((char *)ubuf, (uint32_t)size);
}

static long sc_getdents64(uiox_reg_t fd, uiox_reg_t ubuf, uiox_reg_t count, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->readdir) return -SCFS_ENOSYS;
    return (long)f->f_op->readdir(f, (void *)ubuf, (size_t)count);
}

static long sc_link(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_unlink(uiox_reg_t uid, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    if (!uid) return -SCFS_EFAULT;
    return vfs_unlink((const char *)uid);
}

static long sc_rename(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

static long sc_fsync(uiox_reg_t fd, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{
    (void)x; (void)y; (void)z; (void)w; (void)v;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (f->f_op && f->f_op->fsync) { f->f_op->fsync(f); return SCFS_OK; }
    return vfs_sync_inode(f->f_inode);   /* falls through to journal once UNFS lands */
}

static long sc_sync(uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v, uiox_reg_t u)
{ (void)x;(void)y;(void)z;(void)w;(void)v;(void)u; return vfs_sync_all(); }

static long sc_mount(uiox_reg_t src, uiox_reg_t tgt, uiox_reg_t fstype, uiox_reg_t flags, uiox_reg_t data, uiox_reg_t u)
{
    (void)src; (void)flags; (void)data; (void)u;
    if (!tgt || !fstype) return -SCFS_EINVAL;
    return vfs_mount((const char *)tgt, (const char *)fstype);
}

static long sc_statfs(uiox_reg_t uid, uiox_reg_t ubuf, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w)
{
    (void)x; (void)y; (void)z; (void)w;
    return vfs_statfs((const char *)uid, (void *)ubuf);
}

static long sc_dup(uiox_reg_t fd, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z, uiox_reg_t w, uiox_reg_t v)
{ (void)x;(void)y;(void)z;(void)w;(void)v; return vfs_fd_dup((uint32_t)fd); }

static long sc_fcntl(uiox_reg_t fd, uiox_reg_t cmd, uiox_reg_t arg, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{ (void)fd;(void)cmd;(void)arg;(void)x;(void)y;(void)z; return -SCFS_ENOSYS; }

static long sc_ioctl(uiox_reg_t fd, uiox_reg_t req, uiox_reg_t arg, uiox_reg_t x, uiox_reg_t y, uiox_reg_t z)
{
    (void)x; (void)y; (void)z;
    uiox_file_t *f;
    long rc = scfs_fd_file(fd, &f);
    if (rc < 0) return rc;
    if (!f->f_op || !f->f_op->ioctl) return -SCFS_ENOSYS;
    return (long)f->f_op->ioctl(f, (uint32_t)req, (void *)arg);
}

static long sc_mmap(uiox_reg_t fd, uiox_reg_t uaddr, uiox_reg_t prot, uiox_reg_t flags, uiox_reg_t off, uiox_reg_t len)
{ (void)uaddr;(void)prot;(void)flags;(void)off;(void)len; (void)fd; return -SCFS_ENOSYS; }

static long sc_enosys(uiox_reg_t a, uiox_reg_t b, uiox_reg_t c, uiox_reg_t d, uiox_reg_t e, uiox_reg_t f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return -SCFS_ENOSYS; }

/* =====================================================================
 * Registration — hand the table to the arch syscall dispatcher.
 * ===================================================================== */
extern void arch_syscall_install(const scfs_syscall_t *table, uint32_t n);

void scfs_register(void)
{
    arch_syscall_install(s_scfs_table, (uint32_t)SCFS_TABLE_N);
    early_puts("[scfs] syscall table installed\r\n");
}

/* SCFS's own fs_type registration with the VFS (filesystem identity). */
void scfs_init(void)
{
    vfs_register_fs("scfs", (const uiox_file_ops_t *)0, (const uiox_inode_ops_t *)0);
    early_puts("[scfs] registered fs_type\r\n");
}
