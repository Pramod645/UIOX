/*
 *  31_BufferCache/00_FileBuff/buffers/src/bcache_init.c
 *
 *  The buffer pool, its two sentinels, the hash table, the platform I/O
 *  hooks, and the debug printers.
 *
 *  ── why this file exists ──────────────────────────────────────────────
 *  Bach presents getblk/brelse/bread/breada/bwrite as five algorithms
 *  over ONE pool.  Splitting them into five files made the pool itself
 *  homeless: all five read and write it, so exactly one translation unit
 *  must own the storage or the linker sees multiple definitions.  This
 *  is that unit.
 *
 *  What it owns:
 *      bcache_pool[NUM_BUFFERS]        the 256 buffer headers
 *      free_head                       the LRU free list's dummy head
 *      hash_heads[NUM_HASH_QUEUES]     the 64 hash-queue sentinels
 *      bcache_stats                    the counters
 *
 *  ── Bach's two sentinels, and why they are sentinels ─────────────────
 *  "This is a doubly linked circular list of buffers with a dummy buffer
 *   header that marks its beginning and end."
 *
 *  The dummy head is not laziness — it removes every NULL edge case from
 *  the list operations.  Inserting before the sentinel IS inserting at
 *  the tail; inserting after it IS inserting at the head; and popping
 *  from an empty list is the single test `free_head.free_next == &free_head`.
 *  Without it, each of the four list operations needs its own empty-list
 *  branch, and four branches are four places to get it wrong.
 *
 *  ── the hash table ────────────────────────────────────────────────────
 *  Bach: "Kernel uses hashing function that distributes the buffer
 *  uniformly across the set of hash queues and hash function is simple to
 *  improve performance.  Admin should configure the number of hash queues
 *  when generating the OS."
 *
 *  NUM_HASH_QUEUES is that configuration, and it is 64 — a power of two,
 *  so the modulo in bcache_hash_slot() is a mask on any compiler worth
 *  the name.  256 buffers over 64 queues averages 4 per queue: short
 *  chains, which is the point.
 *
 *  Each hash queue is also a circular doubly-linked list WITH a sentinel
 *  (hash_heads[i]), so a lookup never needs a NULL test at the head.
 *
 *  ── the platform hooks ────────────────────────────────────────────────
 *  The default backs the pool with a DRAM region so the filesystem can be
 *  exercised before a block driver exists.  10_BSP replaces these.
 *
 *  NOTE on overriding: these are ORDINARY functions, not static-inline or
 *  weak, so they override by simply not being linked — the BSP build lists
 *  its own bcache_plat_read_block and this one is dropped.  A weak
 *  static-inline in a header, which is what bcache_types.h used for
 *  bcache_plat_num_blocks, cannot be overridden that way: a static inline
 *  is a private copy per translation unit.  See the note under
 *  bcache_plat_num_blocks below.
 *
 *  ── the block address arithmetic ──────────────────────────────────────
 *  A (dev, blkno) pair maps to a byte offset in the DRAM region as:
 *
 *      base + dev * BCACHE_DEV_STRIDE + blkno * BCACHE_SECTOR_SIZE
 *
 *  where the stride is the per-device size in bytes.  The stride must be
 *  at least NUM_DISK_BLOCKS_DEFAULT * 512 or two devices overlap, and an
 *  overlap is silent: device 1's block 0 would be device 0's block 2048.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "bcache.h"
#include "bcache_internal.h"

/* ═════════════════════════════════════════════════════════════════════
 * THE POOL — the one definition of each of these four
 * ═════════════════════════════════════════════════════════════════════ */

/* The 256 buffer headers.  Bach's BufferHeaders, arrayed. */
BufHdr   bcache_pool[NUM_BUFFERS];

/* The free list's dummy head.  Circular: it points at itself when empty. */
BufHdr   free_head;

/* One sentinel per hash queue, so a chain walk needs no NULL test. */
BufHdr   hash_heads[NUM_HASH_QUEUES];

/* Counters — read by bcache_stats_print() and by the bring-up log. */
BufStats bcache_stats;

/* ═════════════════════════════════════════════════════════════════════
 * Platform I/O — the default DRAM backing
 *
 * A real device is reached by NOT linking this file's definitions; the
 * BSP provides its own with the same names.
 * ═════════════════════════════════════════════════════════════════════ */
#define BCACHE_DRAM_BASE_DEFAULT   0x46000000UL

/* Per-device stride.  NUM_DISK_BLOCKS_DEFAULT blocks of sector size,
 * rounded up to a page multiple so devices do not share a page. */
#define BCACHE_DEV_STRIDE_DEFAULT \
    ((((uintptr_t)NUM_DISK_BLOCKS_DEFAULT * BCACHE_SECTOR_SIZE) + 0xFFFu) & ~0xFFFu)

uintptr_t bcache_plat_dram_base(void)
{
    return (uintptr_t)BCACHE_DRAM_BASE_DEFAULT;
}

/*
 * bcache_plat_num_blocks — how many blocks a device holds.
 *
 * NOTE: bcache_types.h declares a `static inline __attribute__((weak))`
 * version of this.  A static inline is a private copy in every
 * translation unit that includes the header, so a BSP cannot replace it
 * at link time — the header's copy always wins inside each unit.
 *
 * The fix is a two-line change to bcache_types.h: delete the static
 * inline and declare
 *
 *      extern uint32_t bcache_plat_num_blocks(uint8_t dev);
 *
 * then define it here (as below) and let the BSP provide its own.  Until
 * that happens, this definition is unreachable and every caller sees the
 * header's default.
 */
uint32_t bcache_plat_num_blocks(uint8_t dev)
{
    (void)dev;
    return NUM_DISK_BLOCKS_DEFAULT;
}

/* ── the byte address of a (dev, blkno) — bounds-checked ────────────── */
static uintptr_t plat_addr(uint8_t dev, uint32_t blkno)
{
    uintptr_t base = bcache_plat_dram_base();

    /* Clamp the device: a stray dev must not write into the next
     * device's region, or worse past the end of the region entirely. */
    if (dev >= MAX_DEVICES) dev = MAX_DEVICES - 1u;

    return base
         + (uintptr_t)dev * BCACHE_DEV_STRIDE_DEFAULT
         + (uintptr_t)blkno * BCACHE_SECTOR_SIZE;
}

void bcache_plat_read_block(uint8_t dev, uint32_t blkno, uint8_t *buf)
{
    uint32_t nblocks = bcache_plat_num_blocks(dev);
    uintptr_t addr   = plat_addr(dev, blkno);

    if (!buf) return;

    /* A read past the device end returns zeros rather than whatever DRAM
     * happened to hold.  bread() checks the same bound and reports it, so
     * this is the second line of defence — a caller that ignored
     * BUF_ERROR still must not see stale bytes as file data. */
    if (nblocks != 0u && blkno >= nblocks) {
        memset(buf, 0, BCACHE_SECTOR_SIZE);
        return;
    }

    memcpy(buf, (const void *)addr, BCACHE_SECTOR_SIZE);
}

void bcache_plat_write_block(uint8_t dev, uint32_t blkno, const uint8_t *buf)
{
    uint32_t nblocks = bcache_plat_num_blocks(dev);
    uintptr_t addr   = plat_addr(dev, blkno);

    if (!buf) return;

    /* A write past the end is dropped, not clamped onto the last block:
     * silently corrupting a valid block is worse than losing an invalid
     * write, and the caller's block map is the thing that was wrong. */
    if (nblocks != 0u && blkno >= nblocks) {
        printf("[bcache] ERROR: write dev=%u blk=%u past device end "
               "(%u blocks) — dropped\n",
               (unsigned)dev, (unsigned)blkno, (unsigned)nblocks);
        return;
    }

    memcpy((void *)addr, buf, BCACHE_SECTOR_SIZE);
}

/* ═════════════════════════════════════════════════════════════════════
 * bcache_init — build the pool
 *
 * Bach's mkfs of the cache: every buffer starts on the free list, and
 * the two sentinels are made circular and empty.
 * ═════════════════════════════════════════════════════════════════════ */
void bcache_init(void)
{
    uint32_t i;

    memset(bcache_pool,  0, sizeof bcache_pool);
    memset(hash_heads,   0, sizeof hash_heads);
    memset(&bcache_stats,0, sizeof bcache_stats);
    memset(&free_head,   0, sizeof free_head);

    /* ── the free list sentinel: circular and empty ───────────────── */
    free_head.free_next = &free_head;
    free_head.free_prev = &free_head;

    /* ── every hash-queue sentinel: circular and empty ───────────── */
    for (i = 0u; i < NUM_HASH_QUEUES; i++) {
        hash_heads[i].dev    = 0xFFu;
        hash_heads[i].blkno  = 0xFFFFFFFFu;
        hash_heads[i].status = 0u;
        hash_heads[i].hash_next = &hash_heads[i];   /* itself */
        hash_heads[i].hash_prev = &hash_heads[i];
    }

    /* ── every buffer starts FREE, on the free list ────────────────
     * Its dev/blkno are set to values no real block can have, so a
     * hash lookup can never match a buffer that holds no block. */
    for (i = 0u; i < NUM_BUFFERS; i++) {
        BufHdr *b = &bcache_pool[i];

        b->dev    = 0xFFu;
        b->blkno  = 0xFFFFFFFFu;
        b->status = 0u;
        b->hash_next = (BufHdr *)0;
        b->hash_prev = (BufHdr *)0;

        /* Tail = the list is ordered, and the first pop takes index 0. */
        bcache_fl_insert_tail(b);
    }

    printf("[bcache] init: %u buffers, %u hash queues, "
           "%u bytes per block, %u blocks per device\n",
           (unsigned)NUM_BUFFERS,
           (unsigned)NUM_HASH_QUEUES,
           (unsigned)BCACHE_SECTOR_SIZE,
           (unsigned)NUM_DISK_BLOCKS_DEFAULT);
}

/* ═════════════════════════════════════════════════════════════════════
 * bcache_print — the pool's current state
 *
 * Bach's five conditions, printed per buffer, so a starved pool shows
 * WHICH buffers are stuck and in what state.
 * ═════════════════════════════════════════════════════════════════════ */
void bcache_print(void)
{
    uint32_t i;
    uint32_t busy = 0u;

    printf("[bcache] pool:\n");

    for (i = 0u; i < NUM_BUFFERS; i++) {
        BufHdr *b = &bcache_pool[i];

        if (!(b->status & (BUF_LOCKED | BUF_VALID | BUF_DELWRITE))) continue;

        busy++;

        printf("  [%3u] dev=%u blk=%-6u %s%s%s%s%s%s\n",
               (unsigned)i,
               (unsigned)b->dev,
               (unsigned)b->blkno,
               (b->status & BUF_LOCKED)   ? "LOCKED "   : "",
               (b->status & BUF_VALID)    ? "VALID "    : "",
               (b->status & BUF_DELWRITE) ? "DELWRITE " : "",
               (b->status & BUF_IOBUSY)   ? "IOBUSY "   : "",
               (b->status & BUF_WANTED)   ? "WANTED "   : "",
               (b->status & BUF_ERROR)    ? "ERROR "    : "");
    }

    if (busy == 0u) printf("  (all buffers free)\n");

    /* The sum of the five per-buffer states is not a summary — this is. */
    printf("[bcache] %u of %u buffers holding state\n",
           (unsigned)busy, (unsigned)NUM_BUFFERS);
}

/* ═════════════════════════════════════════════════════════════════════
 * bcache_stats_print — the counters
 *
 * hits    : scenario 1 — found in cache, free
 * misses  : scenario 2 — took a free buffer and reassigned it
 * delayed : scenario 3 — took a delayed-write buffer, flushed it
 * free_w  : scenario 4 — free list empty, spun
 * busy_w  : scenario 5 — block cached but locked, spun
 *
 * The last two are the ones to watch: either climbing means the pool is
 * at or past its working set, and a getblk that returned NULL will have
 * logged before this runs.
 * ═════════════════════════════════════════════════════════════════════ */
void bcache_stats_print(void)
{
    printf("[bcache] hits=%u misses=%u reads=%u writes=%u\n",
           (unsigned)bcache_stats.hits,
           (unsigned)bcache_stats.misses,
           (unsigned)bcache_stats.reads,
           (unsigned)bcache_stats.writes);

    printf("[bcache] readaheads=%u delayed_writes=%u\n",
           (unsigned)bcache_stats.readaheads,
           (unsigned)bcache_stats.delayed_writes);

    printf("[bcache] back-pressure: free_waits=%u busy_waits=%u\n",
           (unsigned)bcache_stats.free_waits,
           (unsigned)bcache_stats.busy_waits);
}
