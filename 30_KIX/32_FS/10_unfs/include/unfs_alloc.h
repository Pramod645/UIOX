/*
 * 30_KIX/32_FS/10_unfs/include/unfs_alloc.h
 *
 * UNFS block and inode allocation — over the group BITMAPS, not Bach's
 * free list.
 *
 * ── why this is separate from superblock.h ────────────────────────────
 * 01_fsa already has an allocator: fs_alloc_begin / fs_alloc_commit_dev /
 * fs_alloc / fs_free, and SuperBlock carries Bach's free_blocks[] chain
 * with free_block_idx as its cursor.  That is a free-LIST design — a
 * linked list of block-number arrays, Ch.4 §5.
 *
 * UNFS does not store a free list on disk.  unfs_format.c lays down a
 * BITMAP per group, one bit per block, and the superblock carries only
 * the counts (s_free_blocks / s_free_inodes).  The two models cannot be
 * merged: a bitmap can answer "give me five contiguous blocks" and a
 * free list cannot, which is exactly what an extent allocator needs.
 *
 * Nothing here takes a filesystem pointer.  The device number and the
 * group index are the whole input, so this allocator has no dependency
 * on 01_fsa or on unfs_fs_t — and no shared state with Bach's.
 *
 * ── THE BIT CONVENTION, which is the opposite of the obvious ─────────
 * unfs_format.c's helpers, verbatim:
 *
 *     bitmap_set_free    bm[bit >> 3] |=  (1 << (bit & 7))   1 = FREE
 *     bitmap_set_used    bm[bit >> 3] &= ~(1 << (bit & 7))   0 = USED
 *
 * A SET bit means FREE.  This is inverted from ext2 and from most
 * bitmaps, and it is the single thing to get wrong here: a scan that
 * reads a set bit as "allocated" hands out blocks already in use, and
 * the damage surfaces much later as two files sharing a block.  Every
 * scan below looks for a SET bit to find a free block and CLEARS it to
 * allocate.
 *
 * ── ordering: bitmap first, then the counter ──────────────────────────
 * unfs_format.c writes the bitmap block and only then adjusts
 * s_free_blocks.  Allocation follows the same order: clear the bits,
 * write the bitmap, decrement.  A crash in between leaves the count too
 * HIGH by the number of blocks being placed, which is self-correcting —
 * the next allocation finds no free bit and stops.  The reverse order
 * would leave the count too low, which is also safe, but reporting free
 * blocks as used is the direction that wastes nothing.
 *
 * @version 1.0.0  @date 2026-09-27
 */
#ifndef UNFS_ALLOC_H
#define UNFS_ALLOC_H

#include "unfs_format.h"
#include "unfs_errno.h"

/* ── One contiguous run of blocks ─────────────────────────────────────
 * The unit this allocator hands out.  An extent allocator wants a run,
 * not a block: four inline extents cover 4 x 64 KB, so allocating one
 * block at a time fills the inline slots immediately and pushes every
 * file into the overflow tree.
 *
 * A run never spans two groups.  The bitmaps are per-group, so a span
 * would need two bitmap updates with no way to make them atomic; a
 * caller wanting more than one group's worth calls twice. */
typedef struct {
    uiox_uint32_t first_blk;   /* first block, UNFS block units (4096) */
    uiox_uint32_t count;       /* blocks in the run; 0 = none found    */
    uiox_uint32_t group;       /* which group the run came from        */
} unfs_run_t;

/* ── Block allocation ─────────────────────────────────────────────────
 * unfs_alloc_run — find and claim 'want' contiguous free blocks.
 *
 * Scans for a run of 'want' set bits, clears them, writes the bitmap
 * back and decrements s_free_blocks.  Returns UNFS_OK with out->count
 * set, or UNFS_ENOSPC when no group has a run that long.
 *
 * FIRST-FIT with no cooldown, which matches the layout mkfs produces:
 * blocks are marked free in ascending order from UNFS_GROUP0_DATA, so a
 * fresh volume stays nearly contiguous.  A rotating hint would fragment
 * it for no benefit at this size.
 *
 * UNFS block units throughout.  Conversion to bcache sectors is
 * unfs_io.c's job (UNFS_SECTORS_PER_BLOCK). */
int unfs_alloc_run(uiox_uint32_t dev, uiox_uint32_t want, unfs_run_t *out);

/* unfs_alloc_block — one block for a caller that does not care which.
 * Returns the block number, or 0 on failure — 0 is never a data block,
 * so it doubles as the error value. */
uiox_uint32_t unfs_alloc_block(uiox_uint32_t dev);

/* ── Block freeing ────────────────────────────────────────────────────
 * Sets the bits back and increments s_free_blocks.  Same order as
 * allocation — bitmap, then counter — for the mirrored reason. */
int unfs_free_run(uiox_uint32_t dev, uiox_uint32_t first_blk,
                  uiox_uint32_t count);

/* ── Statistics ───────────────────────────────────────────────────────
 * Counts set bits in one group's bitmap.  For fsck-shaped code, and for
 * a test that wants to prove the superblock counter and the bitmap
 * agree — which is the invariant the ordering rules above protect. */
uiox_uint32_t unfs_group_free_blocks(uiox_uint32_t dev, uiox_uint32_t group);

/* ── Inode bitmap ─────────────────────────────────────────────────────
 * Same convention, same ordering, same bit arithmetic — kept here rather
 * than in a second header, because splitting them would duplicate the
 * one comment that matters. */
int unfs_alloc_inode_bit(uiox_uint32_t dev, uiox_uint32_t *ino_out);
int unfs_free_inode_bit (uiox_uint32_t dev, uiox_uint32_t ino);

#endif /* UNFS_ALLOC_H */
