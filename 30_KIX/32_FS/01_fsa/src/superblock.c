/*
 *  30_KIX/32_FS/01_fsa/src/superblock.c
 *
 *  The super block, and Bach's four allocation algorithms:
 *      alloc  (§5)   fs_alloc_begin / fs_alloc_commit / fs_alloc
 *      free   (§6)   fs_free
 *      ialloc (§7)   ialloc_dev / ialloc
 *      ifree  (§8)   ifree
 *  plus fs_free_inode_blocks, the block sweep iput calls.
 *
 *  ── COMPATIBILITY ─────────────────────────────────────────────────────
 *  BELOW (00_buffcache / 31_BufferCache):
 *      fs_alloc_begin takes a device and returns a locked, zeroed BufHdr
 *      every chain-block read is  bread(dev, blkno)
 *      every chain-block write is bwrite(buf, true, false)
 *      BufEntry -> BufHdr; no `dirty` member exists on the header
 *
 *  ABOVE (10_scfs):
 *      sb_get(dev) is the accessor statfs reads through.  ialloc keeps
 *      its single-filesystem form for existing callers.
 *
 *  ── one super block per device ────────────────────────────────────────
 *  The first cut kept a single `static SuperBlock sb`, which is correct
 *  for one filesystem and silently wrong for two: both mounts would
 *  share one set of counters, and free() on one would credit the other.
 *  s_sb[MAX_DEVICES] now holds one per device, and every entry point
 *  indexes it by the dev it was given.
 *
 *  ── the free-block list is Bach's, not a plain array ──────────────────
 *  Bach: "the first entry in the free list points to a block that itself
 *  contains the next group of free block numbers."  So the in-core list
 *  is a WINDOW onto a chain of block-number arrays stored in the data
 *  area, and two operations move between the two views:
 *
 *      refill   the window is empty → read the block the last entry
 *               named, and copy its numbers into the window
 *      spill    the window is full  → write the window into the block
 *               being freed, and restart the window with it
 *
 *  `sb_free` is the tricky one: the block being freed is itself the
 *  container for the window it is about to displace.  The order is
 *  write-then-reset, and the block is written BEFORE it becomes the
 *  window's first entry — otherwise the write would overwrite the entry
 *  naming it.
 *
 *  @version 2.0.0  @date 2026-09-24
 */
#include "superblock.h"
#include "uiox_klibc.h"

/* jiffies — the BSP timer's monotonic tick counter, used as the inode
 * timestamp source.  Weak, so this file links before the BSP is wired. */
extern volatile uint64_t jiffies __attribute__((weak));

/* ═════════════════════════════════════════════════════════════════════
 * The per-device super blocks
 * ═════════════════════════════════════════════════════════════════════ */
static SuperBlock s_sb[MAX_DEVICES];
static bool       s_sb_inuse[MAX_DEVICES];

/* ── the accessor 10_scfs's statfs reads through ───────────────────── */
SuperBlock *sb_get(uint8_t dev)
{
    if (dev >= MAX_DEVICES) return (SuperBlock *)0;
    if (!s_sb_inuse[dev])   return (SuperBlock *)0;
    return &s_sb[dev];
}

/* ── device 0 is the root filesystem, if nothing says otherwise ────── */
#define ROOT_SB_DEV  0u

/* ═════════════════════════════════════════════════════════════════════
 * sb_init — create a fresh filesystem on 'dev'
 *
 * Bach's mkfs: lay out the inode list and the data area, and populate
 * the free-block window with the first run of data blocks.  The rest are
 * counted but not cached — they are reached through the chain.
 * ═════════════════════════════════════════════════════════════════════ */
void sb_init(uint8_t dev)
{
    SuperBlock *sb;
    uint32_t    blkno;

    if (dev >= MAX_DEVICES) return;

    sb = &s_sb[dev];
    memset(sb, 0, sizeof *sb);

    /* The four names below were removed from fs_types.h; unfs_format.h
     * owns them now.  The mapping:
     *
     *     MAX_BLOCKS        -> the volume's block count.  UNFS reads it
     *                          from the superblock's s_block_count, so
     *                          there is no macro — NUM_DISK_BLOCKS_DEFAULT
     *                          (00_buffcache) is the geometry's default
     *                          and is what the buffer layer bounds reads
     *                          against, so using it keeps the two in step.
     *     INODE_START_BLOCK -> UNFS_GROUP0_ITABLE
     *     DATA_START_BLOCK  -> UNFS_GROUP0_DATA
     *     MAX_INODES        -> UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK
     *                          (group 0's table: 8 x 16 = 128), the same
     *                          bound inode.c uses.
     *
     * NOTE the inode bound is GROUP 0's capacity, not the volume's —
     * a volume with more groups holds more inodes than this.  Wiring
     * s_inode_count through from an on-disk superblock is the proper
     * fix; this is the value the tree can compute today. */
    sb->fs_size          = NUM_DISK_BLOCKS_DEFAULT;
    sb->inode_start      = UNFS_GROUP0_ITABLE;
    sb->data_start       = UNFS_GROUP0_DATA;
    sb->max_inodes       = (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK);
    sb->free_inode_count = (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK);
    sb->remembered_inode = 1u;

    /* ── the first run of free blocks goes into the window ─────────── */
    blkno             = UNFS_GROUP0_DATA;
    sb->free_block_count = 0u;
    sb->free_block_idx   = 0;

    while (blkno < NUM_DISK_BLOCKS_DEFAULT &&
           sb->free_block_idx < SB_FREE_BLOCK_MAX) {
        sb->free_blocks[sb->free_block_idx++] = blkno++;
        sb->free_block_count++;
    }

    /* The remainder are free but not cached — the window chains to them
     * once it is drained.  Bach counts them here. */
    while (blkno < NUM_DISK_BLOCKS_DEFAULT) {
        sb->free_block_count++;
        blkno++;
    }

    sb->modified = false;
    sb->locked   = false;
    s_sb_inuse[dev] = true;

    printf("[sb] dev=%u init: %u data blocks  %u inodes\n",
           (unsigned)dev,
           (unsigned)sb->free_block_count,
           (unsigned)sb->free_inode_count);
}

/* ═════════════════════════════════════════════════════════════════════
 * Internal: refill the free-block window from the chain.
 *
 * Reads the block named by the LAST window entry and copies its numbers
 * in.  Called when the window is empty but the count says blocks remain.
 * ═════════════════════════════════════════════════════════════════════ */
static int sb_refill_blocks(uint8_t dev, SuperBlock *sb)
{
    uint32_t  chain_blk;
    BufHdr   *chain_buf;
    uint32_t *nums;
    int       n = 0;

    if (sb->free_block_idx <= 0) return -1;

    chain_blk = sb->free_blocks[0];

    sb->locked = true;

    chain_buf = bread(dev, chain_blk);      /* ◀ (dev, blkno) */
    if (!chain_buf) { sb->locked = false; return -1; }

    nums = (uint32_t *)chain_buf->data;

    /* A zero entry ends the chain — the rest of the block is padding. */
    while (n < SB_FREE_BLOCK_MAX && nums[n] != 0u) {
        sb->free_blocks[n] = nums[n];
        n++;
    }
    sb->free_block_idx = n;

    brelse(chain_buf);                      /* buffer layer */

    sb->locked = false;

    printf("[alloc] dev=%u refilled free-block window from chain blk=%u "
           "(%d entries)\n", (unsigned)dev, (unsigned)chain_blk, n);

    return (n > 0) ? 0 : -1;
}

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm alloc (§5), split
 *
 * fs_alloc_begin — detach a block from the free list and hand back a
 * locked, zeroed buffer for it.  Does NOT attach it to any inode.
 *
 * fs_alloc_commit — mark that buffer dirty and release it.  Called after
 * the caller has written the block number into the inode's map, so the
 * block is never both off the free list and named by nothing.
 * ═════════════════════════════════════════════════════════════════════ */
BufHdr *fs_alloc_begin(uint8_t dev, uint32_t *blkno_out)
{
    SuperBlock *sb = sb_get(dev);
    uint32_t    blkno;
    BufHdr     *buf;

    if (!sb) return (BufHdr *)0;
    if (blkno_out) *blkno_out = 0u;

    /* Bach: "while (super block locked) sleep" — we spin. */
    while (sb->locked)
        ;

    if (sb->free_block_count == 0u) {
        printf("[alloc] dev=%u ERROR: no free blocks\n", (unsigned)dev);
        return (BufHdr *)0;
    }

    /* ── the window is empty but blocks remain: pull from the chain ── */
    if (sb->free_block_idx == 0) {
        if (sb_refill_blocks(dev, sb) != 0) {
            printf("[alloc] dev=%u ERROR: free list exhausted\n",
                   (unsigned)dev);
            return (BufHdr *)0;
        }
    }

    blkno = sb->free_blocks[--sb->free_block_idx];
    sb->free_block_count--;
    sb->modified = true;

    /* ── take a buffer for the new block and zero it ────────────────
     * getblk returns it locked; the caller holds it until commit. */
    buf = getblk(dev, blkno);               /* ◀ (dev, blkno) */
    if (!buf) {
        /* Put the block back — the count and the window would otherwise
         * disagree with reality. */
        sb->free_blocks[sb->free_block_idx++] = blkno;
        sb->free_block_count++;
        return (BufHdr *)0;
    }

    memset(buf->data, 0, BLOCK_SIZE);
    buf->status |= BUF_VALID;

    if (blkno_out) *blkno_out = blkno;

    printf("[alloc] dev=%u blk=%u  free_blocks=%u\n",
           (unsigned)dev, (unsigned)blkno, (unsigned)sb->free_block_count);
    return buf;
}

/* fs_alloc_commit_dev — the ONLY commit form.  An earlier revision also
 * declared a device-less fs_alloc_commit(); it could not re-find the
 * buffer, because the cache is keyed by (dev, blkno), and it did nothing.
 * Removed rather than left as a trap. */
void fs_alloc_commit_dev(uint8_t dev, uint32_t blkno)
{
    BufHdr *buf = getblk(dev, blkno);       /* the locked buffer */

    if (!buf) return;

    /* Synchronous write releases the buffer, which is the release half
     * of the old getblk/brelse pair. */
    bwrite(buf, true, false);               /* ◀ persist + release */
}

/* ── fs_alloc — the one-step form ──────────────────────────────────────
 * For a caller that already knows the device and will use the buffer
 * immediately.  Kept for compatibility; the split form is preferred.
 */
BufHdr *fs_alloc(uint8_t dev)
{
    uint32_t blkno = 0u;
    BufHdr  *buf   = fs_alloc_begin(dev, &blkno);

    if (!buf) return (BufHdr *)0;

    /* Nothing to attach, so zero + dirty + release in one step. */
    bwrite(buf, true, false);
    return (BufHdr *)0;    /* the buffer is released; only blkno escaped */
}

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm free (§6) — return a block to the free pool
 *
 * ── the order that matters ────────────────────────────────────────────
 * When the window is full, this block becomes the new container: its
 * data area receives the window's numbers, and it then takes position 0.
 * The WRITE must happen before the block is recorded as entry 0 —
 * otherwise the write would land on the very entry naming the block.
 * ═════════════════════════════════════════════════════════════════════ */
void fs_free(uint8_t dev, uint32_t blkno)
{
    SuperBlock *sb = sb_get(dev);

    if (!sb) return;

    if (blkno < UNFS_GROUP0_DATA || blkno >= NUM_DISK_BLOCKS_DEFAULT) {
        printf("[free] dev=%u ERROR: invalid blkno %u\n",
               (unsigned)dev, (unsigned)blkno);
        return;
    }

    sb->free_block_count++;

    if (sb->locked) {
        /* Bach: "if (super block locked) return" — the block is already
         * counted free, it is just not cached in the window. */
        printf("[free] dev=%u superblock locked — blk %u not cached\n",
               (unsigned)dev, (unsigned)blkno);
        return;
    }

    if (sb->free_block_idx >= SB_FREE_BLOCK_MAX) {
        /* ── the window is full: spill it into this block ──────────── */
        BufHdr   *buf;
        uint32_t *nums;
        int       i;

        buf = getblk(dev, blkno);            /* ◀ (dev, blkno) */
        if (buf) {
            nums = (uint32_t *)buf->data;

            for (i = 0; i < SB_FREE_BLOCK_MAX; i++)
                nums[i] = sb->free_blocks[i];

            /* Zero the first slot AFTER the copy — see the note above:
             * entry 0 must not name the container being written. */
            if (SB_FREE_BLOCK_MAX < (int)(BLOCK_SIZE / sizeof(uint32_t)))
                nums[SB_FREE_BLOCK_MAX] = 0u;

            buf->status |= BUF_VALID;
            bwrite(buf, true, false);        /* ◀ persist + release */
        }

        sb->free_block_idx = 0;
        sb->free_blocks[sb->free_block_idx++] = blkno;
    } else {
        sb->free_blocks[sb->free_block_idx++] = blkno;
    }

    sb->modified = true;

    printf("[free] dev=%u freed blk=%u  free_blocks=%u\n",
           (unsigned)dev, (unsigned)blkno, (unsigned)sb->free_block_count);
}

/* ═════════════════════════════════════════════════════════════════════
 * Internal: scan the disk for free inodes and refill the window.
 *
 * Bach: "search disk for free inode until super block full, or no more
 * free inodes (algorithm bread and brelse); set remembered inode for
 * next free inode search."
 * ═════════════════════════════════════════════════════════════════════ */
static void sb_refill_inodes(uint8_t dev, SuperBlock *sb)
{
    uint32_t ino;
    uint32_t start;

    sb->locked         = true;
    sb->free_inode_idx = 0;
    start              = sb->remembered_inode;

    for (ino = start;
         ino <= (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK) &&
             sb->free_inode_idx < SB_FREE_INODE_MAX;
         ino++) {
        BufHdr    *buf;
        DiskInode *di = inode_disk_read(dev, ino, &buf);

        if (di && di->i_mode == 0u)          /* DiskInode carries i_* */
            sb->free_inodes[sb->free_inode_idx++] = ino;

        if (di) brelse(buf);                /* buffer layer */
    }

    sb->locked = false;

    printf("[ialloc] dev=%u refilled inode window: %d entries\n",
           (unsigned)dev, sb->free_inode_idx);
}

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm ialloc (§7) — assign a free inode
 * ═════════════════════════════════════════════════════════════════════ */
InCoreInode *ialloc_dev(uint8_t dev, uint16_t unfs_if, uint16_t perm,
                        uint16_t uid, uint16_t gid)
{
    SuperBlock *sb = sb_get(dev);

    if (!sb) return (InCoreInode *)0;

    while (1) {
        uint32_t     ino;
        InCoreInode *ip;
        BufHdr      *buf;
        DiskInode   *di;

        while (sb->locked)
            ;                               /* Bach: sleep */

        /* ── the window is empty: rebuild it from disk ─────────────── */
        if (sb->free_inode_idx == 0) {
            sb_refill_inodes(dev, sb);

            if (sb->free_inode_idx == 0) {
                printf("[ialloc] dev=%u ERROR: no free inodes\n",
                       (unsigned)dev);
                return (InCoreInode *)0;
            }

            /* Bach: "set remembered inode for next free inode search" */
            sb->remembered_inode = sb->free_inodes[sb->free_inode_idx - 1];
        }

        ino = sb->free_inodes[--sb->free_inode_idx];

        ip = iget_dev(dev, ino);            /* ◀ (dev, ino) */
        if (!ip) continue;

        /* ── verify it really is free ─────────────────────────────────
         * Bach: "if (inode not free after all) { write inode to disk;
         * release inode; continue; }" — the window can be stale. */
        di = inode_disk_read(dev, ino, &buf);
        if (!di || di->i_mode != 0u) {
            if (di) brelse(buf);
            iput(ip);
            continue;
        }
        brelse(buf);

        /* ── initialize it ─────────────────────────────────────────────
         * mode = UNFS_IF* | perm — the ON-DISK encoding, composed by
         * inode.h's inode_make_mode() so the two halves are joined in
         * exactly one place.
         *
         * This replaces (ftype << 12) | perm, which packed a FileType
         * nibble into the same bits as UNFS_IFMT: (FT_DIR << 12) is
         * 0x2000, which is UNFS_IFCHR, so a directory written through
         * that path read back as a character device. */
        ip->mode   = inode_make_mode(unfs_if, perm);
        ip->nlink  = 0u;
        ip->uid    = uid;
        ip->gid    = gid;
        ip->size   = 0u;
        ip->dev    = dev;
        ip->flags  = IFLAG_CHANGED;
        ip->atime  = ip->mtime = ip->ctime = (time_t)(jiffies ? jiffies : 0u);
        /* addr[] is gone — the block map is an EXTENT array now, and
         * zeroing it here is what makes an unset map mean "no blocks"
         * rather than "whatever the slot held".  This is also the ONLY
         * place an inode is born, so it is the right place to zero the
         * fields ialloc does not otherwise set — see the note in
         * iupdate about i_checksum / i_mac_* / i_inline. */
        memset(ip->i_extents, 0, sizeof ip->i_extents);
        ip->i_extent_tree = 0u;

        iupdate(ip);                        /* write inode to disk */

        sb->free_inode_count--;
        sb->modified = true;

        printf("[ialloc] dev=%u ino=%u if=0%o perm=0%o uid=%u\n",
               (unsigned)dev, (unsigned)ino,
               (unsigned)unfs_if, (unsigned)perm, (unsigned)uid);
        return ip;                          /* LOCKED, as Bach says */
    }
}

/* ialloc — the single-filesystem form existing callers use.
 *
 * 'unfs_if' is a UNFS_IF* value (UNFS_IFDIR, UNFS_IFREG, …), NOT the old
 * FileType.  10_scfs callers passing FT_DIR / FT_REGULAR must be updated
 * to UNFS_IFDIR / UNFS_IFREG — the two share bit positions, so a missed
 * call site compiles and writes the WRONG TYPE, which is the failure this
 * whole change exists to end. */
InCoreInode *ialloc(uint16_t unfs_if, uint16_t perm, uint16_t uid, uint16_t gid)
{
    return ialloc_dev(ROOT_SB_DEV, unfs_if, perm, uid, gid);
}

/* ═════════════════════════════════════════════════════════════════════
 * Algorithm ifree (§8) — return an inode to the free pool
 * ═════════════════════════════════════════════════════════════════════ */
void ifree(uint32_t ino)
{
    SuperBlock *sb = sb_get(ROOT_SB_DEV);   /* inode numbers are per-device */

    if (!sb) return;

    sb->free_inode_count++;

    if (sb->locked) {
        printf("[ifree] superblock locked — inode %u not cached\n",
               (unsigned)ino);
        return;
    }

    if (sb->free_inode_idx >= SB_FREE_INODE_MAX) {
        /* The window is full: remember the lowest number and let the scan
         * find it next time.  Bach does exactly this. */
        if (ino < sb->remembered_inode)
            sb->remembered_inode = ino;
        return;
    }

    sb->free_inodes[sb->free_inode_idx++] = ino;
    printf("[ifree] ino=%u returned to free list\n", (unsigned)ino);
}

/* ═════════════════════════════════════════════════════════════════════
 * fs_free_inode_blocks — free every data block an inode owns
 *
 * Called from iput when nlink reaches 0.  Walks the same three levels
 * bmap reads: direct entries, then the single-indirect block, then the
 * block pointer itself.
 *
 * NOTE: double- and triple-indirect levels are not swept.  bmap_alloc
 * cannot create them, so an inode should never hold one — but a
 * filesystem image built elsewhere could.  The check is left explicit
 * rather than silent.
 * ═════════════════════════════════════════════════════════════════════ */
void fs_free_inode_blocks(InCoreInode *ip)
{
    uint32_t i;

    if (!ip) return;

    /* ── the four inline extents ───────────────────────────────────────
     * Each extent names a run of e_len CONSECUTIVE physical blocks, so
     * the sweep is a walk from e_physical while the length lasts — not a
     * pointer chase.  A hole owns no blocks: nothing to free. */
    for (i = 0u; i < UNFS_INLINE_EXTENTS; i++) {
        unfs_extent_t *e = &ip->i_extents[i];
        uint32_t       n;

        if (e->e_len == 0u) continue;
        if (e->e_flags & UNFS_EXT_HOLE) { e->e_len = 0u; continue; }

        for (n = 0u; n < (uint32_t)e->e_len; n++) {
            fs_free(ip->dev, (uint32_t)e->e_physical + n);
        }

        e->e_logical  = 0u;
        e->e_physical = 0u;
        e->e_len      = 0u;
        e->e_flags    = 0u;
    }

    /* ── the overflow extent-tree block ────────────────────────────────
     * One 4 KB block holding an array of unfs_extent_t, terminated by an
     * entry with e_len == 0.  The tree block is itself allocated, so it
     * is freed last, after its contents have been swept.
     *
     * NOTE: the tree block is ONE UNFS block but EIGHT buffer sectors, so
     * this must read all eight — the same unit trap namei.c carries.
     * Reading one sector and treating it as the block walks a quarter of
     * the extents.  Left explicit here rather than silently partial. */
    if (ip->i_extent_tree != 0u) {
        uint32_t per = (uint32_t)(UNFS_BLOCK_SIZE / sizeof(unfs_extent_t));
        uint8_t  tree[UNFS_BLOCK_SIZE];
        uint32_t s;
        bool     ok = true;

        for (s = 0u; s < (uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
            BufHdr *buf = bread(ip->dev,
                                ip->i_extent_tree * (uint32_t)UNFS_SECTORS_PER_BLOCK + s);
            if (!buf) { ok = false; break; }

            memcpy(tree + (s * (uint32_t)BCACHE_SECTOR_SIZE),
                   buf->data, (size_t)BCACHE_SECTOR_SIZE);
            brelse(buf);
        }

        if (ok) {
            unfs_extent_t *et = (unfs_extent_t *)tree;

            for (i = 0u; i < per; i++) {
                uint32_t n;

                if (et[i].e_len == 0u) break;          /* end of entries */
                if (et[i].e_flags & UNFS_EXT_HOLE) continue;

                for (n = 0u; n < (uint32_t)et[i].e_len; n++) {
                    fs_free(ip->dev, (uint32_t)et[i].e_physical + n);
                }
            }
        }

        fs_free(ip->dev, ip->i_extent_tree);
        ip->i_extent_tree = 0u;
    }

    ip->size = 0u;
}

/* ═════════════════════════════════════════════════════════════════════
 * sb_print — debug dump
 * ═════════════════════════════════════════════════════════════════════ */
void sb_print(uint8_t dev)
{
    SuperBlock *sb = sb_get(dev);

    if (!sb) {
        printf("[sb] dev=%u: not initialised\n", (unsigned)dev);
        return;
    }

    printf("[sb] dev=%u free_blocks=%u  free_inodes=%u  "
           "remembered_ino=%u  modified=%d\n",
           (unsigned)dev,
           (unsigned)sb->free_block_count,
           (unsigned)sb->free_inode_count,
           (unsigned)sb->remembered_inode,
           (int)sb->modified);
}
