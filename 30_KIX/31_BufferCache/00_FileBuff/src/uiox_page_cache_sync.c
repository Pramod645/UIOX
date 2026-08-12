/*
 * 31_BufferCache/00_FileBuff/src/uiox_page_cache_sync.c
 *
 * Connects the page cache writeback to the syscall layer.
 *
 * Called from:
 *   SYS_SYNC    → uiox_pc_sync_all()
 *   SYS_FSYNC   → uiox_pc_sync_inode(ino)
 *   SYS_FDATASYNC → uiox_pc_sync_inode(ino)  (data only — same here)
 *   SYS_SYNCFS  → uiox_pc_sync_all()
 *   uiox_jr_tick() → uiox_pc_sync_all() on interval
 *
 * Ordering guarantee:
 *   1. uiox_pc_writeback_all()  — dirty pages → block device via bwrite()
 *   2. uiox_jr_force_commit()   — journal commit block → device
 *   3. uiox_jr_checkpoint()     — advance journal head, mark clean
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_page_cache.h"
 #include "bcache.h"
 #include "uiox_soc_stdio.h"
 
 /* Forward from 32_FS/02_journal */
 extern int uiox_jr_force_commit(void);
 extern int uiox_jr_checkpoint(void);
 
 /*
  * uiox_pc_sync_all — full filesystem sync.
  * Flushes all dirty pages + journal commit + checkpoint.
  * Called by SYS_SYNC / SYS_SYNCFS.
  */
 int uiox_pc_sync_all(void)
 {
     /* Step 1 — writeback all dirty pages to block device */
     int rc = uiox_pc_writeback_all();
 
     /* Step 2 — commit journal (makes writes durable) */
     uiox_jr_force_commit();
 
     /* Step 3 — checkpoint (advance journal head, reclaim space) */
     uiox_jr_checkpoint();
 
     return rc;
 }
 
 /*
  * uiox_pc_sync_inode — sync one inode's dirty pages.
  * Called by SYS_FSYNC / SYS_FDATASYNC.
  */
 int uiox_pc_sync_inode(uint32_t ino)
 {
     int rc = uiox_pc_writeback(ino);
     uiox_jr_force_commit();
     return rc;
 }
 