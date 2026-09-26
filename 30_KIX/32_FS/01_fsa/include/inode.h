/*
 *  30_KIX/32_FS/01_fsa/include/inode.h
 *
 *  In-core and on-disk inode — UNFS format.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §1.
 *
 *  ── CHANGED in this revision (v3.0.0) ─────────────────────────────────
 *  addr[13] is GONE.  Both structures now carry UNFS's extent fields:
 *
 *      unfs_extent_t i_extents[4];   four inline extents
 *      uint32_t      i_extent_tree;  overflow extent-tree block, or 0
 *
 *  The indirect scheme (10 direct + single + double + triple) covered
 *  about 5 KB of a 512-byte-block geometry.  At UNFS's 4096-byte blocks
 *  and 256-byte inode, four inline extents cover 4 × 64 KB directly with
 *  one overflow level — and a 512 KB file is ONE extent entry rather than
 *  128 addr[] slots to fill and follow.
 *
 *  This is not a deviation from Bach.  Bach's bmap() is "map a file
 *  offset to a block number"; the mechanism is an implementation detail,
 *  and the signature is unchanged, so bmap.c, namei.c, readwrite.c,
 *  superblock.c and truncate.c all call it as written.
 *
 *  ── the disk layout ──────────────────────────────────────────────────
 *  DiskInode must be exactly UNFS_INODE_SIZE (256) bytes.  The static
 *  assert below pins it: INODES_PER_BLOCK is derived from this size, so
 *  a field added without thought would shift every inode on disk.
 *
 *  dev is kept as uint8_t and placed LAST.  It is NOT part of the UNFS
 *  on-disk record — unfs_format.h's inode has no device field — so it
 *  lives in the last byte of the 256-byte slot, which UNFS reserves as
 *  padding.  That keeps the in-core dev working without touching format.
 *
 *  ── the type-encoding conflict is RESOLVED ───────────────────────────
 *  FileType is gone, and inode_type() with it.  They are replaced by
 *  inode_is_dir() / inode_is_reg() / inode_is_lnk(), which test
 *  UNFS_IFMT against the on-disk UNFS_IF* values.
 *
 *  unfs_format.h is the authority — it says of its type block "These are
 *  the ON-DISK values and must not change".  And the enum was worse than
 *  a different spelling: it packed into the SAME bits, so the two
 *  encodings COLLIDED rather than merely disagreeing.
 *
 *      (FT_DIR    << 12) = 0x2000 = UNFS_IFCHR   → dir reads as a chardev
 *      (FT_REGULAR<< 12) = 0x1000 = UNFS_IFIFO
 *      (FT_CHAR   << 12) = 0x3000 = nothing at all
 *
 *  A character device is the one that shows up: mkfs wrote FT_DIR, and
 *  inode_type() reported FT_BLOCK on the way back — which is why a
 *  directory and a block device were the two symptoms seen.
 *
 *  @version 3.2.0  @date 2026-09-26
 */
#ifndef UIOX_INODE_H
#define UIOX_INODE_H

#include "fs_types.h"       /* brings in unfs_format.h */
#include "buffer.h"

/* ═════════════════════════════════════════════════════════════════════
 * On-disk inode layout
 *
 *   block_num   = ((ino - 1) / INODES_PER_BLOCK) + UNFS_GROUP0_ITABLE
 *   byte_offset = ((ino - 1) % INODES_PER_BLOCK) * sizeof(DiskInode)
 *
 * INODES_PER_BLOCK comes from fs_types.h as (UNFS_BLOCK_SIZE /
 * sizeof(DiskInode)) — 4096 / 256 = 16, not 8.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    /* ── the FORMAT's fields, in the format's order ────────────────
     * This struct is the on-disk image, so it mirrors unfs_inode_t in
     * unfs_format.h field for field.  An earlier revision carried only
     * a SUBSET — no i_mac_label, no i_inline, no i_checksum — and used
     * `time_t` for what the format declares as uint64_t nanoseconds.
     * It totalled 92 bytes against a 256-byte target and the assert
     * below caught it.  Do not "simplify" this list. */
    uint16_t  i_mode;      /* UNFS_IF* | permission bits               */
    uint16_t  i_uid;
    uint16_t  i_gid;
    uint16_t  i_nlink;     /* hard link count                          */
    uint64_t  i_size;      /* file size in bytes                       */
    uint64_t  i_atime_ns;  /* last access, ns since epoch              */
    uint64_t  i_mtime_ns;  /* last data modification                   */
    uint64_t  i_ctime_ns;  /* last inode change                        */
    uint32_t  i_blocks;    /* 512-byte blocks allocated                */
    uint32_t  i_flags;     /* misc flags                               */

    uint8_t   i_mac_label[16];  /* MAC security label — 33_PCS/05_sec  */
    uint32_t  i_mac_flags;

    /* ── UNFS extent mapping, replacing addr[13] ───────────────────
     * sizeof(unfs_extent_t) is 12 — 4+4+2+2 — NOT 8.  The inline
     * comment in unfs_format.h that reads "4 x 8 = 32 bytes" is wrong:
     * the array is 48.  That single miscount is what made the format's
     * own size assert fail. */
    unfs_extent_t i_extents[UNFS_INLINE_EXTENTS];   /* 4 x 12 = 48  */
    uint32_t      i_extent_tree;                    /* overflow or 0 */

    uint8_t   i_inline[60];     /* inline symlink target               */
    uint32_t  i_checksum;       /* CRC32C of bytes 0..251              */

    /* ── in-core only: the device this inode lives on ──────────────
     * NOT part of the UNFS record.  unfs_format.h's inode has no device
     * field, so this sits where the format keeps padding.  It costs 8
     * bytes of the pad (1 for dev plus 7 to keep the 8-byte tail), and
     * therefore shrinks _pad below from 72 to 64 — which is why this
     * struct and the format's are the same size but not identical. */
    uint8_t   dev;
    uint8_t   dev_pad[7];

    /* ── EXPLICIT PAD — 64 + 184 + 8 = 256 ────────────────────────
     *         fields above (as the format declares them)    184
     *         dev + dev_pad                                  8
     *         -------------------------------------------------
     *         subtotal                                     192
     *         this pad                                      64
     *         -------------------------------------------------
     *         sizeof                                       256
     *
     * The format's own unfs_inode_t is 184 + _pad[72] = 256.  DiskInode
     * spends 8 of those 72 on dev, leaving 64.
     *
     * A future format field must REPLACE part of this pad — never be
     * appended after it, or sizeof becomes 257 and the assert fails in
     * the other direction. */
    uint8_t   _pad[64];
} DiskInode;

/* The layout arithmetic only holds at exactly 256 bytes.  If a field is
 * added and this fires, INODES_PER_BLOCK, the group inode-table size and
 * every inode offset on disk change with it — fix the struct, not the
 * assert. */
_Static_assert(sizeof(DiskInode) == UNFS_INODE_SIZE,
               "DiskInode must be exactly UNFS_INODE_SIZE (256) bytes");

/* ═════════════════════════════════════════════════════════════════════
 * In-core inode (inode cache entry)
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct InCoreInode {
    /* ── In-core-only fields ─────────────────────────────────────── */
    uint32_t         ino;          /* inode number                   */
    int              refcount;     /* open references                */
    bool             locked;
    bool             on_free_list;
    uint8_t          flags;        /* IFLAG_* dirty bits             */

    /* ── Fields mirrored from DiskInode ─────────────────────────── */
    uint16_t         mode;
    uint16_t         nlink;
    uint16_t         uid;
    uint16_t         gid;
    uint32_t         size;

    /* Mirror of the on-disk extent map.  bmap() reads ip->i_extents
     * and ip->i_extent_tree — the same field names as the disk struct,
     * so a copy is a plain assignment with no renaming. */
    unfs_extent_t    i_extents[UNFS_INLINE_EXTENTS];
    uint32_t         i_extent_tree;

    time_t           atime;
    time_t           mtime;
    time_t           ctime;

    uint8_t          dev;          /* device this inode lives on     */

    /* ── Hash / free list links ─────────────────────────────────── */
    struct InCoreInode *hash_next;
    struct InCoreInode *free_next;
    struct InCoreInode *free_prev;
} InCoreInode;

/* ═════════════════════════════════════════════════════════════════════
 * Inode cache API
 * ═════════════════════════════════════════════════════════════════════ */
void          inode_cache_init(void);

/*
 * Algorithm iget  (§1)
 * Allocate or retrieve an in-core inode for inode number 'ino'.
 * Returns a locked InCoreInode, or NULL on error.
 *
 * The DEVICE must be supplied by the caller — an inode number alone is
 * ambiguous once more than one filesystem is mounted.  iget_dev() is the
 * general form; iget(ino) is the single-filesystem convenience.
 */
InCoreInode  *iget_dev(uint8_t dev, uint32_t ino);
InCoreInode  *iget(uint32_t ino);          /* dev = ROOT_DEV (0)      */

/*
 * Algorithm iput  (§2)
 * Release an in-core inode.
 * Frees disk blocks and the inode if the link count reaches 0.
 */
void          iput(InCoreInode *ip);

/* Write in-core inode back to disk */
void          iupdate(InCoreInode *ip);

/* Read a DiskInode from disk into buf; caller provides inode number */
DiskInode    *inode_disk_read(uint8_t dev, uint32_t ino, BufHdr **out_buf);

/* The device the root filesystem lives on — 0 unless a platform says
 * otherwise.  Used by the iget(ino) convenience above. */
#define ROOT_DEV  0u

/* ── File type from the mode word ────────────────────────────────────
 * inode_type() is GONE, and the FileType enum with it.
 *
 * Three type encodings existed in this stack and only two are on disk:
 *
 *      FileType enum        FT_DIR = 2           REMOVED — was in-core
 *      UNFS_IF* in i_mode   UNFS_IFDIR = 0040000 ON DISK  ◀ canonical
 *      UNFS_DT_* in d_type  UNFS_DT_DIR = 4      ON DISK (dirent only)
 *
 * FileType was not merely a different spelling — it packed into the SAME
 * bits as UNFS_IF*, so the two COLLIDED:
 *
 *      (FT_DIR     << 12) = 0x2000 = UNFS_IFCHR
 *      (FT_REGULAR << 12) = 0x1000 = UNFS_IFIFO
 *
 * mkfs wrote FT_DIR (0x2000 in the mode word) and inode_type() read the
 * nibble back as FT_BLOCK — the two visible symptoms.
 *
 * These predicates test UNFS_IFMT, which masks the format bits out of
 * i_mode.  Use them in place of any enum comparison.
 * ───────────────────────────────────────────────────────────────────── */
static inline uint16_t inode_type_bits(const InCoreInode *ip)
{
    return (uint16_t)(ip->mode & UNFS_IFMT);
}

static inline bool inode_is_dir(const InCoreInode *ip)
{
    return ip && inode_type_bits(ip) == UNFS_IFDIR;
}

static inline bool inode_is_reg(const InCoreInode *ip)
{
    return ip && inode_type_bits(ip) == UNFS_IFREG;
}

static inline bool inode_is_lnk(const InCoreInode *ip)
{
    return ip && inode_type_bits(ip) == UNFS_IFLNK;
}

static inline bool inode_is_chr(const InCoreInode *ip)
{
    return ip && inode_type_bits(ip) == UNFS_IFCHR;
}

static inline bool inode_is_blk(const InCoreInode *ip)
{
    return ip && inode_type_bits(ip) == UNFS_IFBLK;
}

/* Compose a mode word from a UNFS_IF* type and permission bits.  This is
 * the ONE place the two halves are joined, so ialloc() and every future
 * caller agree about the encoding. */
static inline uint16_t inode_make_mode(uint16_t unfs_if, uint16_t perm)
{
    return (uint16_t)((unfs_if & UNFS_IFMT) | (perm & 0x1FFu));
}

/* ── Permission bits from the mode word ──────────────────────────── */
static inline uint16_t inode_perm(const InCoreInode *ip)
{
    return (uint16_t)(ip->mode & 0x1FF);
}

/* ── Permission test ─────────────────────────────────────────────────
 * NOTE: this performs NO super-user bypass.  Bach's kernel has one
 * ("the super user bypasses all permission checks except execute on a
 * file"); this implementation tests owner / group / other only.  If a
 * bypass is wanted it belongs HERE, in one place, so that open, chdir,
 * access and every other caller get the same answer.
 */
bool inode_access_ok(const InCoreInode *ip, uint16_t uid, uint16_t gid,
                     int want_read, int want_write, int want_exec);

#endif /* UIOX_INODE_H */
