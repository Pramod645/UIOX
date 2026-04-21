#ifndef UIOX_INODE_H
#define UIOX_INODE_H

#include "fs_types.h"
#include "buffer.h"

/* ─────────────────────────────────────────────────────────────
 * On-disk inode layout
 *
 * block_num  = ((ino - 1) / INODES_PER_BLOCK) + INODE_START_BLOCK
 * byte_offset = ((ino - 1) % INODES_PER_BLOCK) * sizeof(DiskInode)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint16_t  mode;        /* FileType + permission bits             */
    uint16_t  nlink;       /* hard link count                        */
    uint16_t  uid;
    uint16_t  gid;
    uint32_t  size;        /* file size in bytes                     */
    uint32_t  addr[NDIRECT + NINDIRECT + NDINDIRECT + NTINDIRECT];
    time_t    atime;       /* last access                            */
    time_t    mtime;       /* last data modification                 */
    time_t    ctime;       /* last inode change                      */
} DiskInode;

/* ─────────────────────────────────────────────────────────────
 * In-core inode (inode cache entry)
 * ───────────────────────────────────────────────────────────── */
typedef struct InCoreInode {
    /* ── In-core-only fields ──────────────────────────────── */
    uint32_t         ino;          /* inode number                   */
    int              refcount;     /* open references                */
    bool             locked;
    bool             on_free_list;
    uint8_t          flags;        /* IFLAG_* dirty bits             */

    /* ── Fields mirrored from DiskInode ──────────────────── */
    uint16_t         mode;
    uint16_t         nlink;
    uint16_t         uid;
    uint16_t         gid;
    uint32_t         size;
    uint32_t         addr[NDIRECT + NINDIRECT + NDINDIRECT + NTINDIRECT];
    time_t           atime;
    time_t           mtime;
    time_t           ctime;

    /* ── Hash / free list links ───────────────────────────── */
    struct InCoreInode *hash_next;
    struct InCoreInode *free_next;
    struct InCoreInode *free_prev;
} InCoreInode;

/* ─────────────────────────────────────────────────────────────
 * Inode cache API
 * ───────────────────────────────────────────────────────────── */
void          inode_cache_init(void);

/*
 * Algorithm iget  (§1)
 * Allocate or retrieve an in-core inode for inode number 'ino'.
 * Returns a locked InCoreInode, or NULL on error.
 */
InCoreInode  *iget(uint32_t ino);

/*
 * Algorithm iput  (§2)
 * Release an in-core inode.
 * Frees disk blocks and inode if link count reaches 0.
 */
void          iput(InCoreInode *ip);

/* Write in-core inode back to disk */
void          iupdate(InCoreInode *ip);

/* Read a DiskInode from disk into buf; caller provides inode number */
DiskInode    *inode_disk_read(uint32_t ino, BufEntry **out_buf);

/* Return FileType portion of mode word */
static inline FileType inode_type(const InCoreInode *ip)
{
    return (FileType)((ip->mode >> 12) & 0xF);
}

/* Return permission bits from mode */
static inline uint16_t inode_perm(const InCoreInode *ip)
{
    return (uint16_t)(ip->mode & 0x1FF);
}

/* Convenience: check that 'uid' has read permission */
bool inode_access_ok(const InCoreInode *ip, uint16_t uid, uint16_t gid,
                     int want_read, int want_write, int want_exec);

#endif /* UIOX_INODE_H */
