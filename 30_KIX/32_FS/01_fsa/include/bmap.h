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
 *  @version 2.0.0  @date 2026-09-24
 */
#ifndef UIOX_BMAP_H
#define UIOX_BMAP_H

#include "inode.h"

/* ─────────────────────────────────────────────────────────────
 * Result of Algorithm bmap  (§3)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  dev;          /* device the block lives on   ◀ NEW       */
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
 * Handles direct, single-, double- and triple-indirect blocks.
 * Returns a filled BmapResult; result.valid == false on error or when
 * the offset is past the end of the mapped range.
 */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset);

/*
 * bmap_alloc — like bmap but allocates missing blocks.
 * Used during writes to extend or fill sparse files.
 */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset);

#endif /* UIOX_BMAP_H */
