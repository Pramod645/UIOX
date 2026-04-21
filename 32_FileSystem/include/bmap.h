#ifndef UIOX_BMAP_H
#define UIOX_BMAP_H

#include "inode.h"

/* ─────────────────────────────────────────────────────────────
 * Result of Algorithm bmap  (§3)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t blkno;          /* block number in filesystem         */
    uint32_t blk_offset;     /* byte offset within that block      */
    uint32_t io_bytes;       /* bytes available for I/O this call  */
    uint32_t readahead_blk;  /* next block for read-ahead (or 0)   */
    bool     valid;
} BmapResult;

/* ─────────────────────────────────────────────────────────────
 * bmap API
 * ───────────────────────────────────────────────────────────── */

/*
 * Algorithm bmap  (§3)
 *
 * Maps a logical byte offset within a file to the physical disk
 * block and offset within that block.
 *
 * Handles:
 *   direct blocks (0..NDIRECT-1)
 *   single-indirect block
 *   double-indirect block
 *   triple-indirect block
 *
 * Returns a filled BmapResult; result.valid == false on error.
 */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset);

/*
 * bmap_alloc — like bmap but allocates missing blocks.
 * Used during writes to extend or fill sparse files.
 */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset);

#endif /* UIOX_BMAP_H */
