#include "bcache.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────
 * Simulated disk
 * ───────────────────────────────────────────────────────────── */
static uint8_t  sim_disk[MAX_DEVICES][NUM_DISK_BLOCKS][BLOCK_SIZE];

/* ─────────────────────────────────────────────────────────────
 * Buffer pool storage
 * ───────────────────────────────────────────────────────────── */
static BufHdr   pool[NUM_BUFFERS];

/* ─────────────────────────────────────────────────────────────
 * Global list heads (defined in header as extern)
 * ───────────────────────────────────────────────────────────── */
BufHdr    hash_heads[NUM_HASH_QUEUES];
BufHdr    free_head;
BufStats  bcache_stats;

/* ─────────────────────────────────────────────────────────────
 * Simulated sleep/wakeup state
 *
 * In a real kernel, sleep() suspends the calling process and
 * wakeup() resumes all processes sleeping on an event address.
 * Here we use a simple flag array to simulate the effect:
 *   wakeup_any_free  — set when brelse makes a buffer available
 *   wakeup_buf[i]    — set when buffer i is released
 * ───────────────────────────────────────────────────────────── */
static volatile bool wakeup_any_free;
static volatile bool wakeup_buf[NUM_BUFFERS];

static void sim_sleep_any_free(void)
{
    /* Simulate: block until wakeup_any_free is signalled */
    printf("  [sleep] waiting for any free buffer\n");
    wakeup_any_free = false; /* will be set by brelse */
}

static void sim_sleep_buf(BufHdr *buf)
{
    int idx = (int)(buf - pool);
    printf("  [sleep] waiting for buffer %d (dev=%u blk=%u) to unlock\n",
           idx, buf->dev, buf->blkno);
    wakeup_buf[idx] = false;
}

static void sim_wakeup_any(void)
{
    wakeup_any_free = true;
    printf("  [wakeup] signalling: any free buffer available\n");
}

static void sim_wakeup_buf(BufHdr *buf)
{
    int idx = (int)(buf - pool);
    wakeup_buf[idx] = true;
    printf("  [wakeup] signalling: buffer %d now free\n", idx);
}

/* ─────────────────────────────────────────────────────────────
 * Hash function — : blkno mod NUM_HASH_QUEUES
 * Device number is also factored in to keep multiple filesystems
 * from colliding on the same queue.
 * ───────────────────────────────────────────────────────────── */
static int hash_slot(uint8_t dev, uint32_t blkno)
{
    return (int)((dev * 7u + blkno) % NUM_HASH_QUEUES);
}

/* ─────────────────────────────────────────────────────────────
 * Circular doubly-linked list helpers
 *
 * All lists use a sentinel dummy node so that insert/remove never
 * need to special-case an empty list.
 * ───────────────────────────────────────────────────────────── */

/* Initialise a sentinel node to point to itself */
static void list_init_sentinel(BufHdr *sentinel)
{
    sentinel->hash_next = sentinel->hash_prev = sentinel;
    sentinel->free_next = sentinel->free_prev = sentinel;
}

/* ── Hash-queue list operations ────────────────────────────── */

static void hq_insert_after(BufHdr *pos, BufHdr *node)
{
    node->hash_next       = pos->hash_next;
    node->hash_prev       = pos;
    pos->hash_next->hash_prev = node;
    pos->hash_next        = node;
}

static void hq_remove(BufHdr *node)
{
    node->hash_prev->hash_next = node->hash_next;
    node->hash_next->hash_prev = node->hash_prev;
    node->hash_next = node->hash_prev = node; /* self-loop */
}

static BufHdr *hq_lookup(uint8_t dev, uint32_t blkno)
{
    BufHdr *head = &hash_heads[hash_slot(dev, blkno)];
    BufHdr *cur  = head->hash_next;
    while (cur != head) {
        if (cur->dev == dev && cur->blkno == blkno) return cur;
        cur = cur->hash_next;
    }
    return NULL;
}

/* ── Free-list operations ──────────────────────────────────── */

static bool fl_is_empty(void)
{
    return free_head.free_next == &free_head;
}

/* Insert node just before 'pos' (i.e. at TAIL: pos = &free_head) */
static void fl_insert_before(BufHdr *pos, BufHdr *node)
{
    node->free_prev       = pos->free_prev;
    node->free_next       = pos;
    pos->free_prev->free_next = node;
    pos->free_prev        = node;
}

/* Insert node just after 'pos' (i.e. at HEAD: pos = &free_head) */
static void fl_insert_after(BufHdr *pos, BufHdr *node)
{
    node->free_next       = pos->free_next;
    node->free_prev       = pos;
    pos->free_next->free_prev = node;
    pos->free_next        = node;
}

static void fl_remove(BufHdr *node)
{
    node->free_prev->free_next = node->free_next;
    node->free_next->free_prev = node->free_prev;
    node->free_next = node->free_prev = node; /* self-loop */
}

/* Return first real buffer from the free list (head), or NULL */
static BufHdr *fl_pop_head(void)
{
    if (fl_is_empty()) return NULL;
    BufHdr *b = free_head.free_next;
    fl_remove(b);
    return b;
}

/* ─────────────────────────────────────────────────────────────
 * Simulated disk I/O
 * ───────────────────────────────────────────────────────────── */
static void disk_read(BufHdr *buf)
{
    printf("  [disk] READ  dev=%u blk=%u\n", buf->dev, buf->blkno);
    if (buf->dev < MAX_DEVICES && buf->blkno < NUM_DISK_BLOCKS)
        memcpy(buf->data, sim_disk[buf->dev][buf->blkno], BLOCK_SIZE);
    else
        memset(buf->data, 0, BLOCK_SIZE);
    buf->status |=  BUF_VALID;
    buf->status &= (uint32_t)~BUF_IOBUSY;
}

static void disk_write(BufHdr *buf)
{
    printf("  [disk] WRITE dev=%u blk=%u\n", buf->dev, buf->blkno);
    if (buf->dev < MAX_DEVICES && buf->blkno < NUM_DISK_BLOCKS)
        memcpy(sim_disk[buf->dev][buf->blkno], buf->data, BLOCK_SIZE);
    buf->status &= (uint32_t)~(BUF_IOBUSY | BUF_DELWRITE | BUF_OLD);
}

/* ─────────────────────────────────────────────────────────────
 * bcache_init
 *
 * During system initialisation:
 *   • All buffers go onto the free list.
 *   • All buffers also go on hash queue 0 (they have no valid dev/blk
 *     yet; they will be re-hashed the first time they are allocated).
 * ───────────────────────────────────────────────────────────── */
void bcache_init(void)
{
    memset(sim_disk, 0, sizeof sim_disk);
    memset(pool,     0, sizeof pool);
    memset(&bcache_stats, 0, sizeof bcache_stats);
    wakeup_any_free = false;
    memset((void*)wakeup_buf, 0, sizeof wakeup_buf);

    /* Initialise all sentinel nodes */
    list_init_sentinel(&free_head);
    for (int i = 0; i < NUM_HASH_QUEUES; i++)
        list_init_sentinel(&hash_heads[i]);

    /* Place every buffer on the free list and hash queue 0 */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        pool[i].dev    = 0;
        pool[i].blkno  = (uint32_t)i;   /* arbitrary initial blkno */
        pool[i].status = 0;             /* free, invalid            */

        /* Free list — tail insert → FIFO order at boot */
        fl_insert_before(&free_head, &pool[i]);

        /* Hash queue */
        int slot = hash_slot(pool[i].dev, pool[i].blkno);
        hq_insert_after(&hash_heads[slot], &pool[i]);
    }

    printf("[bcache] init: %d buffers  %d hash queues  "
           "block_size=%d\n",
           NUM_BUFFERS, NUM_HASH_QUEUES, BLOCK_SIZE);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm 2 — brelse 
 *
 * Must be callable from an interrupt handler (async I/O
 * completion), so it raises/lowers the interrupt level.
 * In simulation we model that with a comment.
 * ───────────────────────────────────────────────────────────── */
void brelse(BufHdr *buf)
{
    if (!buf) return;

    /* Wakeup all processes waiting for any free buffer (scenario 4) */
    sim_wakeup_any();

    /* Wakeup all processes waiting specifically for this buffer (scenario 5) */
    sim_wakeup_buf(buf);

    /* ── Raise processor execution level (block interrupts) ── */
    /* [In real kernel: spl6() or equivalent] */

    if ((buf->status & BUF_VALID) && !(buf->status & BUF_OLD)) {
        /* Valid, not old → LRU tail (most-recently-used position) */
        fl_insert_before(&free_head, buf);
        printf("[brelse] dev=%u blk=%-4u → tail of free list (LRU)\n",
               buf->dev, buf->blkno);
    } else {
        /* Invalid or old → free-list head (evict next) */
        fl_insert_after(&free_head, buf);
        printf("[brelse] dev=%u blk=%-4u → head of free list (evict soon)\n",
               buf->dev, buf->blkno);
    }

    /* ── Lower processor execution level (allow interrupts) ── */
    /* [In real kernel: splx()] */

    buf->status &= (uint32_t)~BUF_LOCKED;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm 1 — getblk 
 *
 * Five scenarios implemented in order:
 *   1  block in cache, buffer free         → mark busy, return
 *   5  block in cache, buffer busy         → sleep, restart
 *   4  block not in cache, free list empty → sleep, restart
 *   3  block not in cache, free buf is DW  → async write, retry
 *   2  block not in cache, clean free buf  → reassign, return
 * ───────────────────────────────────────────────────────────── */
BufHdr *getblk(uint8_t dev, uint32_t blkno)
{
    printf("[getblk] request dev=%u blk=%u\n", dev, blkno);

    while (1) {
        BufHdr *buf = hq_lookup(dev, blkno);

        if (buf) {
            /* ── Block IS in the cache ─────────────────────── */

            if (buf->status & BUF_LOCKED) {
                /* ── Scenario 5: buffer busy ─────────────────
                 * Mark it "wanted"; sleep; restart the loop.
                 * On wake-up we must re-search — another process
                 * may have re-assigned this buffer to a different
                 * block (race described in Section 3.3 / Fig 3.12).
                 */
                printf("[getblk] scenario 5: dev=%u blk=%u busy — sleeping\n",
                       dev, blkno);
                buf->status |= BUF_WANTED;
                bcache_stats.busy_waits++;
                sim_sleep_buf(buf);
                continue; /* restart while loop */
            }

            /* ── Scenario 1: found and free ────────────────── */
            printf("[getblk] scenario 1: dev=%u blk=%u found in cache\n",
                   dev, blkno);
            buf->status |= BUF_LOCKED;
            fl_remove(buf);                  /* remove from free list */
            bcache_stats.hits++;
            return buf;

        } else {
            /* ── Block NOT in the cache ────────────────────── */

            if (fl_is_empty()) {
                /* ── Scenario 4: no free buffers ───────────────
                 * Sleep until brelse() signals that a buffer is
                 * available; then restart the ENTIRE loop (another
                 * process may have loaded our block while we slept).
                 */
                printf("[getblk] scenario 4: dev=%u blk=%u — "
                       "no free buffers, sleeping\n", dev, blkno);
                bcache_stats.free_waits++;
                sim_sleep_any_free();
                continue; /* restart while loop */
            }

            /* Remove the LRU buffer from the head of the free list */
            BufHdr *reuse = fl_pop_head();

            if (reuse->status & BUF_DELWRITE) {
                /* ── Scenario 3: delayed-write buffer ──────────
                 * We cannot reuse it immediately — must flush to
                 * disk first (asynchronously).  Put it back on the
                 * hash queue temporarily, start the write, then
                 * restart the loop.  When the async write completes
                 * brelse() will place it at the FREE LIST HEAD
                 * (because BUF_OLD will be set).
                 */
                printf("[getblk] scenario 3: dev=%u blk=%u — "
                       "delayed-write buf (dev=%u blk=%u), flushing\n",
                       dev, blkno, reuse->dev, reuse->blkno);
                bcache_stats.delayed_writes++;

                /* Mark for async completion → head of free list */
                reuse->status |= (BUF_IOBUSY | BUF_OLD | BUF_LOCKED);
                disk_write(reuse);          /* simulated async write   */
                brelse(reuse);              /* release → head of list  */
                continue; /* restart while loop */
            }

            /* ── Scenario 2: clean free buffer ─────────────── */
            printf("[getblk] scenario 2: dev=%u blk=%u — "
                   "reassigning free buf (was dev=%u blk=%u)\n",
                   dev, blkno, reuse->dev, reuse->blkno);

            /* Remove from old hash queue */
            hq_remove(reuse);

            /* Assign new identity */
            reuse->dev    = dev;
            reuse->blkno  = blkno;
            reuse->status = BUF_LOCKED;    /* valid cleared intentionally */

            /* Insert on new hash queue */
            int slot = hash_slot(dev, blkno);
            hq_insert_after(&hash_heads[slot], reuse);

            bcache_stats.misses++;
            return reuse;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm 3 — bread  
 * ───────────────────────────────────────────────────────────── */
BufHdr *bread(uint8_t dev, uint32_t blkno)
{
    printf("[bread] dev=%u blk=%u\n", dev, blkno);
    bcache_stats.reads++;

    BufHdr *buf = getblk(dev, blkno);

    if (buf->status & BUF_VALID) {
        /* Data already in cache — no disk I/O needed */
        printf("[bread] cache hit dev=%u blk=%u\n", dev, blkno);
        return buf;
    }

    /* Initiate disk read; simulated synchronously here */
    buf->status |= BUF_IOBUSY;
    disk_read(buf);              /* sets BUF_VALID, clears BUF_IOBUSY */

    /* In a real kernel: sleep(event disk read complete) */
    printf("[bread] disk read complete dev=%u blk=%u\n", dev, blkno);
    return buf;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm 4 — breada  
 *
 * Read 'blkno' for the caller; asynchronously pre-fetch 'ra_blkno'.
 * The read-ahead buffer is released automatically when I/O completes
 * so any later bread(ra_blkno) will find it in the cache.
 * ───────────────────────────────────────────────────────────── */
BufHdr *breada(uint8_t dev, uint32_t blkno, uint32_t ra_blkno)
{
    printf("[breada] dev=%u blk=%u  ra_blk=%u\n",
           dev, blkno, ra_blkno);

    BufHdr *first_buf  = NULL;
    bool    first_in_cache = false;

    /* ── First block ─────────────────────────────────────── */
    BufHdr *existing = hq_lookup(dev, blkno);
    if (!existing || !(existing->status & BUF_VALID)) {
        first_buf = getblk(dev, blkno);
        if (!(first_buf->status & BUF_VALID)) {
            first_buf->status |= BUF_IOBUSY;
            disk_read(first_buf);
        }
    } else {
        first_in_cache = true;
        printf("[breada] first block already in cache\n");
    }

    /* ── Second block (read-ahead) ───────────────────────── */
    BufHdr *ra_existing = hq_lookup(dev, ra_blkno);
    if (!ra_existing || !(ra_existing->status & BUF_VALID)) {
        BufHdr *ra_buf = getblk(dev, ra_blkno);
        if (ra_buf->status & BUF_VALID) {
            /* Already valid — release immediately */
            brelse(ra_buf);
        } else {
            /* Initiate asynchronous read for read-ahead block */
            printf("[breada] initiating async read-ahead dev=%u blk=%u\n",
                   dev, ra_blkno);
            bcache_stats.readaheads++;
            ra_buf->status |= (BUF_IOBUSY | BUF_ASYNC);
            disk_read(ra_buf);          /* simulated; real: async */
            ra_buf->status &= (uint32_t)~BUF_ASYNC;
            /*
             * When async I/O completes the interrupt handler calls
             * brelse(), placing the buffer on the free list so it
             * is available for a later bread(ra_blkno).
             */
            brelse(ra_buf);
        }
    } else {
        printf("[breada] read-ahead block already in cache\n");
    }

    /* ── Return first block ──────────────────────────────── */
    if (first_in_cache) {
        return bread(dev, blkno);   /* will be a cache hit */
    }

    /* first_buf I/O already completed above (simulated sync) */
    printf("[breada] returning first block dev=%u blk=%u\n", dev, blkno);
    return first_buf;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm 5 — bwrite  
 * ───────────────────────────────────────────────────────────── */
void bwrite(BufHdr *buf, bool sync, bool delayed)
{
    if (!buf) return;
    bcache_stats.writes++;

    if (delayed) {
        /*
         * Delayed write: mark the buffer; do NOT start I/O now.
         * getblk scenario 3 will flush it before re-assigning.
         */
        buf->status |= BUF_DELWRITE;
        buf->status &= (uint32_t)~BUF_LOCKED;
        printf("[bwrite] DELAYED  dev=%u blk=%u marked for later flush\n",
               buf->dev, buf->blkno);
        /* Note: caller should not call brelse() for delayed writes;
         * it is released here implicitly via the flag. */
        fl_insert_before(&free_head, buf); /* place at tail of free list */
        return;
    }

    /* Initiate disk write */
    buf->status |= BUF_IOBUSY;
    disk_write(buf);

    if (sync) {
        /*
         * Synchronous write: sleep until I/O complete, then release.
         * (In real kernel: sleep(event I/O complete))
         */
        printf("[bwrite] SYNC    dev=%u blk=%u — write complete\n",
               buf->dev, buf->blkno);
        buf->status &= (uint32_t)~BUF_IOBUSY;
        brelse(buf);
    } else {
        /*
         * Asynchronous write: I/O started; do not sleep.
         * brelse() will be called by the interrupt handler when done.
         * Mark OLD so brelse() places it at the HEAD of the free list.
         */
        printf("[bwrite] ASYNC   dev=%u blk=%u — I/O in progress\n",
               buf->dev, buf->blkno);
        buf->status |= BUF_OLD;
        buf->status &= (uint32_t)~BUF_IOBUSY;
        brelse(buf);   /* interrupt handler releases; head of list */
    }
}

/* ─────────────────────────────────────────────────────────────
 * bdwrite — mark buffer for delayed write and release
 * ───────────────────────────────────────────────────────────── */
void bdwrite(BufHdr *buf)
{
    if (!buf) return;
    printf("[bdwrite] dev=%u blk=%u\n", buf->dev, buf->blkno);
    bwrite(buf, false, true);
}

/* ─────────────────────────────────────────────────────────────
 * bflush — flush all delayed-write buffers for a device
 * Called e.g. by sync(2) or umount(2).
 * ───────────────────────────────────────────────────────────── */
void bflush(uint8_t dev)
{
    printf("[bflush] flushing delayed-write buffers for dev=%u\n", dev);
    int flushed = 0;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        BufHdr *b = &pool[i];
        if (b->dev == dev && (b->status & BUF_DELWRITE)) {
            /* Remove from free list, lock it, write synchronously */
            fl_remove(b);
            b->status |= BUF_LOCKED;
            b->status &= (uint32_t)~BUF_DELWRITE;
            disk_write(b);
            brelse(b);
            flushed++;
        }
    }
    printf("[bflush] flushed %d buffer(s)\n", flushed);
}

/* ─────────────────────────────────────────────────────────────
 * bcache_print — show pool state (debug)
 * ───────────────────────────────────────────────────────────── */
void bcache_print(void)
{
    printf("\n[bcache] pool state:\n");
    printf("  %-4s  %-4s  %-6s  %-6s  %-7s  %-8s  %-7s  %-5s\n",
           "idx", "dev", "blkno", "locked", "valid", "delwrite",
           "iobusy", "old");
    for (int i = 0; i < NUM_BUFFERS; i++) {
        BufHdr *b = &pool[i];
        printf("  %-4d  %-4u  %-6u  %-6s  %-7s  %-8s  %-7s  %-5s\n",
               i, b->dev, b->blkno,
               (b->status & BUF_LOCKED)  ? "YES" : "no",
               (b->status & BUF_VALID)   ? "YES" : "no",
               (b->status & BUF_DELWRITE)? "YES" : "no",
               (b->status & BUF_IOBUSY)  ? "YES" : "no",
               (b->status & BUF_OLD)     ? "YES" : "no");
    }

    printf("\n  Hash queues:\n");
    for (int q = 0; q < NUM_HASH_QUEUES; q++) {
        printf("  Q%d [blkno %% %d == %d]: ",
               q, NUM_HASH_QUEUES, q);
        BufHdr *head = &hash_heads[q];
        BufHdr *cur  = head->hash_next;
        while (cur != head) {
            printf("(dev=%u blk=%u%s) ",
                   cur->dev, cur->blkno,
                   (cur->status & BUF_LOCKED) ? "L" : "");
            cur = cur->hash_next;
        }
        printf("\n");
    }

    printf("\n  Free list (LRU head→tail): ");
    BufHdr *cur = free_head.free_next;
    while (cur != &free_head) {
        printf("(dev=%u blk=%u) ", cur->dev, cur->blkno);
        cur = cur->free_next;
    }
    printf("\n");
}

/* ─────────────────────────────────────────────────────────────
 * bcache_stats_print
 * ───────────────────────────────────────────────────────────── */
void bcache_stats_print(void)
{
    uint64_t total = bcache_stats.hits + bcache_stats.misses;
    printf("\n[bcache] statistics:\n");
    printf("  hits          : %llu\n",
           (unsigned long long)bcache_stats.hits);
    printf("  misses        : %llu\n",
           (unsigned long long)bcache_stats.misses);
    printf("  hit rate      : %.1f%%\n",
           total ? 100.0 * (double)bcache_stats.hits / (double)total : 0.0);
    printf("  delayed writes: %llu\n",
           (unsigned long long)bcache_stats.delayed_writes);
    printf("  free waits    : %llu  (scenario 4)\n",
           (unsigned long long)bcache_stats.free_waits);
    printf("  busy waits    : %llu  (scenario 5)\n",
           (unsigned long long)bcache_stats.busy_waits);
    printf("  reads (bread) : %llu\n",
           (unsigned long long)bcache_stats.reads);
    printf("  writes(bwrite): %llu\n",
           (unsigned long long)bcache_stats.writes);
    printf("  readaheads    : %llu\n",
           (unsigned long long)bcache_stats.readaheads);
}
