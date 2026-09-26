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
 *  ── what the signature keeps ──────────────────────────────────────────
 *  Bach's algorithm is "map a file offset to a block number".  The
 *  MECHANISM is an implementation detail, so the interface is unchanged:
 *
 *      BmapResult bmap(InCoreInode *ip, uint32_t byte_offset);
 *
 *  01_fsa's readwrite.c, namei.c, superblock.c and truncate.c all call it
 *  as written and need no edit.  Only what happens inside changed.
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
 *  conversion is UNFS_SECTORS_PER_BLOCK (8).  It is applied at the
 *  buffer boundary, not here, so this function stays in UNFS units.
 *
 *  ── what this file cannot do yet ──────────────────────────────────────
 *  bmap_alloc is NOT rewritten.  Allocating into an extent tree means
 *  finding a free run, merging with neighbours where they are contiguous,
 *  and splitting when they are not — materially more work than filling an
 *  addr[] slot, and it belongs with 10_unfs's bitmap.  It is reported as
 *  unimplemented rather than left in its old addr[] form, which would
 *  write block numbers into a struct that no longer has that field.
 *
 *  @version 3.0.0  @date 2026-09-26
 */
#include "bmap.h"
#include "inode.h"
#include "buffer.h"
#include "uiox_klibc.h"

/*
 * The overflow extent-tree block is an array of unfs_extent_t.  Its
 * capacity follows from the 4096-byte block and the 8-byte entry.
 */
#define EXTREE_PER_BLOCK ((uint32_t)(UNFS_BLOCK_SIZE / sizeof(unfs_extent_t)))

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
 * The tree block is one 4096-byte block holding an array of extents,
 * terminated by an entry with e_len == 0.  A single level, as the
 * bootloader's extent_lookup() assumes — so the two agree.
 * ───────────────────────────────────────────────────────────── */
static uint32_t extree_lookup(uint8_t dev, uint32_t tree_blk,
                              uint32_t lb)
{
    BufHdr      *buf;
    const unfs_extent_t *et;
    uint32_t     i;

    /* the tree block is one UNFS block = 8 buffer sectors; read the
     * first sector and treat the buffer's data area as the block */
    buf = bread(dev, tree_blk * UNFS_SECTORS_PER_BLOCK);
    if (!buf) return 0u;

    et = (const unfs_extent_t *)buf->data;

    for (i = 0u; i < EXTREE_PER_BLOCK; i++) {
        if (et[i].e_len == 0u) break;              /* end of entries */
        if (extent_covers(&et[i], lb)) {
            uint32_t p = extent_phys(&et[i], lb);
            brelse(buf);
            return p;
        }
    }

    brelse(buf);
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
    for (i = 0u; i < 4u; i++) {
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
        uint32_t p = extree_lookup(ip->dev,
                                   ip->i_extent_tree, lb);
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
 * bmap_alloc — NOT implemented for extents
 *
 * Allocating into an extent tree is: find a free run of blocks, decide
 * whether it merges with an adjacent extent's physical range, extend that
 * extent or append a new one, and spill to the tree block when the four
 * inline slots are full.  That work needs 10_unfs's block bitmap, which
 * does not exist yet.
 *
 * The previous addr[]-based version is REMOVED rather than kept: it wrote
 * block numbers into InCoreInode.addr[], a field this format does not
 * have, so keeping it would be a silent corruption rather than a failure.
 *
 * This returns an invalid mapping, which makes the write path stop and
 * report ENOSPC — the honest outcome until allocation is written.
 * ═════════════════════════════════════════════════════════════ */
BmapResult bmap_alloc(InCoreInode *ip, uint32_t byte_offset)
{
    BmapResult r;

    memset(&r, 0, sizeof r);

    if (ip) r.dev = ip->dev;
    r.blk_offset = byte_offset % UNFS_BLOCK_SIZE;
    r.io_bytes   = UNFS_BLOCK_SIZE - r.blk_offset;
    r.valid      = false;      /* caller writes short, then ENOSPC */

    return r;
}
