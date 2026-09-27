/*
 *  30_KIX/32_FS/01_fsa/src/bmap.c
 *
 *  Extent-based block map — UNFS's scheme, in place of Bach Ch.4 §3.
 *
 *  ── why this is not the indirect tree ─────────────────────────────────
 *  Bach's bmap() walks addr[13]: 10 direct pointers, then single-,
 *  double- and triple-indirect levels, each requiring a bread() to follow.
 *  That design fits 512-byte blocks, where 13 pointers cover about 5 KB
 *  and indirect levels are genuinely needed.
 *
 *  UNFS uses 4096-byte blocks and a 256-byte inode, and stores EXTENTS:
 *
 *      unfs_extent_t i_extents[4];    four inline extents
 *      uint32_t      i_extent_tree;   overflow tree block, or 0
 *
 *  Four inline extents already cover 4 x 64 KB directly, with one
 *  overflow level beyond that.  A 512 KB file is one extent entry, not
 *  128 map entries.
 *
 *  ── the units trap ────────────────────────────────────────────────────
 *  UNFS_BLOCK_SIZE is 4096.  00_buffcache's unit is 512
 *  (BCACHE_SECTOR_SIZE).  bmap() returns an EXTENT address, so:
 *
 *      r.blkno  = UNFS block number        (4096-byte units)
 *      r.dev    = the device, from the inode
 *
 *  A caller that passes r.blkno straight to bread() reads the first 512
 *  bytes of the wrong location — one eighth of the way in.  The
 *  conversion is UNFS_SECTORS_PER_BLOCK (8), applied at the buffer
 *  boundary, so this file stays in UNFS units throughout.
 *
 *  ── WHAT CHANGED IN v3.1.0 ────────────────────────────────────────────
 *
 *  1. extree_lookup() NO LONGER READS PAST ITS BUFFER.  The tree block is
 *     ONE 4096-byte UNFS block holding an array of 12-byte extents — 341
 *     of them.  bread() returns a BufHdr whose data[] is BLOCK_SIZE = 512
 *     bytes, so the old loop
 *
 *         buf = bread(dev, tree_blk * UNFS_SECTORS_PER_BLOCK);
 *         et  = (const unfs_extent_t *)buf->data;
 *         for (i = 0; i < EXTREE_PER_BLOCK; i++)     EXTREE_PER_BLOCK = 341
 *
 *     read 299 entries PAST the end of the buffer.  Any file with more
 *     than ~42 overflow extents would have hit it.  The fix reads ALL
 *     EIGHT SECTORS rather than shrinking the bound — bounding at 42
 *     would be safe but would make 299 reachable entries unreachable.
 *
 *  2. bmap_alloc() IS IMPLEMENTED.  It was a stub returning
 *     valid = false so every write stopped at ENOSPC.  It now allocates
 *     through unfs_alloc_run() and places the block into the inode's
 *     extent map.
 *
 *  3. THE EXTENT WRITE OPS ARE HERE, not in the snapshot file.  Both
 *     bmap_alloc() and unfs_cow_block() need to place a block into an
 *     extent map; duplicating that logic somewhere bmap() cannot see is
 *     how the two would drift apart.
 *
 *  @version 3.1.0  @date 2026-09-27
 */
#include "bmap.h"
#include "inode.h"
#include "buffer.h"
#include "uiox_klibc.h"
#include "unfs_alloc.h"     /* unfs_alloc_run, unfs_run_t            */
#include "unfs_errno.h"

/*
 * The overflow extent-tree block is an array of unfs_extent_t, 12 bytes
 * each — 4 + 4 + 2 + 2, NOT 8.  An earlier comment here said "the 8-byte
 * entry" and that miscount is what made the format's own size assert
 * fail.
 *
 * 4096 / 12 = 341 entries in the block, which is the number of entries
 * the READER may see.  It is NOT the number reachable from one 512-byte
 * buffer — see extree_lookup() below.
 */
#define EXTREE_PER_BLOCK ((uint32_t)(UNFS_BLOCK_SIZE / sizeof(unfs_extent_t)))

/* Entries that fit in ONE buffer-cache buffer (BLOCK_SIZE == 512). */
#define EXTREE_PER_SECTOR ((uint32_t)(BLOCK_SIZE / sizeof(unfs_extent_t)))

/* ─────────────────────────────────────────────────────────────
 * Internal: does this extent cover logical block 'lb'?
 * ───────────────────────────────────────────────────────────── */
static int extent_covers(const unfs_extent_t *e, uint32_t lb)
{
    if (e->e_len == 0u) return 0;                  /* empty slot */
    if (lb <  e->e_logical) return 0;
    if (lb >= (uint32_t)e->e_logical + (uint32_t)e->e_len) return 0;
    return 1;
}

/* ─────────────────────────────────────────────────────────────
 * Internal: map logical block via one extent.
 *
 * Returns the physical block, or 0 for "not mapped here".  A hole extent
 * (UNFS_EXT_HOLE) is deliberately reported as 0 as well: the caller sees
 * an unmapped block and reads zeros, which is what a hole IS.
 * ───────────────────────────────────────────────────────────── */
static uint32_t extent_phys(const unfs_extent_t *e, uint32_t lb)
{
    uint32_t delta;

    if (!extent_covers(e, lb)) return 0u;
    if (e->e_flags & UNFS_EXT_HOLE) return 0u;

    delta = lb - (uint32_t)e->e_logical;
    return (uint32_t)e->e_physical + delta;
}

/* ─────────────────────────────────────────────────────────────
 * Internal: search the overflow extent-tree block.
 *
 * The tree block is one 4096-byte UNFS block holding an array of extents,
 * terminated by an entry with e_len == 0.  A single level, as the
 * bootloader's extent_lookup() assumes — so the two agree.
 *
 * WHY THE SECTOR LOOP: buf->data is BLOCK_SIZE (512), so one buffer holds
 * only EXTREE_PER_SECTOR entries.  The old single-buffer form read
 * EXTREE_PER_BLOCK of them — 299 past the end.  This walks all
 * UNFS_SECTORS_PER_BLOCK sectors and bounds each buffer's read at what
 * that buffer actually holds.
 *
 * The entry is COPIED out rather than aliased: unfs_extent_t is packed,
 * but a pointer cast into a uint8_t array can still trip
 * -Werror=cast-align on some targets, and a 12-byte memcpy is free.
 * ───────────────────────────────────────────────────────────── */
static uint32_t extree_lookup(uint8_t dev, uint32_t tree_blk, uint32_t lb)
{
    uint32_t base;
    uint32_t s;
    uint32_t k;

    if (EXTREE_PER_SECTOR == 0u) return 0u;

    base = tree_blk * (uint32_t)UNFS_SECTORS_PER_BLOCK;

    for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
        BufHdr *buf = bread(dev, base + s);
        if (!buf) return 0u;

        for (k = 0u; k < EXTREE_PER_SECTOR; k++) {
            unfs_extent_t et;
            uint32_t      slot = k * (uint32_t)sizeof(unfs_extent_t);

            memcpy(&et, buf->data + slot, sizeof(et));

            if (et.e_len == 0u) { brelse(buf); return 0u; }  /* end */
            if (extent_covers(&et, lb)) {
                uint32_t p = extent_phys(&et, lb);
                brelse(buf);
                return p;
            }
        }
        brelse(buf);
    }

    return 0u;
}

/* ═════════════════════════════════════════════════════════════
 * bmap — map a file byte offset to a device block
 * ═════════════════════════════════════════════════════════════ */
BmapResult bmap(InCoreInode *ip, uint32_t byte_offset)
{
    BmapResult r;
    uint32_t   lb;          /* logical block within the file       */
    uint32_t   i;

    memset(&r, 0, sizeof r);

    if (!ip) return r;

    r.dev = ip->dev;

    /* UNFS units: the offset is divided by 4096, not 512 */
    lb           = byte_offset / UNFS_BLOCK_SIZE;
    r.blk_offset = byte_offset % UNFS_BLOCK_SIZE;
    r.io_bytes   = UNFS_BLOCK_SIZE - r.blk_offset;

    /* ── the four inline extents ─────────────────────────────────── */
    for (i = 0u; i < (uint32_t)UNFS_INLINE_EXTENTS; i++) {
        uint32_t p = extent_phys(&ip->i_extents[i], lb);

        if (extent_covers(&ip->i_extents[i], lb)) {
            r.blkno = p;
            /* a hole maps to block 0 — valid=false, caller reads zeros */
            r.valid = (p != 0u);
            return r;
        }
    }

    /* ── the overflow extent tree ────────────────────────────────── */
    if (ip->i_extent_tree != 0u) {
        uint32_t p = extree_lookup(ip->dev, ip->i_extent_tree, lb);
        r.blkno = p;
        r.valid = (p != 0u);
        return r;
    }

    /* No extent covers this offset: past EOF, or an unmapped region.
     * Bach's bmap() reports the same way — valid stays false and the
     * read stops. */
    return r;
}

/* ═════════════════════════════════════════════════════════════
 * EXTENT WRITE OPS
 *
 * bmap() maps a file offset to a block.  PLACING a block into the map was
 * bmap_alloc()'s job and it was a stub; unfs_cow_block() has the same
 * need from the other direction — it copies a block but cannot repoint
 * the extent at the copy.  Both call the two functions below.
 * ═════════════════════════════════════════════════════════════ */

/* ─────────────────────────────────────────────────────────────
 * bmap_extent_find — which inline slot covers lb, or -1
 * ───────────────────────────────────────────────────────────── */
int bmap_extent_find(const InCoreInode *ip, uint32_t lb)
{
    uint32_t i;

    if (!ip) return -1;

    for (i = 0u; i < (uint32_t)UNFS_INLINE_EXTENTS; i++) {
        if (extent_covers(&ip->i_extents[i], lb)) return (int)i;
    }
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * bmap_extent_place — put physical block p at logical block lb
 *
 *   covers lb, length 1   -> repoint e_physical.  No structural change.
 *   covers lb, longer     -> SPLIT.  A file with one 8-block extent and a
 *                            change to block 3 needs three entries:
 *                            [0..2] old, [3] new, [4..7] old.  Needs one
 *                            free slot.
 *   no cover, free slot   -> append a length-1 extent.
 *   no cover, array full  -> UNFS_ENOTSUP.  The overflow tree is READABLE
 *                            (extree_lookup above) but nothing WRITES one
 *                            yet; spilling means allocating a tree block,
 *                            writing the entents into it, clearing the
 *                            inline array and recording the block.
 *
 * Cases 1-3 hold for any file whose data is contiguous up to four extents'
 * worth of fragmentation — which is what a fresh volume produces, since
 * unfs_alloc_run() allocates in ascending order.
 * ───────────────────────────────────────────────────────────── */
int bmap_extent_place(InCoreInode *ip, uint32_t lb, uint32_t p)
{
    int      slot;
    uint32_t i;
    uint32_t first;
    uint32_t len;

    if (!ip) return UNFS_EINVAL;
    if (p == 0u) return UNFS_EINVAL;

    slot = bmap_extent_find(ip, lb);

    /* ── case 1: covers lb, length 1 — repoint ──────────────────────── */
    if (slot >= 0) {
        unfs_extent_t *e = &ip->i_extents[slot];

        if (e->e_len == 1u) {
            e->e_physical = p;
            e->e_flags    = UNFS_EXT_LEAF;
            return UNFS_OK;
        }

        /* ── case 2: covers lb, longer — split ──────────────────────── */
        first = e->e_logical;
        len   = e->e_len;

        {
            int free_slot = -1;

            for (i = 0u; i < (uint32_t)UNFS_INLINE_EXTENTS; i++) {
                if (i == (uint32_t)slot) continue;
                if (ip->i_extents[i].e_len == 0u) { free_slot = (int)i; break; }
            }
            if (free_slot < 0) return UNFS_ENOTSUP;        /* case 4 */

            /* the head keeps its position and shrinks to end at lb-1 */
            ip->i_extents[slot].e_len = (uint16_t)(lb - first);

            /* the tail is a new entry after the changed block */
            ip->i_extents[free_slot].e_logical  = lb + 1u;
            ip->i_extents[free_slot].e_physical = e->e_physical
                                                + (lb + 1u - first);
            ip->i_extents[free_slot].e_len      = (uint16_t)
                                                  (len - (lb - first) - 1u);
            ip->i_extents[free_slot].e_flags    = e->e_flags;
        }

        /* the changed block itself: reuse the slot when the head
         * collapsed to zero length, otherwise take a free one */
        if (ip->i_extents[slot].e_len == 0u) {
            ip->i_extents[slot].e_logical  = lb;
            ip->i_extents[slot].e_physical = p;
            ip->i_extents[slot].e_len      = 1u;
            ip->i_extents[slot].e_flags    = UNFS_EXT_LEAF;
            return UNFS_OK;
        }

        for (i = 0u; i < (uint32_t)UNFS_INLINE_EXTENTS; i++) {
            if (ip->i_extents[i].e_len != 0u) continue;
            ip->i_extents[i].e_logical  = lb;
            ip->i_extents[i].e_physical = p;
            ip->i_extents[i].e_len      = 1u;
            ip->i_extents[i].e_flags    = UNFS_EXT_LEAF;
            return UNFS_OK;
        }
        return UNFS_ENOTSUP;                               /* case 4 */
    }

    /* ── case 3: nothing covers lb — append ────────────────────────── */
    for (i = 0u; i < (uint32_t)UNFS_INLINE_EXTENTS; i++) {
        if (ip->i_extents[i].e_len != 0u) continue;
        ip->i_extents[i].e_logical  = lb;
        ip->i_extents[i].e_physical = p;
        ip->i_extents[i].e_len      = 1u;
        ip->i_extents[i].e_flags    = UNFS_EXT_LEAF;
        return UNFS_OK;
    }

    /* ── case 4: no inline room — the overflow tree ──────────────────
     * NOT IMPLEMENTED.  Reported rather than done badly. */
    return UNFS_ENOTSUP;
}

/* ═════════════════════════════════════════════════════════════
 * bmap_alloc — allocate a block and place it at byte_offset
 *
 * Replaces the stub that returned valid = false so the write path stopped
 * at ENOSPC.  Now it allocates through unfs_alloc_run() and places the
 * block into the inode's extent map.
 *
 * ── the device, and the uiox_ types ───────────────────────────────────
 * ip->dev is a uint8_t, which is what bcache takes — but unfs_alloc_run()
 * is declared with the uiox_ spellings:
 *
 *     int unfs_alloc_run(uiox_uint32_t dev, uiox_uint32_t want,
 *                        unfs_run_t *out);
 *
 * so the device is cast UP at that one call site.  The two spellings are
 * the same width on every target this builds for, and mixing them
 * silently is what produced the "conflicting types" rounds earlier, so
 * the cast is written out rather than left implicit.
 *
 * ── ordering, and the leak it accepts ─────────────────────────────────
 * The block is ALLOCATED first, then PLACED.  If the place fails — the
 * inline array is full and the tree is not written — the block has been
 * taken from the bitmap and nothing names it.  The alternative order
 * (search for a slot, then allocate) can still lose the race, because the
 * split in case 2 consumes a slot.  So the leak is real, and it is
 * REPORTED: the caller gets valid = false AND r.blkno holding the orphan,
 * so a reclaim pass can free it.  Reporting a valid mapping would be a
 * silent corruption of the file; hiding the number would leak the block
 * with no way to find it.
 * ═════════════════════════════════════════════════════════════ */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset)
{
    BmapResult r;
    unfs_run_t run;
    uint32_t   lb;
    int        rc;

    memset(&r, 0, sizeof r);

    if (!ip) return r;

    r.dev        = ip->dev;
    lb           = byte_offset / UNFS_BLOCK_SIZE;
    r.blk_offset = byte_offset % UNFS_BLOCK_SIZE;
    r.io_bytes   = UNFS_BLOCK_SIZE - r.blk_offset;

    /* already mapped?  then this is a read-shaped call, not an alloc */
    if (bmap_extent_find(ip, lb) >= 0) {
        return bmap(ip, byte_offset);
    }

    run.first_blk = 0u;
    run.count     = 0u;
    run.group     = 0u;

    rc = unfs_alloc_run((uiox_uint32_t)ip->dev, (uiox_uint32_t)1u,
                        (unfs_run_t *)&run);
    if (rc != UNFS_OK || run.count == 0u) {
        r.valid = false;                /* caller writes short, ENOSPC */
        return r;
    }

    if (bmap_extent_place(ip, lb, run.first_blk) != UNFS_OK) {
        /* allocated but unnamed — see the ordering note above */
        r.blkno = run.first_blk;
        r.valid = false;
        return r;
    }

    r.blkno = run.first_blk;
    r.valid = true;

    /* grow the file if this block extends it */
    if ((lb + 1u) * UNFS_BLOCK_SIZE > ip->size)
        ip->size = (lb + 1u) * UNFS_BLOCK_SIZE;

    return r;
}
