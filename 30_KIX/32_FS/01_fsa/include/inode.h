/*
 *  30_KIX/32_FS/01_fsa/include/inode.h
 *
 *  In-core and on-disk inode.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §1.
 *
 *  ── CHANGED in this revision ───────────────────────────────────────────
 *  Both inode structures gained a DEVICE field.
 *
 *  Why it is needed: the buffer cache below is multi-device —
 *  getblk/bread/breada all take (dev, blkno) — and the page cache's
 *  map_fn already declares the same (dev, blkno) contract.  An inode
 *  lives on exactly ONE filesystem, so the device belongs to the inode
 *  rather than being passed alongside it everywhere.
 *
 *  This one field is what unblocks four separate places at once:
 *    · bmap()  can now fill BmapResult.dev, which bread() needs
 *    · mount() can record a real m_dev instead of 0
 *    · rename()'s cross-filesystem test has something to compare
 *    · stat()'s device bytes are no longer forced to zero
 *
 *  ── the disk layout ──────────────────────────────────────────────────
 *  dev is stored as uint8_t and placed LAST in DiskInode, so a filesystem
 *  image written before this change still reads correctly: the older
 *  struct simply had no trailing byte, and the field reads as 0 — which
 *  is device 0, the root filesystem.  Nothing before it moves.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#ifndef UIOX_INODE_H
#define UIOX_INODE_H

#include "fs_types.h"
#include "buffer.h"

/* ═════════════════════════════════════════════════════════════════════
 * On-disk inode layout
 *
 *   block_num   = ((ino - 1) / INODES_PER_BLOCK) + INODE_START_BLOCK
 *   byte_offset = ((ino - 1) % INODES_PER_BLOCK) * sizeof(DiskInode)
 *
 * dev is appended last so the earlier fields keep their offsets.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct {
    uint16_t  mode;        /* FileType nibble | permission bits        */
    uint16_t  nlink;       /* hard link count                          */
    uint16_t  uid;
    uint16_t  gid;
    uint32_t  size;        /* file size in bytes                       */
    uint32_t  addr[NDIRECT + NINDIRECT + NDINDIRECT + NTINDIRECT];
    time_t    atime;       /* last access                              */
    time_t    mtime;       /* last data modification                   */
    time_t    ctime;       /* last inode change                        */

    uint8_t   dev;         /* device this inode lives on   ◀ NEW       */
    uint8_t   pad[3];      /* keep sizeof a multiple of 8              */
} DiskInode;

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
    uint32_t         addr[NDIRECT + NINDIRECT + NDINDIRECT + NTINDIRECT];
    time_t           atime;
    time_t           mtime;
    time_t           ctime;

    uint8_t          dev;          /* device this inode lives on ◀ NEW */

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

/* ── FileType portion of the mode word ───────────────────────────── */
static inline FileType inode_type(const InCoreInode *ip)
{
    return (FileType)((ip->mode >> 12) & 0xF);
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
