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

    sb->fs_size          = MAX_BLOCKS;
    sb->inode_start      = INODE_START_BLOCK;
    sb->data_start       = DATA_START_BLOCK;
    sb->max_inodes       = MAX_INODES;
    sb->free_inode_count = MAX_INODES;
    sb->remembered_inode = 1u;

    /* ── the first run of free blocks goes into the window ─────────── */
    blkno             = DATA_START_BLOCK;
    sb->free_block_count = 0u;
    sb->free_block_idx   = 0;

    while (blkno < MAX_BLOCKS && sb->free_block_idx < SB_FREE_BLOCK_MAX) {
        sb->free_blocks[sb->free_block_idx++] = blkno++;
        sb->free_block_count++;
    }

    /* The remainder are free but not cached — the window chains to them
     * once it is drained.  Bach counts them here. */
    while (blkno < MAX_BLOCKS) {
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

    if (blkno < DATA_START_BLOCK || blkno >= MAX_BLOCKS) {
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
         ino <= MAX_INODES && sb->free_inode_idx < SB_FREE_INODE_MAX;
         ino++) {
        BufHdr    *buf;
        DiskInode *di = inode_disk_read(dev, ino, &buf);

        if (di && di->mode == 0u)
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
InCoreInode *ialloc_dev(uint8_t dev, FileType ftype, uint16_t perm,
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
        if (!di || di->mode != 0u) {
            if (di) brelse(buf);
            iput(ip);
            continue;
        }
        brelse(buf);

        /* ── initialize it ─────────────────────────────────────────────
         * mode = (ftype << 12) | perm, matching inode_type()'s read. */
        ip->mode   = (uint16_t)(((uint16_t)ftype << 12) | (perm & 0x1FFu));
        ip->nlink  = 0u;
        ip->uid    = uid;
        ip->gid    = gid;
        ip->size   = 0u;
        ip->dev    = dev;
        ip->flags  = IFLAG_CHANGED;
        ip->atime  = ip->mtime = ip->ctime = (time_t)(jiffies ? jiffies : 0u);
        memset(ip->addr, 0, sizeof ip->addr);

        iupdate(ip);                        /* write inode to disk */

        sb->free_inode_count--;
        sb->modified = true;

        printf("[ialloc] dev=%u ino=%u type=%d perm=0%o uid=%u\n",
               (unsigned)dev, (unsigned)ino, (int)ftype,
               (unsigned)perm, (unsigned)uid);
        return ip;                          /* LOCKED, as Bach says */
    }
}

/* ialloc — the single-filesystem form existing callers use. */
InCoreInode *ialloc(FileType ftype, uint16_t perm, uint16_t uid, uint16_t gid)
{
    return ialloc_dev(ROOT_SB_DEV, ftype, perm, uid, gid);
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

    /* ── direct ─────────────────────────────────────────────────────── */
    for (i = 0u; i < (uint32_t)NDIRECT; i++) {
        if (ip->addr[i]) {
            fs_free(ip->dev, ip->addr[i]);
            ip->addr[i] = 0u;
        }
    }

    /* ── single indirect: free the data blocks, then the container ──── */
    if (ip->addr[NDIRECT]) {
        BufHdr   *buf = bread(ip->dev, ip->addr[NDIRECT]);   /* ◀ (dev,blk) */

        if (buf) {
            uint32_t *ptrs = (uint32_t *)buf->data;
            uint32_t  j;

            for (j = 0u; j < (uint32_t)PTRS_PER_BLOCK; j++) {
                if (ptrs[j]) {
                    fs_free(ip->dev, ptrs[j]);
                    ptrs[j] = 0u;
                }
            }

            /* Zero the container's pointers before releasing it — leaving
             * dangling numbers in a block that is about to be freed is
             * the same defect truncate.c was fixed for. */
            bwrite(buf, true, false);        /* ◀ persist + release */
        }

        fs_free(ip->dev, ip->addr[NDIRECT]);
        ip->addr[NDIRECT] = 0u;
    }

    /* ── the levels bmap_alloc cannot build ────────────────────────── */
    if (ip->addr[NDIRECT + NINDIRECT] ||
        ip->addr[NDIRECT + NINDIRECT + NDINDIRECT]) {
        printf("[free] WARNING: ino=%u holds a double/triple-indirect "
               "block — not swept (bmap_alloc cannot create one)\n",
               (unsigned)ip->ino);
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
