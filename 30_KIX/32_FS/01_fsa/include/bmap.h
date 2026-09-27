/*
 *  30_KIX/32_FS/01_fsa/include/bmap.h
 *
 *  Block map of a logical file byte offset to a file system block.
 *  Bach, The Design of the UNIX Operating System, Ch.4 §3.
 *
 *  ── CHANGED in this revision ───────────────────────────────────────────
 *  BmapResult gained a DEVICE field.  The buffer cache below is
 *  multi-device: getblk/bread/breada all take (dev, blkno), and the page
 *  cache's map_fn already declares the same (dev, blkno) contract.  The
 *  block map was the only layer that could not supply a device number,
 *  so bmap() now returns one and every caller passes it down.
 *
 *  Without this the filesystem layer above had no device identity to give
 *  the buffer layer — which is the same missing field behind mount's
 *  m_dev being always 0 and rename's cross-filesystem test having nothing
 *  to compare.
 *
 *  ── the four outputs Bach names ───────────────────────────────────────
 *  "block number in the file system, byte offset into block, bytes of I/O
 *   in block, read ahead block number"
 *
 *  plus the device the block lives on, and a validity flag.
 *
 *  ── the extent write ops (v2.1.0) ─────────────────────────────────────
 *  bmap() and bmap_alloc() both need to place a block into an inode's
 *  extent map: bmap_alloc() for a fresh block, and unfs_cow_block() for a
 *  copy.  Those three operations live in bmap.c next to the read side, so
 *  the extent map has ONE owner and the two callers cannot drift apart.
 *
 *  @version 2.1.0  @date 2026-09-27
 */
#ifndef UIOX_BMAP_H
#define UIOX_BMAP_H

#include "inode.h"

/* ─────────────────────────────────────────────────────────────
 * Result of Algorithm bmap  (§3)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  dev;          /* device the block lives on               */
    uint32_t blkno;        /* block number in that device              */
    uint32_t blk_offset;   /* byte offset within that block            */
    uint32_t io_bytes;     /* bytes available for I/O this call        */
    uint32_t readahead_blk;/* next block for read-ahead, or 0          */
    bool     valid;        /* false when the offset maps to nothing    */
} BmapResult;

/* ─────────────────────────────────────────────────────────────
 * bmap API
 * ───────────────────────────────────────────────────────────── */

/*
 * Algorithm bmap  (§3)
 *
 * Maps a logical byte offset within a file to the physical disk block,
 * the device holding it, and the offset within that block.
 *
 * Walks the four INLINE EXTENTS, then the overflow extent-tree block when
 * i_extent_tree names one.  Returns a filled BmapResult; result.valid ==
 * false when the offset maps to nothing, which includes a HOLE — a hole
 * extent reports valid == false so the caller reads zeros rather than a
 * block.
 */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset);

/*
 * bmap_alloc — like bmap but allocates a block when the offset is not
 * already mapped.
 *
 * Allocates through unfs_alloc_run() (10_unfs's group-bitmap allocator)
 * and places the block into the inode's extent map.  Returns a valid
 * mapping on success; r.valid == false on failure, and in that case
 * r.blkno may still hold a block that was allocated and not placed — see
 * the ordering note in bmap.c.
 */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset);

/* ─────────────────────────────────────────────────────────────
 * Extent write ops — used by bmap_alloc() and by unfs_cow_block()
 * ───────────────────────────────────────────────────────────── */

/* Which inline slot covers logical block lb, or -1 when none does. */
int bmap_extent_find(const InCoreInode *ip, uint32_t lb);

/*
 * Place physical block p at logical block lb.
 *
 *   covers lb, length 1   -> repoint
 *   covers lb, longer     -> split into head / changed / tail
 *   no cover, free slot   -> append a length-1 extent
 *   no cover, array full  -> UNFS_ENOTSUP (the overflow tree is not
 *                            written by anything yet)
 *
 * Returns UNFS_OK or a negative UNFS_E* code.
 */
int bmap_extent_place(InCoreInode *ip, uint32_t lb, uint32_t p);

#endif /* UIOX_BMAP_H */
