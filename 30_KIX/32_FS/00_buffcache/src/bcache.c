/*
 * 31_BufferCache/00_FileBuff/src/bcache.c
 *
 * UIOX Block Buffer Cache implementation.
 * Implements getblk/brelse/bread/breada/bwrite/bdwrite/bflush.
 *
 * Based on the classic Unix buffer cache (Bach, 1986).
 * Freestanding — no libc, no malloc, no stdio.
 *
 * Physical I/O stub: reads/writes to a DRAM region.
 * Override bcache_plat_read_block() / bcache_plat_write_block()
 * from 10_BSP for real block device I/O.
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "bcache.h"
 #include "uiox_soc_string.h"   /* memset, memcpy — freestanding         */
 #include "uiox_soc_stdio.h"    /* early_puts                            */
 
 /* ── Buffer pool ───────────────────────────────────────────────────── */
 static BufHdr  s_pool[NUM_BUFFERS];
 
 /* ── Free list — circular doubly-linked with dummy head ────────────── */
 static BufHdr  s_free_head;   /* sentinel — never holds data           */
 
 /* ── Hash queues — singly-chained by hash_next ─────────────────────── */
 static BufHdr *s_hash_heads[NUM_HASH_QUEUES];
 
 /* ── Statistics ─────────────────────────────────────────────────────── */
 BufStats bcache_stats;
 
 /* ── Platform I/O stubs ─────────────────────────────────────────────── */
 #define BCACHE_DRAM_BASE_DEFAULT  0x46000000UL
 
 __attribute__((weak))
 uintptr_t bcache_plat_dram_base(void) { return BCACHE_DRAM_BASE_DEFAULT; }
 
 static void plat_read_block(uint8_t dev, uint32_t blkno, uint8_t *buf)
 {
     uintptr_t addr = bcache_plat_dram_base()
                    + (uintptr_t)dev * 1024u * BCACHE_SECTOR_SIZE
                    + (uintptr_t)blkno * BCACHE_SECTOR_SIZE;
     memcpy(buf, (const void *)addr, BCACHE_SECTOR_SIZE);
 }
 
 static void plat_write_block(uint8_t dev, uint32_t blkno,
                               const uint8_t *buf)
 {
     uintptr_t addr = bcache_plat_dram_base()
                    + (uintptr_t)dev * 1024u * BCACHE_SECTOR_SIZE
                    + (uintptr_t)blkno * BCACHE_SECTOR_SIZE;
     memcpy((void *)addr, buf, BCACHE_SECTOR_SIZE);
 }
 
 /* ── Hash function ──────────────────────────────────────────────────── */
 static inline uint32_t hash_slot(uint8_t dev, uint32_t blkno)
 {
     return (((uint32_t)dev * 31u) ^ blkno) % NUM_HASH_QUEUES;
 }
 
 /* ── Free list operations ───────────────────────────────────────────── */
 static void fl_remove(BufHdr *b)
 {
     b->free_prev->free_next = b->free_next;
     b->free_next->free_prev = b->free_prev;
     b->free_next = b->free_prev = (BufHdr *)0;
 }
 
 static void fl_insert_tail(BufHdr *b)
 {
     /* Insert before sentinel = insert at tail = MRU position */
     BufHdr *prev     = s_free_head.free_prev;
     b->free_prev     = prev;
     b->free_next     = &s_free_head;
     prev->free_next  = b;
     s_free_head.free_prev = b;
 }
 
 static void fl_insert_head(BufHdr *b)
 {
     /* Insert after sentinel = insert at head = LRU evict soon */
     BufHdr *next     = s_free_head.free_next;
     b->free_next     = next;
     b->free_prev     = &s_free_head;
     next->free_prev  = b;
     s_free_head.free_next = b;
 }
 
 static BufHdr *fl_pop_head(void)
 {
     /* Pop from head = take oldest (LRU) buffer */
     if (s_free_head.free_next == &s_free_head)
         return (BufHdr *)0;   /* free list empty */
     BufHdr *b = s_free_head.free_next;
     fl_remove(b);
     return b;
 }
 
 /* ── Hash queue operations ──────────────────────────────────────────── */
 static void hash_insert(BufHdr *b)
 {
     uint32_t slot    = hash_slot(b->dev, b->blkno);
     b->hash_next     = s_hash_heads[slot];
     s_hash_heads[slot] = b;
 }
 
 static void hash_remove(BufHdr *b)
 {
     uint32_t  slot = hash_slot(b->dev, b->blkno);
     BufHdr  **pp   = &s_hash_heads[slot];
     while (*pp && *pp != b) pp = &(*pp)->hash_next;
     if (*pp) *pp = b->hash_next;
     b->hash_next = (BufHdr *)0;
 }
 
 static BufHdr *hash_lookup(uint8_t dev, uint32_t blkno)
 {
     uint32_t slot = hash_slot(dev, blkno);
     BufHdr  *b    = s_hash_heads[slot];
     while (b) {
         if (b->dev == dev && b->blkno == blkno) return b;
         b = b->hash_next;
     }
     return (BufHdr *)0;
 }
 
 /* ── bcache_init ────────────────────────────────────────────────────── */
 void bcache_init(void)
 {
     memset(s_pool,       0, sizeof(s_pool));
     memset(s_hash_heads, 0, sizeof(s_hash_heads));
     memset(&bcache_stats,0, sizeof(bcache_stats));
 
     /* Sentinel points to itself — empty free list */
     s_free_head.free_next = &s_free_head;
     s_free_head.free_prev = &s_free_head;
 
     /* All buffers start on the free list (head = evict first) */
     for (uint32_t i = 0u; i < NUM_BUFFERS; i++) {
         s_pool[i].dev    = 0xFFu;
         s_pool[i].blkno  = 0xFFFFFFFFu;
         s_pool[i].status = 0u;
         fl_insert_tail(&s_pool[i]);
     }
 }
 
 /* ── Algorithm 1: getblk ────────────────────────────────────────────── */
 /*
  * All five scenarios from Bach Ch.3:
  *   S1 — block in cache, buffer free       → lock + return
  *   S2 — block not in cache, free buf avail → assign + return
  *   S3 — block not in cache, free buf has DELWRITE → flush + retry
  *   S4 — block not in cache, free list empty → spin (no sleep in kernel)
  *   S5 — block in cache, buffer locked      → spin
  */
 BufHdr *getblk(uint8_t dev, uint32_t blkno)
 {
     while (1) {
         BufHdr *b = hash_lookup(dev, blkno);
 
         if (b) {
             /* Block found in cache */
             if (b->status & BUF_LOCKED) {
                 /* Scenario 5 — buffer busy, spin */
                 bcache_stats.busy_waits++;
                 continue;
             }
             /* Scenario 1 — found and free */
             fl_remove(b);
             b->status |= BUF_LOCKED;
             bcache_stats.hits++;
             return b;
         }
 
         /* Block not in cache — need a free buffer */
         BufHdr *free_b = fl_pop_head();
 
         if (!free_b) {
             /* Scenario 4 — no free buffer */
             bcache_stats.free_waits++;
             continue;   /* spin — in a real kernel we'd sleep */
         }
 
         if (free_b->status & BUF_DELWRITE) {
             /* Scenario 3 — must flush delayed-write first */
             bcache_stats.delayed_writes++;
             plat_write_block(free_b->dev, free_b->blkno, free_b->data);
             free_b->status &= ~BUF_DELWRITE;
             fl_insert_tail(free_b);
             continue;   /* retry */
         }
 
         /* Scenario 2 — reassign free buffer to new (dev, blkno) */
         hash_remove(free_b);
         free_b->dev    = dev;
         free_b->blkno  = blkno;
         free_b->status = BUF_LOCKED;   /* valid=0 — caller must read */
         hash_insert(free_b);
         bcache_stats.misses++;
         return free_b;
     }
 }
 
 /* ── Algorithm 2: brelse ────────────────────────────────────────────── */
 void brelse(BufHdr *buf)
 {
     if (!buf) return;
 
     /* Valid + not old → tail (LRU = keep longer) */
     if ((buf->status & BUF_VALID) && !(buf->status & BUF_OLD))
         fl_insert_tail(buf);
     else
         fl_insert_head(buf);   /* invalid or old → evict soon */
 
     buf->status &= ~(BUF_LOCKED | BUF_WANTED | BUF_OLD);
 }
 
 /* ── Algorithm 3: bread ─────────────────────────────────────────────── */
 BufHdr *bread(uint8_t dev, uint32_t blkno)
 {
     BufHdr *b = getblk(dev, blkno);
 
     if (b->status & BUF_VALID) {
         bcache_stats.reads++;
         return b;   /* cache hit */
     }
 
     /* Cache miss — read from device */
     plat_read_block(dev, blkno, b->data);
     b->status |= BUF_VALID;
     b->status &= ~BUF_ERROR;
     bcache_stats.reads++;
     return b;
 }
 
 /* ── Algorithm 4: breada (read + read-ahead) ────────────────────────── */
 BufHdr *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno)
 {
     BufHdr *b = getblk(dev, blkno);
 
     /* Initiate read-ahead asynchronously if not already cached */
     if (ra_blkno != blkno) {
         BufHdr *ra = hash_lookup(dev, ra_blkno);
         if (!ra || !(ra->status & BUF_VALID)) {
             /* Prefetch read-ahead block — fire and forget */
             BufHdr *ra_b = getblk(dev, ra_blkno);
             if (!(ra_b->status & BUF_VALID)) {
                 plat_read_block(dev, ra_blkno, ra_b->data);
                 ra_b->status |= BUF_VALID | BUF_ASYNC;
             }
             brelse(ra_b);
             bcache_stats.readaheads++;
         }
     }
 
     if (b->status & BUF_VALID) return b;
 
     plat_read_block(dev, blkno, b->data);
     b->status |= BUF_VALID;
     bcache_stats.reads++;
     return b;
 }
 
 /* ── Algorithm 5: bwrite ────────────────────────────────────────────── */
 void bwrite(BufHdr *buf, bool sync, bool delayed)
 {
     if (!buf) return;
 
     if (delayed) {
         /* Delayed write — mark, release, do not write now */
         buf->status |= BUF_DELWRITE;
         brelse(buf);
         bcache_stats.writes++;
         return;
     }
 
     /* Write to device */
     plat_write_block(buf->dev, buf->blkno, buf->data);
     buf->status &= ~(BUF_DELWRITE | BUF_DIRTY | BUF_ERROR);
     buf->status |= BUF_VALID;
     bcache_stats.writes++;
 
     if (sync) {
         /* Synchronous — release buffer now */
         brelse(buf);
     }
     /* Async — caller responsible for brelse on completion */
 }
 
 /* ── bdwrite — delayed write convenience ───────────────────────────── */
 void bdwrite(BufHdr *buf)
 {
     bwrite(buf, false, true);
 }
 
 /* ── bflush — flush all delayed writes for a device ────────────────── */
 /*
  * Called by SYS_SYNC / SYS_FSYNC.
  * Scans entire pool for BUF_DELWRITE on the given device.
  */
 void bflush(uint8_t dev)
 {
     for (uint32_t i = 0u; i < NUM_BUFFERS; i++) {
         BufHdr *b = &s_pool[i];
         if (b->dev == dev &&
             (b->status & BUF_DELWRITE) &&
             !(b->status & BUF_LOCKED)) {
             b->status |= BUF_LOCKED;
             plat_write_block(b->dev, b->blkno, b->data);
             b->status &= ~(BUF_DELWRITE | BUF_DIRTY);
             b->status |= BUF_VALID;
             brelse(b);
         }
     }
 }
 
 /* ── bcache_print ───────────────────────────────────────────────────── */
 void bcache_print(void)
 {
     early_puts("[bcache] stats: hits/misses/reads/writes\n");
 }
 