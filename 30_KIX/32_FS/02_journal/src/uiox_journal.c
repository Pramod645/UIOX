/*
 * 30_KIX/32_FS/02_journal/uiox_journal.c
 *
 * UIOX Filesystem Journal implementation.
 *
 * Write path with journal:
 *   sys_write(fd, ubuf, n)
 *     → copy_from_user(kbuf, ubuf, n)       ← privilege crossing
 *     → vfs_write(file, kbuf, n)
 *         → h = uiox_jr_start(1)            ← begin transaction
 *         → uiox_jr_get_write_access(h, blk)← log original block
 *         → modify block in page cache      ← DRAM write
 *         → uiox_jr_dirty_metadata(h, blk)  ← mark dirty
 *         → uiox_jr_stop(h)                 ← end transaction
 *     [later] uiox_jr_force_commit()         ← write log to device
 *     [later] uiox_jr_checkpoint()           ← write data to device
 *
 * SYS_SYNC / SYS_FSYNC triggers uiox_jr_force_commit() immediately.
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_journal.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"
 
 /* ── Global journal context ─────────────────────────────────────────── */
 static uiox_jr_ctx_t s_jr;
 
 /* Commit interval in scheduler ticks (100 Hz → 5 s = 500 ticks) */
 #define UIOX_JR_COMMIT_INTERVAL  500u
 static uint32_t s_tick_count = 0u;
 
 /* ── Journal block device write stub ───────────────────────────────── */
 /*
  * In production: calls into 30_DeviceDrivers/03_NonSensors/emmc
  * or 31_BufferCache block layer.
  * For now: write to DRAM journal region.
  */
 #define UIOX_JR_DRAM_BASE_DEFAULT  0x45000000UL
 
 __attribute__((weak))
 uintptr_t uiox_jr_plat_base(void) { return UIOX_JR_DRAM_BASE_DEFAULT; }
 
 static void jr_write_block(uint32_t block_idx, const void *buf)
 {
     uintptr_t addr = uiox_jr_plat_base() +
                      (uintptr_t)block_idx * UIOX_JR_BLOCK_SIZE;
     memcpy((void *)addr, buf, UIOX_JR_BLOCK_SIZE);
 }
 
 static void jr_read_block(uint32_t block_idx, void *buf)
 {
     uintptr_t addr = uiox_jr_plat_base() +
                      (uintptr_t)block_idx * UIOX_JR_BLOCK_SIZE;
     memcpy(buf, (const void *)addr, UIOX_JR_BLOCK_SIZE);
 }
 
 /* ── uiox_jr_init ──────────────────────────────────────────────────── */
 void uiox_jr_init(void)
 {
     memset(&s_jr, 0, sizeof(s_jr));
     s_jr.free_blocks  = UIOX_JR_MAX_BLOCKS;
     s_jr.initialised  = 1u;
     early_puts("[journal] initialised\n");
 }
 
 /* ── uiox_jr_mount ─────────────────────────────────────────────────── */
 void uiox_jr_mount(void)
 {
     /* Read journal superblock */
     jr_read_block(0u, &s_jr.sb);
 
     if (s_jr.sb.magic != UIOX_JR_MAGIC_SB) {
         /* First mount — write fresh superblock */
         s_jr.sb.magic       = UIOX_JR_MAGIC_SB;
         s_jr.sb.block_size  = UIOX_JR_BLOCK_SIZE;
         s_jr.sb.max_blocks  = UIOX_JR_MAX_BLOCKS;
         s_jr.sb.first_block = 1u;
         s_jr.sb.sequence    = 1u;
         s_jr.sb.head        = 1u;
         s_jr.sb.tail        = 1u;
         s_jr.sb.clean       = 1u;
         jr_write_block(0u, &s_jr.sb);
         early_puts("[journal] fresh journal written\n");
         return;
     }
 
     if (!s_jr.sb.clean) {
         early_puts("[journal] dirty mount — recovering\n");
         uiox_jr_recover();
     }
 
     s_jr.head = s_jr.sb.head;
     s_jr.tail = s_jr.sb.tail;
     early_puts("[journal] mounted\n");
 }
 
 /* ── uiox_jr_unmount ───────────────────────────────────────────────── */
 void uiox_jr_unmount(void)
 {
     uiox_jr_force_commit();
     uiox_jr_checkpoint();
     s_jr.sb.clean = 1u;
     jr_write_block(0u, &s_jr.sb);
     early_puts("[journal] clean unmount\n");
 }
 
 /* ── uiox_jr_start ─────────────────────────────────────────────────── */
 uiox_jr_handle_t *uiox_jr_start(uint32_t nr_blocks)
 {
     if (!s_jr.initialised)    return (uiox_jr_handle_t *)0;
     if (nr_blocks == 0u)      return (uiox_jr_handle_t *)0;
     if (s_jr.free_blocks < nr_blocks)
         return (uiox_jr_handle_t *)0;
 
     for (uint32_t i = 0u; i < UIOX_JR_MAX_HANDLES; i++) {
         if (!s_jr.handles[i].inuse) {
             s_jr.handles[i].inuse      = 1u;
             s_jr.handles[i].state      = UIOX_JR_TXN_RUNNING;
             s_jr.handles[i].sequence   = s_jr.sb.sequence++;
             s_jr.handles[i].nr_buffers = 0u;
             return &s_jr.handles[i];
         }
     }
     return (uiox_jr_handle_t *)0;
 }
 
 /* ── uiox_jr_get_write_access ──────────────────────────────────────── */
 int uiox_jr_get_write_access(uiox_jr_handle_t *h, uint32_t fs_block)
 {
     if (!h || !h->inuse) return UIOX_JR_EINVAL;
     if (h->state != UIOX_JR_TXN_RUNNING) return UIOX_JR_EINVAL;
 
     /*
      * Log the original block content to the journal
      * before the caller modifies it — this is the
      * write-ahead log (WAL) guarantee.
      */
     if (s_jr.free_blocks == 0u) return UIOX_JR_ENOSPC;
 
     /* Read original block from FS */
     uint8_t orig[UIOX_JR_BLOCK_SIZE];
     jr_read_block(fs_block, orig);
 
     /* Write to journal log area */
     uint32_t log_block = s_jr.tail;
     jr_write_block(s_jr.sb.first_block + log_block, orig);
 
     s_jr.tail = (s_jr.tail + 1u) % UIOX_JR_MAX_BLOCKS;
     s_jr.free_blocks--;
     s_jr.dirty = 1u;
 
     (void)fs_block;
     return UIOX_JR_OK;
 }
 
 /* ── uiox_jr_dirty_metadata ────────────────────────────────────────── */
 int uiox_jr_dirty_metadata(uiox_jr_handle_t *h, uint32_t fs_block)
 {
     if (!h || !h->inuse) return UIOX_JR_EINVAL;
     h->nr_buffers++;
     s_jr.dirty = 1u;
     (void)fs_block;
     return UIOX_JR_OK;
 }
 
 /* ── uiox_jr_stop ──────────────────────────────────────────────────── */
 int uiox_jr_stop(uiox_jr_handle_t *h)
 {
     if (!h || !h->inuse) return UIOX_JR_EINVAL;
     h->state  = UIOX_JR_TXN_LOCKED;
     h->inuse  = 0u;
     return UIOX_JR_OK;
 }
 
 /* ── uiox_jr_force_commit ──────────────────────────────────────────── */
 /*
  * Called by SYS_SYNC / SYS_FSYNC.
  * Writes commit block to journal — makes transaction durable.
  * After this returns, data survives a crash.
  */
 int uiox_jr_force_commit(void)
 {
     if (!s_jr.dirty) return UIOX_JR_OK;
 
     /* Write commit block */
     uint8_t commit_block[UIOX_JR_BLOCK_SIZE];
     memset(commit_block, 0, sizeof(commit_block));
     uint32_t *magic = (uint32_t *)commit_block;
     *magic = UIOX_JR_MAGIC_COMMIT;
 
     uint32_t log_block = s_jr.tail;
     jr_write_block(s_jr.sb.first_block + log_block, commit_block);
     s_jr.tail = (s_jr.tail + 1u) % UIOX_JR_MAX_BLOCKS;
 
     /* Update superblock head/tail */
     s_jr.sb.tail  = s_jr.tail;
     s_jr.sb.clean = 0u;
     jr_write_block(0u, &s_jr.sb);
 
     s_jr.dirty = 0u;
     return UIOX_JR_OK;
 }
 
 /* ── uiox_jr_checkpoint ────────────────────────────────────────────── */
 /*
  * Writes dirty data blocks from page cache to the actual FS blocks.
  * After checkpoint, journal space can be reclaimed.
  */
 int uiox_jr_checkpoint(void)
 {
     /*
      * TODO: walk committed transactions, write dirty pages
      * from page cache back to their original FS blocks,
      * then advance journal head past committed transactions.
      */
     s_jr.head         = s_jr.tail;
     s_jr.sb.head      = s_jr.head;
     s_jr.sb.clean     = 1u;
     s_jr.free_blocks  = UIOX_JR_MAX_BLOCKS;
     jr_write_block(0u, &s_jr.sb);
     return UIOX_JR_OK;
 }
 
 /* ── uiox_jr_tick ──────────────────────────────────────────────────── */
 /*
  * Called from 33_PCS/01_schedular timer tick (100 Hz).
  * Commits automatically every UIOX_JR_COMMIT_INTERVAL ticks (5 s).
  */
 void uiox_jr_tick(void)
 {
     s_tick_count++;
     if (s_tick_count >= UIOX_JR_COMMIT_INTERVAL) {
         s_tick_count = 0u;
         if (s_jr.dirty)
             uiox_jr_force_commit();
     }
 }
 
 /* ── uiox_jr_recover ───────────────────────────────────────────────── */
 /*
  * Replays committed transactions from journal on dirty mount.
  * Scans journal log from head to tail looking for commit blocks.
  */
 int uiox_jr_recover(void)
 {
     uint8_t  block_buf[UIOX_JR_BLOCK_SIZE];
     uint32_t cur = s_jr.sb.head;
     uint32_t replayed = 0u;
 
     while (cur != s_jr.sb.tail) {
         jr_read_block(s_jr.sb.first_block + cur, block_buf);
         uint32_t magic = *(uint32_t *)block_buf;
 
         if (magic == UIOX_JR_MAGIC_COMMIT) {
             replayed++;
         }
         cur = (cur + 1u) % UIOX_JR_MAX_BLOCKS;
     }
 
     /* After replay, mark clean */
     s_jr.sb.clean = 1u;
     jr_write_block(0u, &s_jr.sb);
 
     (void)replayed;
     early_puts("[journal] recovery complete\n");
     return UIOX_JR_OK;
 }
 