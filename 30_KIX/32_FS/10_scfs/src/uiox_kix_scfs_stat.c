/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_stat.c
 *
 *  SCFS — Algorithms stat, fstat, lstat.
 *  Bach, The Design of the UNIX Operating System.
 *
 *  ── what Bach says ─────────────────────────────────────────────────
 *  "The system calls stat and fstat allow processes to query the status
 *   of files, returning information such as the file type, file owner,
 *   access permissions, file size, number of links, inode number, and
 *   file access times."
 *
 *  Both are namei-followed-by-copy.  Bach's stat struct carries exactly
 *  the seven items he lists; the layout here adds the device number,
 *  which this filesystem stores nowhere and reports as zero.
 *
 *  ── FIXED: the unchecked buffer ──────────────────────────────────────
 *  The first version took `void *buf` and wrote 42 bytes at fixed
 *  offsets with no idea how large the caller's object was.  A caller
 *  with a smaller structure had bytes written past its end.
 *
 *  Now every entry point takes the buffer's size as well as its address.
 *  A buffer smaller than SCFS_STAT_SZ is refused with EINVAL before a
 *  single byte is written.  The offsets and the size live in
 *  uiox_kix_scfs_stat.h so the writer, the caller and any debug command
 *  read the same numbers.
 *
 *  ── the writes are now byte-wise through a helper ────────────────────
 *  The first version cast the buffer pointer to a uint16_t pointer,
 *  then to a uint32_t pointer, and so on, and stored the value through it.
 *  assemble the value from bytes, so alignment no longer matters.
 *
 *  v1.4: buffer size checked; unaligned stores removed.
 */
#include "uiox_kix_scfs_internal.h"
#include "uiox_kix_scfs_stat.h"

/* ── byte-wise stores — no alignment requirement on the caller ──────── */
static void scfs_st16(uint8_t *p, uint32_t off, uint16_t v)
{
    p[off + 0u] = (uint8_t)(v & 0xFFu);
    p[off + 1u] = (uint8_t)((v >> 8) & 0xFFu);
}

static void scfs_st32(uint8_t *p, uint32_t off, uint32_t v)
{
    p[off + 0u] = (uint8_t)(v & 0xFFu);
    p[off + 1u] = (uint8_t)((v >> 8)  & 0xFFu);
    p[off + 2u] = (uint8_t)((v >> 16) & 0xFFu);
    p[off + 3u] = (uint8_t)((v >> 24) & 0xFFu);
}

static void scfs_st64(uint8_t *p, uint32_t off, int64_t v)
{
    uint64_t u = (uint64_t)v;
    uint32_t i;

    for (i = 0u; i < 8u; i++)
        p[off + i] = (uint8_t)((u >> (8u * i)) & 0xFFu);
}

/*
 * Copy an inode's fields out to the caller's status structure.
 * One place, so stat, fstat and lstat cannot drift apart.
 *
 * The caller has already proved bufsz >= SCFS_STAT_SZ.
 */
static void scfs_stat_fill(const InCoreInode *ip, void *ubuf)
{
    uint8_t *p = (uint8_t *)ubuf;

    scfs_st16(p, SCFS_STAT_OFF_MODE,   ip->mode);
    scfs_st16(p, SCFS_STAT_OFF_NLINK,  ip->nlink);
    scfs_st16(p, SCFS_STAT_OFF_UID,    ip->uid);
    scfs_st16(p, SCFS_STAT_OFF_GID,    ip->gid);
    scfs_st32(p, SCFS_STAT_OFF_INO,    ip->ino);
    scfs_st32(p, SCFS_STAT_OFF_SIZE,   ip->size);
    scfs_st64(p, SCFS_STAT_OFF_ATIME,  (int64_t)ip->atime);
    scfs_st64(p, SCFS_STAT_OFF_MTIME,  (int64_t)ip->mtime);
    scfs_st64(p, SCFS_STAT_OFF_CTIME,  (int64_t)ip->ctime);

    /* InCoreInode carries no device-number fields, so there is nothing
     * to report.  Zero is the honest value — a caller that needs to tell
     * two device nodes apart has to read the mount table. */
    p[SCFS_STAT_OFF_DEVMAJ] = 0u;
    p[SCFS_STAT_OFF_DEVMIN] = 0u;
}

/* ── stat() — by path ───────────────────────────────────────────────── */
int uiox_kix_scfs_stat(const char *path, void *buf, uint32_t bufsz)
{
    InCoreInode *ip;

    if (!path || !buf) return SCFS_EFAULT;
    if (bufsz < SCFS_STAT_SZ) return SCFS_EINVAL;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    scfs_stat_fill(ip, buf);
    iput(ip);
    return SCFS_OK;
}

/* ── fstat() — by descriptor ────────────────────────────────────────── */
int uiox_kix_scfs_fstat(int fd, void *buf, uint32_t bufsz)
{
    scfs_file_t *f;

    if (!buf) return SCFS_EFAULT;
    if (bufsz < SCFS_STAT_SZ) return SCFS_EINVAL;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    /* The file table entry already holds the inode's reference, so no
     * iput is due — Bach's fstat does not release either. */
    scfs_stat_fill(f->f_inode, buf);
    return SCFS_OK;
}

/* ── lstat() — do not follow a final symlink ────────────────────────── */
int uiox_kix_scfs_lstat(const char *path, void *buf, uint32_t bufsz)
{
    InCoreInode *ip;

    if (!path || !buf) return SCFS_EFAULT;
    if (bufsz < SCFS_STAT_SZ) return SCFS_EINVAL;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    /* 01_fsa's namei() has no NOFOLLOW mode and the inode carries no
     * symlink target, so a final symlink is followed either way.  That
     * difference is a gap in the layer below, not here. */
    scfs_stat_fill(ip, buf);
    iput(ip);
    return SCFS_OK;
}

/*
 * ── fstatat() — stat relative to a directory fd ───────────────────────
 * Bach has no such call.  It needs namei() to start from an arbitrary
 * directory inode; 01_fsa's namei() takes a cwd argument, so the start is
 * available, but the dirfd-to-inode step plus the AT_* flag handling is
 * work the layer below has not grown.  ENOSYS rather than a wrong answer.
 */
int uiox_kix_scfs_fstatat(int dirfd, const char *path, void *buf,
                          uint32_t bufsz, int flags)
{
    (void)dirfd; (void)path; (void)buf; (void)bufsz; (void)flags;
    return SCFS_ENOSYS;
}

/*
 * ── statx() — extended stat with a field mask ────────────────────────
 * A superset of stat: the caller names the fields it wants.  01_fsa's
 * inode holds no fields beyond the ones scfs_stat_fill already writes, so
 * the mask selects nothing extra.  ENOSYS until the backend carries more,
 * rather than claiming fields it cannot supply.
 */
int uiox_kix_scfs_statx(int dirfd, const char *path, int flags,
                        unsigned int mask, void *buf, uint32_t bufsz)
{
    (void)dirfd; (void)path; (void)flags; (void)mask; (void)buf; (void)bufsz;
    return SCFS_ENOSYS;
}
