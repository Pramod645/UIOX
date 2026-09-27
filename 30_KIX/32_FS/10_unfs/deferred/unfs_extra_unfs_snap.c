/*
 * 30_KIX/32_FS/10_unfs/unfs_extra_unfs_snap.c
 *
 * UNFS Snapshot + Copy-on-Write management.
 *
 * How snapshots work in UNFS:
 *   1. unfs_snap_create() records the current root inode number
 *      and enables COW mode.
 *   2. All subsequent writes go through unfs_cow_block():
 *      - Read old block, allocate new block, copy content
 *      - Update inode extent to point at new block
 *      - Old block retained (still referenced by snapshot)
 *   3. Snapshot read: look up inode from snapshot's root_ino,
 *      walk extents using snapshot-time block numbers.
 *   4. unfs_snap_delete() marks old blocks for reclaim
 *      when no remaining snapshot references them.
 *
 * -- NO JOURNAL IN THIS FILE ------------------------------------------
 * The journal is a separate concern (02_jrnal) and is not wired in.
 * This file therefore makes NO journalled metadata updates: every
 * change below is written in place, and a crash mid-operation can
 * leave the in-memory snapshot table and the on-disk superblock out
 * of step.  That is the known cost of running without a journal.
 *
 * The one place the journal used to cover is marked
 * UNSAFE-WITHOUT-JOURNAL so it is easy to find when a journal arrives:
 *
 *   unfs_snap_delete()  block reclamation is deferred, not performed
 *
 * -- WHAT CHANGED IN v1.2.0 -------------------------------------------
 *
 * 1. THE INCLUDE IS LIVE.  It read
 *
 *        //#include "unfs_extra_unfs_snap.h"
 *
 *    commented out, so nothing declared unfs_fs_t or unfs_snap_t and
 *    every function signature failed with "unknown type name".  The
 *    declaration now comes from unfs_fs.h — the snapshot API was folded
 *    into that header, and unfs_extra_unfs_snap.h is retired.
 *
 * 2. THE s_cow_enabled WRITES ARE GONE.  This file used to set
 *    fs->disk.s_cow_enabled alongside fs->cow_active.  That field was
 *    removed from unfs_sb_t: COW state is per-mount, not on-disk, so it
 *    lives in unfs_fs_t as cow_active only.  Nothing is lost — the flag
 *    steers unfs_cow_block and a fresh mount correctly starts with COW
 *    off.
 *
 * 3. mem_zero() IS NOT DEFINED HERE.  An earlier revision called it
 *    without a declaration, which is an implicit-declaration error under
 *    -Werror.  The snapshot struct is small enough to clear explicitly.
 *
 * -- STILL MISSING, AND THIS FILE WILL NOT LINK WITHOUT IT -------------
 *  unfs_alloc_block() is declared in unfs_fs.h and has NO BODY anywhere
 *  in the tree.  unfs_cow_block() calls it, so this translation unit
 *  compiles but the link fails on that one symbol.  The body is an
 *  allocator over the group bitmaps unfs_format.c lays down.
 *
 *  To integrate the base UNFS first, move this file out of
 *  10_unfs/src/ — the Makefile globs $(UNFS_DIR)/src/*.c, so nothing
 *  else needs editing.
 *
 * @version 1.2.0  @date 2026-09-27
 */

 #include "unfs_fs.h"            /* unfs_fs_t, unfs_snap_t, UNFS_SNAP_MAX,
 * and the snapshot API.  Declares
 * unfs_alloc_block / unfs_cow_block too. */

#include "uiox_soc_string.h"    /* uiox_strlen                       */
#include "uiox_soc_stdio.h"     /* early_puts                        */
#include "bcache.h"             /* bread / bwrite / brelse / bdwrite  */

/* Errors: the uiox_ names, not bare negatives. */
#ifndef UIOX_OK
#define UIOX_OK        0
#endif
#ifndef UIOX_EINVAL
#define UIOX_EINVAL  (-22)
#endif
#ifndef UIOX_ENOSPC
#define UIOX_ENOSPC  (-28)
#endif
#ifndef UIOX_ENOENT
#define UIOX_ENOENT  (-2)
#endif

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_create
*
* Records the current root inode and turns COW on.
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_create(unfs_fs_t *fs, const char *name)
{
uint32_t     n;
uint32_t     k;
unfs_snap_t *snap;

if (!fs || !name) return UIOX_EINVAL;
if (fs->n_snapshots >= UNFS_SNAP_MAX) return UIOX_ENOSPC;

snap = &fs->snapshots[fs->n_snapshots];

/* Clear the slot field by field.  mem_zero() is not declared in this
* translation unit and the struct is small. */
snap->snap_id    = 0u;
snap->created_ns = 0u;
snap->root_ino   = 0u;
snap->inuse      = 0u;
for (k = 0u; k < (uint32_t)UNFS_SNAP_NAME_MAX; k++) snap->name[k] = '\0';

snap->snap_id    = (uint64_t)(fs->n_snapshots + 1u);
snap->root_ino   = UNFS_ROOT_INO;
snap->inuse      = 1u;

/* No clock available in this layer; 33_PCS owns the timer and this
* field is informational only.  Left zero rather than fabricated. */
snap->created_ns = 0u;

/* Bounded copy: name is at most UNFS_SNAP_NAME_MAX-1 characters and
* always NUL-terminated. */
n = (uint32_t)uiox_strlen(name);
if (n >= (uint32_t)UNFS_SNAP_NAME_MAX) n = (uint32_t)UNFS_SNAP_NAME_MAX - 1u;
for (k = 0u; k < n; k++) snap->name[k] = name[k];
snap->name[n] = '\0';

/* Enable COW.  In memory only — see the v1.2.0 note above. */
fs->cow_active = 1u;

fs->n_snapshots++;

early_puts("[unfs] snapshot created: ");
early_puts(snap->name);
early_puts("\n");

return UIOX_OK;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_delete
*
* Clears the snapshot.  When the last one goes, COW is turned off.
*
* UNSAFE-WITHOUT-JOURNAL: the reclaim of orphaned COW blocks used to be
* a journal checkpoint.  There is no journal, so the blocks are NOT
* reclaimed — they stay marked used.  unfs_free_block() is declared in
* unfs_fs.h and has no implementation yet, so calling it here would be a
* link error.  The count left behind is reported through *reclaimed_out;
* pass NULL if you do not need it.
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_delete(unfs_fs_t *fs, uint64_t snap_id,
uint32_t *reclaimed_out)
{
uint8_t i;
uint8_t any_snap;

if (!fs) return UIOX_EINVAL;

for (i = 0u; i < fs->n_snapshots; i++) {
unfs_snap_t *snap = &fs->snapshots[i];

if (snap->snap_id != snap_id || !snap->inuse) continue;

snap->inuse = 0u;

/* Any snapshot still live? */
any_snap = 0u;
for (uint8_t j = 0u; j < fs->n_snapshots; j++) {
if (fs->snapshots[j].inuse) { any_snap = 1u; break; }
}

if (!any_snap) fs->cow_active = 0u;

/* Deferred: no allocator yet.  Reported, not performed. */
if (reclaimed_out) *reclaimed_out = 0u;

return UIOX_OK;
}

return UIOX_ENOENT;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_snap_list
*
* Copies live snapshots out, oldest first.  Returns the count, or a
* negative error.
* ───────────────────────────────────────────────────────────────────── */
int unfs_snap_list(unfs_fs_t *fs, unfs_snap_t *out, uint32_t max)
{
uint32_t n = 0u;
uint8_t  i;

if (!fs || !out) return UIOX_EINVAL;

for (i = 0u; i < fs->n_snapshots && n < max; i++) {
if (!fs->snapshots[i].inuse) continue;

memcpy(&out[n], &fs->snapshots[i], sizeof(unfs_snap_t));
n++;
}

return (int)n;
}

/* ─────────────────────────────────────────────────────────────────────
* unfs_cow_block — copy-on-write block duplication
*
* Called before a write to an existing block while COW is active.
* Returns the new physical block number, old_phys when COW is off, or
* 0 on allocation failure.
*
* NO JOURNAL: with a journal the caller recorded both the old and the
* new block in one transaction.  Without one, the copy and the extent
* update are two separate writes, and a crash between them leaves the
* old block intact — wasteful, not corrupting, which is why COW is
* usable without a journal at all.
*
* The read/write pair is bcache's.  An earlier revision declared
*     extern void blk_read (uint32_t, void *);
*     extern void blk_write(uint32_t, const void *);
* and neither symbol exists anywhere in the tree.
* ───────────────────────────────────────────────────────────────────── */
uint32_t unfs_cow_block(unfs_fs_t *fs, uint32_t old_phys)
{
uint32_t new_phys;
BufHdr  *bp;

if (!fs || !fs->cow_active) return old_phys;
if (old_phys == 0u) return 0u;

new_phys = unfs_alloc_block(fs);
if (new_phys == 0u) return 0u;

/* Copy through the block cache: bread() may satisfy the read from a
* cached buffer, so this is not an extra device round-trip in the
* common case.  One buffer, borrowed and returned. */
bp = bread(fs->dev_id, old_phys);
if (!bp) {
/* Allocation stands; the copy did not happen.  Freeing it is the
* caller's call, since unfs_free_block is not implemented. */
return 0u;
}

if (bwrite(fs->dev_id, new_phys, bp->data) != UIOX_OK) {
brelse(bp);
return 0u;
}

brelse(bp);
return new_phys;
}

/* ─────────────────────────────────────────────────────────────────────
* END OF FILE
*
* What is deliberately NOT here, and why:
*
*   uiox_journal.h        removed — no journal is wired in.
*   uiox_jr_* calls       removed — same.
*   UNFS_SB_BLOCK         removed — that name does not exist in
*                         unfs_format.h (the superblock is block 0,
*                         reached through UNFS_SB_OFFSET).
*   s_cow_enabled         removed — the field left unfs_sb_t; COW state
*                         is per-mount and lives in fs->cow_active.
*   journalled writes     replaced by in-place writes, marked above.
*   block reclamation     deferred until an allocator exists.
*
* unfs_alloc_block() is declared in unfs_fs.h and has NO body, so this
* file compiles but will not link until that allocator exists.  The body
* belongs with the port: a bit-scan over the group block bitmap that
* unfs_format.c lays down, clearing the bit and decrementing
* s_free_blocks.
* ───────────────────────────────────────────────────────────────────── */
