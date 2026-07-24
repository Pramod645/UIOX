/*
 *  30_KIX/32_FS/01_fsa/src/superblock.c
 *
 *  Freestanding fixes (v1.1):
 *    FIXED: #include "/Users/.../uiox_klibc.h"  →  #include "uiox_klibc.h"
 *    FIXED: fprintf(stderr, ...)                 →  printf(...)   (4 occurrences)
 *    FIXED: time(NULL)                           →  (int64_t)jiffies
 */
#include "superblock.h"
#include "uiox_klibc.h"

/* jiffies — monotonic tick counter provided by the BSP timer layer.
 * Declared weak so this file links standalone before the BSP is wired. */
extern volatile uint64_t jiffies __attribute__((weak));

static SuperBlock sb;

/* ─────────────────────────────────────────────────────────────
 * sb_init — create a fresh simulated filesystem
 * ───────────────────────────────────────────────────────────── */
void sb_init(void)
{
    uint32_t blkno;
    memset(&sb, 0, sizeof sb);
    sb.fs_size          = MAX_BLOCKS;
    sb.inode_start      = INODE_START_BLOCK;
    sb.data_start       = DATA_START_BLOCK;
    sb.max_inodes       = MAX_INODES;
    sb.free_inode_count = MAX_INODES;
    sb.remembered_inode = 1;

    /* Populate free block list with blocks DATA_START_BLOCK..MAX_BLOCKS-1 */
    blkno = DATA_START_BLOCK;
    sb.free_block_count = 0;
    sb.free_block_idx   = 0;

    while (blkno < MAX_BLOCKS && sb.free_block_idx < SB_FREE_BLOCK_MAX) {
        sb.free_blocks[sb.free_block_idx++] = blkno++;
        sb.free_block_count++;
    }
    /* Remaining free blocks counted but not cached in the in-core list */
    while (blkno < MAX_BLOCKS) {
        sb.free_block_count++;
        blkno++;
    }

    sb.modified = false;
    sb.locked   = false;

    printf("[sb] init: %u data blocks  %u inodes\\n",
           sb.free_block_count, sb.free_inode_count);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm alloc  (§5)
 * ───────────────────────────────────────────────────────────── */
BufEntry *fs_alloc(void)
{
    uint32_t blkno;

    /* Wait if superblock locked (simulated: just warn) */
    while (sb.locked)
        printf("[alloc] superblock locked — waiting\\n");

    if (sb.free_block_count == 0) {
        printf("[alloc] ERROR: no free blocks\\n");
        return NULL;
    }

    if (sb.free_block_idx == 0) {
        /*
         * The block we took actually holds the NEXT group of free
         * block numbers — read it and refill the in-core list.
         */
        blkno = sb.free_blocks[0];
        sb.locked = true;
        {
            BufEntry *chain_buf = bread(blkno);
            if (chain_buf) {
                uint32_t *nums = (uint32_t *)chain_buf->data;
                int        n   = 0;
                while (n < SB_FREE_BLOCK_MAX && nums[n] != 0) {
                    sb.free_blocks[n] = nums[n];
                    n++;
                }
                sb.free_block_idx = n;
                brelse(chain_buf);
            }
        }
        sb.locked = false;
        printf("[alloc] refilled free-block list from chain block %u\\n", blkno);

        if (sb.free_block_idx == 0) {
            printf("[alloc] ERROR: filesystem full\\n");
            return NULL;
        }
        blkno = sb.free_blocks[--sb.free_block_idx];
    } else {
        blkno = sb.free_blocks[--sb.free_block_idx];
    }

    sb.free_block_count--;
    sb.modified = true;

    /* Get a buffer for the newly allocated block; zero it */
    {
        BufEntry *buf = getblk(blkno);
        if (!buf) return NULL;
        memset(buf->data, 0, BLOCK_SIZE);
        buf->valid = true;
        buf->dirty = true;
        printf("[alloc] blk=%u  free_blocks=%u\\n", blkno, sb.free_block_count);
        return buf;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm free  (§6)
 * ───────────────────────────────────────────────────────────── */
void fs_free(uint32_t blkno)
{
    if (blkno < DATA_START_BLOCK || blkno >= MAX_BLOCKS) {
        printf("[free] ERROR: invalid blkno %u\\n", blkno);
        return;
    }

    sb.free_block_count++;

    if (sb.locked) {
        printf("[free] superblock locked — block %u not cached\\n", blkno);
        return;
    }

    if (sb.free_block_idx >= SB_FREE_BLOCK_MAX) {
        /*
         * In-core free-block list is full — write it out as a chain
         * block and restart the list with this block as the first entry.
         */
        BufEntry *buf = getblk(blkno);
        if (buf) {
            uint32_t *nums = (uint32_t *)buf->data;
            int        i;
            for (i = 0; i < SB_FREE_BLOCK_MAX; i++)
                nums[i] = sb.free_blocks[i];
            buf->dirty = true;
            bwrite(buf);
            brelse(buf);
        }
        sb.free_block_idx   = 0;
        sb.free_blocks[sb.free_block_idx++] = blkno;
    } else {
        sb.free_blocks[sb.free_block_idx++] = blkno;
    }

    sb.modified = true;
    printf("[free] freed blk=%u  free_blocks=%u\\n",
           blkno, sb.free_block_count);
}

/* ─────────────────────────────────────────────────────────────
 * Internal: scan disk for free inodes and refill sb list
 * ───────────────────────────────────────────────────────────── */
static void sb_refill_inode_list(void)
{
    uint32_t ino;
    sb.locked         = true;
    sb.free_inode_idx = 0;
    {
        uint32_t start = sb.remembered_inode;

        for (ino = start;
             ino <= MAX_INODES &&
             sb.free_inode_idx < SB_FREE_INODE_MAX; ino++) {
            BufEntry  *buf;
            DiskInode *di = inode_disk_read(ino, &buf);
            if (di && di->mode == 0)
                sb.free_inodes[sb.free_inode_idx++] = ino;
            if (di) brelse(buf);
        }
    }
    sb.locked = false;
    printf("[ialloc] refilled inode list: %d entries\\n", sb.free_inode_idx);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm ialloc  (§7)
 * ───────────────────────────────────────────────────────────── */
InCoreInode *ialloc(FileType ftype, uint16_t perm,
                     uint16_t uid, uint16_t gid)
{
    while (1) {
        while (sb.locked) {
            printf("[ialloc] superblock locked — waiting\\n");
            break; /* sim: proceed */
        }

        if (sb.free_inode_idx == 0) {
            sb_refill_inode_list();
            if (sb.free_inode_idx == 0) {
                printf("[ialloc] ERROR: no free inodes\\n");
                return NULL;
            }
        }

        {
            uint32_t     ino = sb.free_inodes[--sb.free_inode_idx];
            InCoreInode *ip  = iget(ino);
            if (!ip) continue;

            /* Verify it is actually free (mode == 0 on disk) */
            {
                BufEntry  *buf;
                DiskInode *di = inode_disk_read(ino, &buf);
                if (!di || di->mode != 0) {
                    if (di) brelse(buf);
                    iput(ip);
                    continue;
                }
                brelse(buf);
            }

            /* Initialise */
            ip->mode   = (uint16_t)(((uint16_t)ftype << 12) | (perm & 0x1FFU));
            ip->nlink  = 0;
            ip->uid    = uid;
            ip->gid    = gid;
            ip->size   = 0;
            ip->flags  = IFLAG_CHANGED;
            /* time(NULL) replaced — use jiffies (monotonic tick counter) */
            ip->atime  = ip->mtime = ip->ctime = (int64_t)(jiffies ? jiffies : 0);
            memset(ip->addr, 0, sizeof ip->addr);

            iupdate(ip);

            sb.free_inode_count--;
            sb.modified = true;

            printf("[ialloc] ino=%u  type=%d  perm=0%o  uid=%u\\n",
                   ino, (int)ftype, perm, uid);
            return ip;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm ifree  (§8)
 * ───────────────────────────────────────────────────────────── */
void ifree(uint32_t ino)
{
    sb.free_inode_count++;

    if (sb.locked) {
        printf("[ifree] superblock locked — inode %u not cached\\n", ino);
        return;
    }

    if (sb.free_inode_idx >= SB_FREE_INODE_MAX) {
        if (ino < sb.remembered_inode)
            sb.remembered_inode = ino;
        return;
    }

    sb.free_inodes[sb.free_inode_idx++] = ino;
    printf("[ifree] ino=%u returned to free list\\n", ino);
}

/* ─────────────────────────────────────────────────────────────
 * fs_free_inode_blocks — free all data blocks belonging to an inode
 * ───────────────────────────────────────────────────────────── */
void fs_free_inode_blocks(InCoreInode *ip)
{
    uint32_t i;
    if (!ip) return;
    for (i = 0; i < NDIRECT; i++) {
        if (ip->addr[i]) {
            fs_free(ip->addr[i]);
            ip->addr[i] = 0;
        }
    }
    /* Single indirect */
    if (ip->addr[NDIRECT]) {
        BufEntry *buf = bread(ip->addr[NDIRECT]);
        if (buf) {
            uint32_t *ptrs = (uint32_t *)buf->data;
            uint32_t  j;
            for (j = 0; j < PTRS_PER_BLOCK; j++)
                if (ptrs[j]) fs_free(ptrs[j]);
            brelse(buf);
        }
        fs_free(ip->addr[NDIRECT]);
        ip->addr[NDIRECT] = 0;
    }
    ip->size = 0;
}

/* ─────────────────────────────────────────────────────────────
 * sb_print — debug dump
 * ───────────────────────────────────────────────────────────── */
void sb_print(void)
{
    printf("[sb] free_blocks=%u  free_inodes=%u  "
           "remembered_ino=%u  modified=%d\\n",
           sb.free_block_count, sb.free_inode_count,
           sb.remembered_inode, sb.modified);
}
