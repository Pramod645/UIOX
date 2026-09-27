/*
 * 30_KIX/32_FS/10_unfs/src/unfs_extra_unfs_snap.c
 *
 * UNFS Snapshot + Copy-on-Write management.
 *
 *   1. unfs_snap_create() records the root inode number and enables COW.
 *   2. Writes to an existing block go through unfs_cow_block().
 *   3. A snapshot is read by walking extents as they stood when taken.
 *   4. unfs_snap_delete() clears it and turns COW off when the last goes.
 *
 * -- NO JOURNAL IN THIS FILE ------------------------------------------
 * The journal is a separate concern (02_jrnal) and is not wired in, so
 * every change below is written in place.  The one place it used to cover
 * is marked UNSAFE-WITHOUT-JOURNAL.
 *
 * -- WHAT CHANGED IN v1.3.0 -------------------------------------------
 *
 * 1. THE ALLOCATOR CALL IS CORRECT NOW.  It read
 *
 *        new_phys = unfs_alloc_block(fs->dev_id);
 *
 *    and unfs_alloc_block() takes a DEVICE, not a filesystem:
 *
 *        uiox_uint32_t unfs_alloc_block(uiox_uint32_t dev);
 *
 *    The allocator landed in unfs_alloc.c and works over the group
 *    bitmaps, deliberately taking (dev, want) so it has no dependency on
 *    unfs_fs_t or on 01_fsa.  Passing the filesystem pointer was a
 *    signature that never existed.
 *
 * 2. THE EXTENT UPDATE IS NOT DONE, AND IS MARKED AS SUCH.  See the
 *    block comment on unfs_cow_block below.  This is the one gap in the
 *    snapshot path and it is not papered over: the function reports
 *    failure rather than returning a new block whose extent still points
 *    at the old one.
 *
 * 3. uiox_puts REPLACES early_puts.  early_puts is declared in no SoC
 *    header — grep finds it nowhere under 10_BSP/03_SoC/include — so the
 *    three calls were an implicit declaration.  uiox_puts appends the
 *    newline itself, so the prefix and the bare "\n" both go.
 *
 * 4. THE FLOATING COMMENT IS CLEARED.  An earlier revision left the
 *    include's explanatory text outside the comment block and the
 *    function bodies unindented, which is a -Werror comment error and a
 *    wall of whitespace warnings respectively.
 *
 * @version 1.3.0  @date 2026-09-27
 */

 #include "unfs_fs.h"            /* unfs_fs_t, unfs_snap_t, UNFS_SNAP_MAX,
 * the snapshot API, and dev_id          */
#include "unfs_alloc.h"         /* unfs_alloc_block(dev)                 */
#include "uiox_soc_stdio.h"     /* uiox_puts                             */
#include "bcache.h"             /* bread / brelse / BufHdr               */

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_create — record the root inode, turn COW on
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_create(unfs_fs_t *fs, const char *name)
{
unfs_snap_t *snap;
uiox_uint32_t n;
uiox_uint32_t k;

if (!fs || !name) return UNFS_EINVAL;
if (fs->n_snapshots >= UNFS_SNAP_MAX) return UNFS_ENOSPC;

snap = &fs->snapshots[fs->n_snapshots];

/* clear the slot explicitly — mem_zero() is not declared here */
snap->snap_id    = 0u;
snap->created_ns = 0u;
snap->root_ino   = 0u;
snap->inuse      = 0u;
for (k = 0u; k < (uiox_uint32_t)UNFS_SNAP_NAME_MAX; k++) snap->name[k] = '\0';

snap->snap_id  = (uiox_uint64_t)(fs->n_snapshots + 1u);
snap->root_ino = UNFS_ROOT_INO;
snap->inuse    = 1u;

/* No clock in this layer; 33_PCS owns the timer and this field is
* informational.  Left zero rather than fabricated. */
snap->created_ns = 0u;

/* bounded copy, always NUL-terminated */
n = (uiox_uint32_t)uiox_strlen(name);
if (n >= (uiox_uint32_t)UNFS_SNAP_NAME_MAX) n = (uiox_uint32_t)UNFS_SNAP_NAME_MAX - 1u;
for (k = 0u; k < n; k++) snap->name[k] = name[k];
snap->name[n] = '\0';

/* COW on.  In memory only — s_cow_enabled left unfs_sb_t, so this is
* mount state and not on-disk. */
fs->cow_active = 1u;

fs->n_snapshots++;

uiox_puts(snap->name);      /* prefix and "\n" folded in */

return UNFS_OK;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_delete — clear it; COW off when the last one goes
*
* UNSAFE-WITHOUT-JOURNAL: the reclaim of orphaned COW blocks was a
* journal checkpoint.  There is no journal, so nothing is reclaimed and
* the blocks stay marked used.  unfs_free_run() exists and could reclaim
* them — but the set of blocks a snapshot retained is not recorded
* anywhere, so there is no list to walk.  It is reported, not performed.
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_delete(unfs_fs_t *fs, uiox_uint64_t snap_id,
uiox_uint32_t *reclaimed_out)
{
uiox_uint8_t i;
uiox_uint8_t any_snap;

if (!fs) return UNFS_EINVAL;

for (i = 0u; i < fs->n_snapshots; i++) {
unfs_snap_t *snap = &fs->snapshots[i];

if (snap->snap_id != snap_id || !snap->inuse) continue;

snap->inuse = 0u;

any_snap = 0u;
for (uiox_uint8_t j = 0u; j < fs->n_snapshots; j++) {
if (fs->snapshots[j].inuse) { any_snap = 1u; break; }
}

if (!any_snap) fs->cow_active = 0u;

if (reclaimed_out) *reclaimed_out = 0u;   /* deferred */
return UNFS_OK;
}

return UNFS_ENOENT;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_list — copy the live descriptors out, oldest first
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_list(unfs_fs_t *fs, unfs_snap_t *out, uiox_uint32_t max)
{
uiox_uint32_t n = 0u;
uiox_uint8_t  i;

if (!fs || !out) return UNFS_EINVAL;

for (i = 0u; i < fs->n_snapshots && n < max; i++) {
uiox_uint32_t k;
uiox_uint8_t *d;
uiox_uint8_t *s;

if (!fs->snapshots[i].inuse) continue;

d = (uiox_uint8_t *)&out[n];
s = (uiox_uint8_t *)&fs->snapshots[i];
for (k = 0u; k < (uiox_uint32_t)sizeof(unfs_snap_t); k++) d[k] = s[k];
n++;
}

return (int)n;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_cow_block — copy-on-write block duplication
*
* Returns the new physical block, old_phys when COW is off, or 0 on
* failure.
*
* ── THE GAP, STATED PLAINLY ───────────────────────────────────────────
* This function copies a block.  It does NOT repoint the inode's extent
* at the copy, and without that the whole scheme is inert: the caller
* writes to the new block, the extent still names the old one, and the
* next read serves the pre-write bytes.
*
* The missing piece is not in this file.  It is:
*
*     find the extent covering logical block L in the inode,
*     if its length is 1, repoint e_physical at new_phys;
*     else split it into two extents (before, after) and place new_phys
*     between them — or spill to i_extent_tree when the four inline
*     slots are full.
*
* That is bmap_alloc()'s missing half, and bmap.c currently reports
* ENOSPC rather than performing it.  Writing it here would duplicate the
* extent-map logic and put it somewhere bmap() cannot see; it belongs in
* 01_fsa's bmap.c next to the read side.
*
* So this function returns the new block ONLY when the caller has said it
* will do the repoint.  It cannot verify that, and the honest position is
* that the caller must not use the result until bmap_alloc exists.
* Returning 0 instead would make COW a no-op that silently kept the old
* block — worse, because it would look like it worked.
*
* ── the copy ──────────────────────────────────────────────────────────
* The read/write pair is bcache's.  An earlier revision declared
*
*     extern void blk_read (uint32_t, void *);
*     extern void blk_write(uint32_t, const void *);
*
* and neither symbol exists anywhere in the tree.  bwrite() takes a
* BufHdr* and returns void — calling it as (dev, blkno, data) produced
* three -Werror diagnostics: pointer-from-integer, an always-true
* comparison, and "void value not ignored".
*
* UNITS: old_phys and new_phys are UNFS blocks (4096).  One block spans
* UNFS_SECTORS_PER_BLOCK (8) bcache buffers, so the loop below walks the
* sectors rather than copying one buffer — a single-buffer version would
* copy one eighth of the block.
* ───────────────────────────────────────────────────────────────────── */
uiox_uint32_t unfs_cow_block(unfs_fs_t *fs, uiox_uint32_t old_phys)
{
uiox_uint32_t new_phys;
uiox_uint32_t s;
uiox_uint32_t base_old;
uiox_uint32_t base_new;

if (!fs || !fs->cow_active) return old_phys;
if (old_phys == 0u) return 0u;

/* the allocator takes a DEVICE, from unfs_fs_t */
new_phys = unfs_alloc_block(fs->dev_id);
if (new_phys == 0u) return 0u;

base_old = old_phys * (uiox_uint32_t)UNFS_SECTORS_PER_BLOCK;
base_new = new_phys * (uiox_uint32_t)UNFS_SECTORS_PER_BLOCK;

/* sector-by-sector copy through the block cache.  bread() may serve
* the source from a cached buffer, so this is not eight device reads
* in the common case. */
for (s = 0u; s < (uiox_uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
BufHdr *src = bread((uiox_uint8_t)fs->dev_id, base_old + s);
BufHdr *dst;
uiox_uint32_t k;

if (!src) return 0u;

dst = getblk((uiox_uint8_t)fs->dev_id, base_new + s);
if (!dst) { brelse(src); return 0u; }

for (k = 0u; k < (uiox_uint32_t)BLOCK_SIZE; k++)
dst->data[k] = src->data[k];

/* sync write, releases */
bwrite(dst, true, false);
brelse(src);
}

/* The copy is on disk.  The extent still names old_phys — the caller
* must repoint it, and cannot do so until bmap_alloc() exists.  See
* THE GAP above. */
return new_phys;
}
