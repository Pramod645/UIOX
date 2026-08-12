/*
 * 31_BufferCache/00_FileBuff/src/uiox_page_cache.c
 *
 * UIOX Page Cache implementation.
 *
 * Layering:
 *   VFS (32_FS)
 *     → uiox_pc_read/write  (this file)
 *         → bread/bwrite    (bcache.c — block buffer cache)
 *             → plat_read/write_block  (DRAM / eMMC device)
 *
 * Key data path:
 *   uiox_pc_read(ino, offset, kbuf, len)
 *     1. Page cache lookup (ino, aligned_offset)
 *     2. HIT:  memcpy page->data + offset_in_page → kbuf
 *     3. MISS: call readpage(ino, offset)
 *              → map_fn(ino, offset) → (dev, blkno)
 *              → for each of 8 sectors: bread(dev, blkno+i)
 *              → memcpy 8×512 bytes into page->data
 *              → set PC_UPTODATE
 *              → memcpy page->data → kbuf
 *
 * mmap zero-copy path:
 *   uiox_pc_get_page_pa(ino, offset)
 *     → ensure page is uptodate (readpage if needed)
 *     → return page->pa  (physical address of page->data[])
 *     → caller inserts PA as PTE in user page table
 *     → user reads VA → same DRAM bytes, no copy
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_page_cache.h"
 #include "bcache.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"
 
 /* ── Page pool ─────────────────────────────────────────────────────── */
 static uiox_page_t  s_pages[UIOX_PC_MAX_PAGES];
 static uint8_t      s_page_inuse[UIOX_PC_MAX_PAGES];
 
 /* ── Hash table ────────────────────────────────────────────────────── */
 static uiox_page_t *s_hash[UIOX_PC_HASH_QUEUES];
 
 /* ── LRU list ──────────────────────────────────────────────────────── */
 static uiox_page_t  s_lru_head;   /* sentinel */
 
 /* ── Block mapping callback ────────────────────────────────────────── */
 static uiox_pc_map_fn_t s_map_fn = (uiox_pc_map_fn_t)0;
 
 /* ── Hash function ──────────────────────────────────────────────────── */
 static uint32_t pc_hash(uint32_t ino, uint64_t offset)
 {
     return ((ino * 2654435761u) ^ (uint32_t)(offset >> 12))
            % UIOX_PC_HASH_QUEUES;
 }
 
 /* ── Page allocator ─────────────────────────────────────────────────── */
 static uiox_page_t *page_alloc(void)
 {
     /* Try unused slot first */
     for (uint32_t i = 0u; i < UIOX_PC_MAX_PAGES; i++) {
         if (!s_page_inuse[i]) {
             s_page_inuse[i] = 1u;
             memset(&s_pages[i], 0, sizeof(s_pages[i]));
             /* Physical address of the data buffer */
             s_pages[i].pa = (uintptr_t)s_pages[i].data;
             return &s_pages[i];
         }
     }
 
     /*
      * No free slot — evict LRU clean page.
      * Walk from LRU head and evict first non-dirty, non-locked page.
      */
     uiox_page_t *victim = s_lru_head.lru_next;
     while (victim != &s_lru_head) {
         if (!(victim->flags & (PC_DIRTY | PC_LOCKED))) {
             /* Remove from hash */
             uint32_t slot = pc_hash(victim->ino, victim->offset);
             uiox_page_t **pp = &s_hash[slot];
             while (*pp && *pp != victim) pp = &(*pp)->hash_next;
             if (*pp) *pp = victim->hash_next;
 
             /* Remove from LRU */
             victim->lru_prev->lru_next = victim->lru_next;
             victim->lru_next->lru_prev = victim->lru_prev;
 
             memset(victim, 0, sizeof(*victim));
             victim->pa = (uintptr_t)victim->data;
             return victim;
         }
         victim = victim->lru_next;
     }
 
     return (uiox_page_t *)0;   /* all pages locked or dirty */
 }
 
 /* ── Hash operations ────────────────────────────────────────────────── */
 static uiox_page_t *pc_lookup(uint32_t ino, uint64_t offset)
 {
     uint64_t      aligned = offset & ~((uint64_t)UIOX_PC_PAGE_SIZE - 1u);
     uint32_t      slot    = pc_hash(ino, aligned);
     uiox_page_t  *p       = s_hash[slot];
     while (p) {
         if (p->ino == ino && p->offset == aligned) return p;
         p = p->hash_next;
     }
     return (uiox_page_t *)0;
 }
 
 static void pc_insert(uiox_page_t *p)
 {
     uint32_t slot    = pc_hash(p->ino, p->offset);
     p->hash_next     = s_hash[slot];
     s_hash[slot]     = p;
 
     /* Insert at LRU tail (most recently used) */
     uiox_page_t *prev = s_lru_head.lru_prev;
     p->lru_prev       = prev;
     p->lru_next       = &s_lru_head;
     prev->lru_next    = p;
     s_lru_head.lru_prev = p;
 }
 
 /* ── readpage — fill a page from the block device ──────────────────── */
 /*
  * This is the core of the page cache miss path:
  *   file offset → block device (dev, blkno) → bread() → page->data
  *
  * One page = BCACHE_BLOCKS_PER_PAGE (8) × 512-byte blocks.
  */
 static int readpage(uiox_page_t *p)
 {
     if (!s_map_fn) {
         /* No filesystem registered — page stays empty */
         memset(p->data, 0, UIOX_PC_PAGE_SIZE);
         p->flags |= PC_UPTODATE;
         return 0;
     }
 
     for (uint32_t i = 0u; i < BCACHE_BLOCKS_PER_PAGE; i++) {
         uint64_t sector_off = p->offset + (uint64_t)i * BCACHE_SECTOR_SIZE;
         uint8_t  dev;
         uint32_t blkno;
 
         int rc = s_map_fn(p->ino, sector_off, &dev, &blkno);
         if (rc != 0) {
             /* Sparse / hole — zero fill */
             memset(p->data + i * BCACHE_SECTOR_SIZE,
                    0, BCACHE_SECTOR_SIZE);
             continue;
         }
 
         /*
          * Call block buffer cache bread().
          * bread() returns a locked BufHdr with 512 bytes of disk data.
          * Copy into our page, then release the buffer.
          */
         BufHdr *b = bread(dev, blkno);
         memcpy(p->data + i * BCACHE_SECTOR_SIZE,
                b->data,
                BCACHE_SECTOR_SIZE);
         brelse(b);
     }
 
     p->flags |= PC_UPTODATE;
     return 0;
 }
 
 /* ── writepage — write dirty page back to block device ─────────────── */
 static int writepage(uiox_page_t *p)
 {
     if (!s_map_fn || !(p->flags & PC_DIRTY)) return 0;
 
     for (uint32_t i = 0u; i < BCACHE_BLOCKS_PER_PAGE; i++) {
         uint64_t sector_off = p->offset + (uint64_t)i * BCACHE_SECTOR_SIZE;
         uint8_t  dev;
         uint32_t blkno;
 
         int rc = s_map_fn(p->ino, sector_off, &dev, &blkno);
         if (rc != 0) continue;
 
                 /*
         * Get a buffer for this block, copy page data into it,
         * then do a synchronous write via bwrite().
         */
        BufHdr *b = getblk(dev, blkno);
        memcpy(b->data,
               p->data + i * BCACHE_SECTOR_SIZE,
               BCACHE_SECTOR_SIZE);
        b->status |= BUF_VALID;
        bwrite(b, true /*sync*/, false /*not delayed*/);
    }

    p->flags &= ~PC_DIRTY;
    return 0;
}

/* ── uiox_pc_init ───────────────────────────────────────────────────── */
void uiox_pc_init(void)
{
    memset(s_pages,     0, sizeof(s_pages));
    memset(s_page_inuse,0, sizeof(s_page_inuse));
    memset(s_hash,      0, sizeof(s_hash));

    /* Sentinel LRU list — points to itself */
    s_lru_head.lru_next = &s_lru_head;
    s_lru_head.lru_prev = &s_lru_head;

    s_map_fn = (uiox_pc_map_fn_t)0;
}

/* ── uiox_pc_register_map ───────────────────────────────────────────── */
void uiox_pc_register_map(uiox_pc_map_fn_t fn)
{
    s_map_fn = fn;
}

/* ── uiox_pc_read ───────────────────────────────────────────────────── */
/*
 * Fill kbuf with 'len' bytes from file inode at byte offset 'offset'.
 *
 * This is the hot path for vfs_read() / sys_read():
 *
 *   Page cache HIT:
 *     memcpy(kbuf, page->data + offset_in_page, len)
 *     → kbuf filled from DRAM  (fast — no device I/O)
 *
 *   Page cache MISS:
 *     allocate page
 *     readpage() → bread() × 8 → fills page->data from device
 *     memcpy(kbuf, page->data + offset_in_page, len)
 *     → kbuf filled from DRAM  (slow first time — device I/O)
 */
ssize_t uiox_pc_read(uint32_t ino, uint64_t offset,
                      void *kbuf, size_t len)
{
    if (!kbuf || len == 0u) return 0;

    size_t   remaining  = len;
    uint8_t *dst        = (uint8_t *)kbuf;
    uint64_t cur_offset = offset;

    while (remaining > 0u) {
        /* Page-align the offset */
        uint64_t page_base = cur_offset &
                             ~((uint64_t)UIOX_PC_PAGE_SIZE - 1u);
        uint32_t page_off  = (uint32_t)(cur_offset - page_base);
        uint32_t can_copy  = UIOX_PC_PAGE_SIZE - page_off;
        if (can_copy > remaining) can_copy = (uint32_t)remaining;

        /* Lookup page in cache */
        uiox_page_t *pg = pc_lookup(ino, page_base);

        if (!pg) {
            /* Cache miss — allocate and fill */
            pg = page_alloc();
            if (!pg) return (ssize_t)-12;  /* ENOMEM */

            pg->ino    = ino;
            pg->offset = page_base;
            pg->flags  = PC_LOCKED;
            pc_insert(pg);

            int rc = readpage(pg);
            pg->flags &= ~PC_LOCKED;
            if (rc != 0) return (ssize_t)rc;
        }

        if (!(pg->flags & PC_UPTODATE))
            return (ssize_t)-5;   /* EIO */

        /*
         * Copy from page cache DRAM page into kernel buffer.
         * This is the DRAM → kbuf copy.
         * The caller (sys_read) then does copy_to_user(ubuf, kbuf, n).
         */
        memcpy(dst, pg->data + page_off, can_copy);

        /* Mark recently used */
        pg->flags |= PC_REFERENCED;

        dst        += can_copy;
        cur_offset += can_copy;
        remaining  -= can_copy;
    }

    return (ssize_t)len;
}

/* ── uiox_pc_write ──────────────────────────────────────────────────── */
/*
 * Write 'len' bytes from kbuf into page cache at file offset.
 * kbuf was already filled by copy_from_user() in sys_write().
 *
 * Write path:
 *   kbuf → page cache DRAM  (immediate, in memory)
 *   page marked dirty
 *   [later] writeback on SYS_SYNC / SYS_FSYNC / pressure
 *           → writepage() → bwrite() → device
 */
ssize_t uiox_pc_write(uint32_t ino, uint64_t offset,
                       const void *kbuf, size_t len)
{
    if (!kbuf || len == 0u) return 0;

    size_t         remaining  = len;
    const uint8_t *src        = (const uint8_t *)kbuf;
    uint64_t       cur_offset = offset;

    while (remaining > 0u) {
        uint64_t page_base = cur_offset &
                             ~((uint64_t)UIOX_PC_PAGE_SIZE - 1u);
        uint32_t page_off  = (uint32_t)(cur_offset - page_base);
        uint32_t can_copy  = UIOX_PC_PAGE_SIZE - page_off;
        if (can_copy > remaining) can_copy = (uint32_t)remaining;

        uiox_page_t *pg = pc_lookup(ino, page_base);

        if (!pg) {
            /* Allocate and read existing content first
             * (for partial-page writes we need the rest of the page) */
            pg = page_alloc();
            if (!pg) return (ssize_t)-12;  /* ENOMEM */

            pg->ino    = ino;
            pg->offset = page_base;
            pg->flags  = PC_LOCKED;
            pc_insert(pg);

            /* Read existing content (read-modify-write) */
            readpage(pg);
            pg->flags &= ~PC_LOCKED;
        }

        /*
         * Copy from kernel buffer into DRAM page.
         * This is the kbuf → DRAM write.
         */
        memcpy(pg->data + page_off, src, can_copy);

        /* Mark page dirty — will be written back on sync */
        pg->flags |= (PC_DIRTY | PC_UPTODATE | PC_REFERENCED);

        src        += can_copy;
        cur_offset += can_copy;
        remaining  -= can_copy;
    }

    return (ssize_t)len;
}

/* ── uiox_pc_get_page_pa ────────────────────────────────────────────── */
/*
 * Returns the physical address of the DRAM page holding
 * file data at offset. Used by sys_mmap() for zero-copy.
 *
 * After this returns, 33_PCS/uiox_sys_mmap.c inserts the PA
 * as a PTE into the user page table — no copy needed at all.
 */
uintptr_t uiox_pc_get_page_pa(uint32_t ino, uint64_t offset)
{
    uint64_t     page_base = offset &
                             ~((uint64_t)UIOX_PC_PAGE_SIZE - 1u);
    uiox_page_t *pg        = pc_lookup(ino, page_base);

    if (!pg) {
        /* Miss — load page first */
        pg = page_alloc();
        if (!pg) return 0u;

        pg->ino    = ino;
        pg->offset = page_base;
        pg->flags  = PC_LOCKED;
        pc_insert(pg);
        readpage(pg);
        pg->flags &= ~PC_LOCKED;
    }

    if (!(pg->flags & PC_UPTODATE)) return 0u;

    /*
     * page->pa is the physical address of page->data[].
     * Since UIOX runs with identity-mapped kernel VA
     * (VA = PA + fixed_offset), this is straightforward:
     *
     *   ARM64: PA = VA - 0xFFFF_0000_0000_0000 + DRAM_base
     *   x86-64: PA = VA - 0xFFFF_8000_0000_0000
     *
     * For the page pool (static array in kernel BSS),
     * the VA→PA conversion is handled by 33_PCS/02_MemMngnt.
     * Here we return the VA stored in pg->pa — the caller
     * uses uiox_virt_to_phys() before inserting the PTE.
     */
    pg->flags |= PC_REFERENCED;
    return pg->pa;
}

/* ── uiox_pc_writeback ──────────────────────────────────────────────── */
/*
 * Write back all dirty pages for a single inode.
 * Called by vfs_fsync() / SYS_FSYNC.
 */
int uiox_pc_writeback(uint32_t ino)
{
    int rc = 0;
    for (uint32_t i = 0u; i < UIOX_PC_MAX_PAGES; i++) {
        uiox_page_t *pg = &s_pages[i];
        if (!s_page_inuse[i])        continue;
        if (pg->ino   != ino)        continue;
        if (!(pg->flags & PC_DIRTY)) continue;

        pg->flags |= PC_LOCKED;
        int r = writepage(pg);
        pg->flags &= ~PC_LOCKED;
        if (r != 0) rc = r;
    }
    /* Also flush block buffer cache delayed writes */
    bflush(0u);
    return rc;
}

/* ── uiox_pc_writeback_all ──────────────────────────────────────────── */
/*
 * Write back all dirty pages for all inodes.
 * Called by SYS_SYNC / uiox_jr_checkpoint().
 */
int uiox_pc_writeback_all(void)
{
    int rc = 0;
    for (uint32_t i = 0u; i < UIOX_PC_MAX_PAGES; i++) {
        uiox_page_t *pg = &s_pages[i];
        if (!s_page_inuse[i])        continue;
        if (!(pg->flags & PC_DIRTY)) continue;

        pg->flags |= PC_LOCKED;
        int r = writepage(pg);
        pg->flags &= ~PC_LOCKED;
        if (r != 0) rc = r;
    }
    /* Flush block buffer cache for all devices */
    for (uint8_t dev = 0u; dev < MAX_DEVICES; dev++)
        bflush(dev);
    return rc;
}

/* ── uiox_pc_invalidate ─────────────────────────────────────────────── */
/*
 * Evict all pages for an inode from the cache.
 * Called on file truncation or final close.
 */
void uiox_pc_invalidate(uint32_t ino)
{
    for (uint32_t i = 0u; i < UIOX_PC_MAX_PAGES; i++) {
        uiox_page_t *pg = &s_pages[i];
        if (!s_page_inuse[i])  continue;
        if (pg->ino != ino)    continue;
        if (pg->flags & PC_LOCKED) continue;  /* skip in-flight I/O */

        /* Remove from hash */
        uint32_t slot = pc_hash(pg->ino, pg->offset);
        uiox_page_t **pp = &s_hash[slot];
        while (*pp && *pp != pg) pp = &(*pp)->hash_next;
        if (*pp) *pp = pg->hash_next;

        /* Remove from LRU */
        pg->lru_prev->lru_next = pg->lru_next;
        pg->lru_next->lru_prev = pg->lru_prev;

        s_page_inuse[i] = 0u;
        memset(pg, 0, sizeof(*pg));
    }
}

 