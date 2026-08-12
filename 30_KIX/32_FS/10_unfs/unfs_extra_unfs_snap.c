/*
 * 30_KIX/32_FS/10_unfs/unfs_snap.c
 *
 * UNFS Snapshot + Copy-on-Write management.
 *
 * How snapshots work in UNFS:
 *   1. unfs_snap_create() records the current root inode number
 *      and enables COW mode.
 *   2. All subsequent writes go through unfs_cow_block():
 *      - Read old block → allocate new block → copy content
 *      - Update inode extent to point at new block
 *      - Old block retained (still referenced by snapshot)
 *   3. Snapshot read: look up inode from snapshot's root_ino,
 *      walk extents using snapshot-time block numbers.
 *   4. unfs_snap_delete() marks old blocks for reclaim
 *      when no remaining snapshot references them.
 *
 * Journal integration:
 *   - Snapshot creation is journalled (metadata update to inode table)
 *   - COW block allocation is journalled (extent update)
 *   - Snapshot deletion triggers journal checkpoint + block reclaim
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "unfs_fs.h"
 #include "uiox_journal.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_snap_create
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_snap_create(unfs_fs_t *fs, const char *name)
 {
     if (!fs || !name) return -22;
     if (fs->n_snapshots >= UNFS_SNAP_MAX) return -28;
 
     /* Begin journal transaction for snapshot metadata */
     uiox_jr_handle_t *h = uiox_jr_start(1u);
     if (!h) return -12;
 
     unfs_snap_t *snap = &fs->snapshots[fs->n_snapshots];
     snap->snap_id    = (uint64_t)(fs->n_snapshots + 1u);
     snap->root_ino   = UNFS_ROOT_INO;
     snap->inuse      = 1u;
 
     /* TODO: read current time from 33_PCS/04_fboot timer */
     snap->created_ns = 0u;
 
     uint8_t i = 0u;
     while (name[i] && i < 31u) { snap->name[i] = name[i]; i++; }
     snap->name[i] = '\0';
 
     /* Enable COW mode — all subsequent writes copy before modifying */
     fs->cow_active        = 1u;
     fs->disk.s_cow_enabled = 1u;
 
     uiox_jr_dirty_metadata(h, UNFS_SB_BLOCK);
     uiox_jr_stop(h);
 
     fs->n_snapshots++;
     early_puts("[unfs] snapshot created: ");
     early_puts(snap->name);
     early_puts("\n");
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_snap_delete
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_snap_delete(unfs_fs_t *fs, uint64_t snap_id)
 {
     if (!fs) return -22;
 
     for (uint8_t i = 0u; i < fs->n_snapshots; i++) {
         unfs_snap_t *snap = &fs->snapshots[i];
         if (snap->snap_id != snap_id || !snap->inuse) continue;
 
         snap->inuse = 0u;
 
         /* If no more snapshots remain, disable COW */
         uint8_t any_snap = 0u;
         for (uint8_t j = 0u; j < fs->n_snapshots; j++)
             if (fs->snapshots[j].inuse) { any_snap = 1u; break; }
 
         if (!any_snap) {
             fs->cow_active = 0u;
             fs->disk.s_cow_enabled = 0u;
         }
 
         /* Trigger checkpoint to reclaim orphaned COW blocks */
         uiox_jr_checkpoint();
         return 0;
     }
     return -2;  /* ENOENT */
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_snap_list
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_snap_list(unfs_fs_t *fs, unfs_snap_t *out, uint32_t max)
 {
     if (!fs || !out) return -22;
     uint32_t n = 0u;
     for (uint8_t i = 0u; i < fs->n_snapshots && n < max; i++) {
         if (fs->snapshots[i].inuse) {
             memcpy(&out[n], &fs->snapshots[i], sizeof(unfs_snap_t));
             n++;
         }
     }
     return (int)n;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * unfs_cow_block — copy-on-write block duplication
  *
  * Called before any write to an existing block when COW is active.
  * Returns new physical block number, or 0 on allocation failure.
  *
  * The caller must update the inode extent to point to new_phys
  * and journal both the old and new block metadata.
  * ───────────────────────────────────────────────────────────────────── */
 uint32_t unfs_cow_block(unfs_fs_t *fs, uint32_t old_phys)
 {
     if (!fs || !fs->cow_active) return old_phys;
     if (old_phys == 0u) return 0u;
 
     uint32_t new_phys = unfs_alloc_block(fs);
     if (new_phys == 0u) return 0u;
 
     /* Copy content from old block to new block via block cache */
     static uint8_t cow_buf[4096];
     extern void blk_read (uint32_t blkno, void *buf);
     extern void blk_write(uint32_t blkno, const void *buf);
     blk_read (old_phys, cow_buf);
     blk_write(new_phys, cow_buf);
 
     return new_phys;
 }
 