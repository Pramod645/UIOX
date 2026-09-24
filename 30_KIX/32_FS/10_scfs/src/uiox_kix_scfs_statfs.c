/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_statfs.c
 *
 *  SCFS — statfs, fstatfs, fstype.
 *  Bach, The Design of the UNIX Operating System.
 *
 *  ── where this sits relative to Bach ─────────────────────────────────
 *  Bach keeps the filesystem's accounting in the SUPER BLOCK — Ch.4 §1.1:
 *
 *    "s_isize, s_fsize   the number of blocks in the inode list and the
 *                         number of blocks in the file system"
 *    "s_nfree            the number of free blocks in the free list"
 *    "s_ninode           the number of free inodes in the cached list"
 *
 *  Everything statfs reports is read out of those fields.  No scan, no
 *  search: a status call on a mounted filesystem is a copy out of the
 *  super block.
 *
 *  ── the field names, corrected ──────────────────────────────────────
 *  The first version read Bach's names off a struct that does not use
 *  them.  The real SuperBlock has fs_size, free_block_count, max_inodes,
 *  free_inode_count — and NO device field, so the f_fsid that version
 *  reported was invented and is now dropped.
 *
 *  Bach's names survive as the named accessors in 01_fsa/sb_access.c, so
 *  the correspondence is visible in one file instead of guessed per call.
 *
 *  ── FIXED: the unchecked buffer ─────────────────────────────────────
 *  The first version took `void *buf` and wrote 64 bytes at fixed offsets
 *  with no idea how large the caller's object was.  Every entry point now
 *  takes the buffer's size and refuses anything smaller than
 *  SCFS_STATFS_SZ before writing a byte.  The size and offsets live in
 *  uiox_kix_scfs_stat.h.
 *
 *  The stores are byte-wise, so the caller's buffer needs no alignment.
 *
 *  v1.4: buffer size checked; field names from the real SuperBlock;
 *        f_fsid dropped; unaligned stores removed.
 */
#include "uiox_kix_scfs_internal.h"
#include "uiox_kix_scfs_stat.h"

/* The 01_fsa accessors (sb_access.c). */
uint32_t sb_total_blocks(void);
uint32_t sb_free_blocks (void);
uint32_t sb_total_inodes(void);
uint32_t sb_free_inodes (void);
uint32_t sb_block_size  (void);
int      sb_is_modified (void);

/* ── byte-wise stores — no alignment requirement on the caller ──────── */
static void scfs_st64(uint8_t *p, uint32_t off, int64_t v)
{
    uint64_t u = (uint64_t)v;
    uint32_t i;

    for (i = 0u; i < 8u; i++)
        p[off + i] = (uint8_t)((u >> (8u * i)) & 0xFFu);
}

/*
 * Copy the super block's counters into the caller's structure.
 * The caller has already proved bufsz >= SCFS_STATFS_SZ.
 */
static int scfs_fill_statfs(void *ubuf)
{
    uint8_t *p = (uint8_t *)ubuf;

    if (!p) return SCFS_EIO;

    scfs_st64(p, SCFS_STATFS_OFF_BSIZE,   (int64_t)sb_block_size());
    scfs_st64(p, SCFS_STATFS_OFF_BLOCKS,  (int64_t)sb_total_blocks());
    scfs_st64(p, SCFS_STATFS_OFF_BFREE,   (int64_t)sb_free_blocks());

    /* No reserved margin on this filesystem: every free block is
     * available to every user.  Reporting a margin that does not exist
     * would make df disagree with what a write can actually get, so the
     * two fields carry the same number and the reason is stated here
     * rather than left as a silent equality. */
    scfs_st64(p, SCFS_STATFS_OFF_BAVAIL,  (int64_t)sb_free_blocks());

    scfs_st64(p, SCFS_STATFS_OFF_FILES,   (int64_t)sb_total_inodes());
    scfs_st64(p, SCFS_STATFS_OFF_FFREE,   (int64_t)sb_free_inodes());

    /* There is no device field in this SuperBlock, so there is no
     * filesystem id to report.  Zero is the honest value; a caller that
     * needs to tell two mounts apart has to compare mount points. */
    scfs_st64(p, SCFS_STATFS_OFF_FSID,    0);

    scfs_st64(p, SCFS_STATFS_OFF_NAMEMAX, (int64_t)(MAX_NAME_LEN - 1));

    return SCFS_OK;
}

/* ── statfs() — by path name ────────────────────────────────────────── */
int uiox_kix_scfs_statfs(const char *path, void *buf, uint32_t bufsz)
{
    InCoreInode *ip;

    if (!path || !buf) return SCFS_EFAULT;
    if (bufsz < SCFS_STATFS_SZ) return SCFS_EINVAL;

    /* Resolve the path only to validate that it names something on a
     * mounted filesystem.  With one mount there is one super block. */
    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;
    iput(ip);                               /* 01_fsa */

    return scfs_fill_statfs(buf);
}

/* ── fstatfs() — by descriptor ──────────────────────────────────────── */
int uiox_kix_scfs_fstatfs(int fd, void *buf, uint32_t bufsz)
{
    scfs_file_t *f;

    if (!buf) return SCFS_EFAULT;
    if (bufsz < SCFS_STATFS_SZ) return SCFS_EINVAL;

    /* The descriptor already proved the file exists and is open, so no
     * namei is needed — only the super block read. */
    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    return scfs_fill_statfs(buf);
}

/*
 * fstype — a private magic number for the filesystem.
 *
 * Bach identifies a filesystem by testing values in its super block when
 * mount() reads it.  This filesystem is recognised by its GEOMETRY, so
 * the test is the geometry rather than a magic field the struct does not
 * carry.  Derived rather than invented, so two builds with different
 * BLOCK_SIZE report different values.
 */
int uiox_kix_scfs_fstype(const char *path)
{
    InCoreInode *ip;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;
    iput(ip);                               /* 01_fsa */

    return (int)(0x55465653u ^ (sb_block_size() << 16));
}

/*
 * quotactl — the per-user quota interface.
 *
 * Bach's kernel has no quota support: a filesystem has free blocks and
 * free inodes, and nothing tracks which user holds them.  SuperBlock has
 * no per-user fields, so there is nothing to read even if a caller asked.
 */
int uiox_kix_scfs_quotactl(int cmd, const char *dev, int id, void *addr)
{
    (void)cmd; (void)dev; (void)id; (void)addr;
    return SCFS_ENOSYS;
}
