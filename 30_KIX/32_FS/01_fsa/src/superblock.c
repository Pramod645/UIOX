#include "superblock.h"
#include "/Users/pramodkumar/Hack/WS/UIOX/30_KIX/33_PCS/include/uiox_klibc.h"

static SuperBlock sb;

/* ─────────────────────────────────────────────────────────────
 * sb_init — create a fresh simulated filesystem
 * ───────────────────────────────────────────────────────────── */
void sb_init(void)
{
    memset(&sb, 0, sizeof sb);
    sb.fs_size          = MAX_BLOCKS;
    sb.inode_start      = INODE_START_BLOCK;
    sb.data_start       = DATA_START_BLOCK;
    sb.max_inodes       = MAX_INODES;
    sb.free_inode_count = MAX_INODES;
    sb.remembered_inode = 1;

    /* Populate free block list with blocks DATA_START_BLOCK..MAX_BLOCKS-1 */
    uint32_t blkno = DATA_START_BLOCK;
    sb.free_block_count = 0;
    sb.free_block_idx   = 0;

    while (blkno < MAX_BLOCKS &&
           sb.free_block_idx < SB_FREE_BLOCK_MAX) {
        sb.free_blocks[sb.free_block_idx++] = blkno++;
        sb.free_block_count++;
    }

    /* Populate free inode list */
    sb.free_inode_idx = 0;
    for (uint32_t i = 1;
         i <= MAX_INODES && sb.free_inode_idx < SB_FREE_INODE_MAX; i++) {
        sb.free_inodes[sb.free_inode_idx++] = i;
    }

    printf("[sb] init: %u blocks  %u inodes  free_blocks=%u\n",
           sb.fs_size, sb.max_inodes, sb.free_block_count);
}

SuperBlock *sb_get(void) { return &sb; }

/* ─────────────────────────────────────────────────────────────
 * Algorithm alloc  (§5)
 * ───────────────────────────────────────────────────────────── */
BufEntry *fs_alloc(void)
{
    /* Wait while superblock is locked */
    while (sb.locked) {
        printf("[alloc] superblock locked — waiting\n");
        /* sim: break immediately */
        break;
    }

    if (sb.free_block_idx == 0) {
        fprintf(stderr, "[alloc] no free blocks\n");
        return NULL;
    }

    /* Remove one block number from free list */
    uint32_t blkno = sb.free_blocks[--sb.free_block_idx];

    if (sb.free_block_idx == 0 && sb.free_block_count > 0) {
        /*
         * We just took the last entry in the in-core free list.
         * The block we took actually holds the NEXT group of free
         * block numbers — read it and refill the in-core list.
         */
        sb.locked = true;
        BufEntry *chain_buf = bread(blkno);
        if (chain_buf) {
            uint32_t *nums = (uint32_t *)chain_buf->data;
            int       n    = 0;
            while (n < SB_FREE_BLOCK_MAX && nums[n] != 0) {
                sb.free_blocks[n] = nums[n];
                n++;
            }
            sb.free_block_idx = n;
            brelse(chain_buf);
        }
        sb.locked = false;
        printf("[alloc] refilled free-block list from chain block %u\n", blkno);

        /* Pick the actual block to return from the refilled list */
        if (sb.free_block_idx == 0) {
            fprintf(stderr, "[alloc] filesystem full\n");
            return NULL;
        }
        blkno = sb.free_blocks[--sb.free_block_idx];
    }

    /* Get a buffer for the newly allocated block; zero it */
    BufEntry *buf = getblk(blkno);
    if (!buf) return NULL;
    memset(buf->data, 0, BLOCK_SIZE);
    buf->valid = true;
    buf->dirty = true;

    sb.free_block_count--;
    sb.modified = true;

    printf("[alloc] allocated blk=%u  free_blocks=%u\n",
           blkno, sb.free_block_count);
    return buf;
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm free  (§6)
 * ───────────────────────────────────────────────────────────── */
void fs_free(uint32_t blkno)
{
    if (blkno < DATA_START_BLOCK || blkno >= MAX_BLOCKS) {
        fprintf(stderr, "[free] invalid blkno %u\n", blkno);
        return;
    }

    sb.free_block_count++;

    if (sb.free_block_idx < SB_FREE_BLOCK_MAX) {
        sb.free_blocks[sb.free_block_idx++] = blkno;
    } else {
        /*
         * In-core list is full.  Write current list into the block
         * being freed, then restart the list with just this block.
         */
        BufEntry *buf = getblk(blkno);
        if (buf) {
            uint32_t *nums = (uint32_t *)buf->data;
            for (int i = 0; i < SB_FREE_BLOCK_MAX; i++)
                nums[i] = sb.free_blocks[i];
            buf->dirty = true;
            bwrite(buf);
            brelse(buf);
        }
        sb.free_block_idx   = 0;
        sb.free_blocks[sb.free_block_idx++] = blkno;
    }

    sb.modified = true;
    printf("[free] freed blk=%u  free_blocks=%u\n",
           blkno, sb.free_block_count);
}

/* ─────────────────────────────────────────────────────────────
 * Internal: scan disk for free inodes and refill sb list
 * ───────────────────────────────────────────────────────────── */
static void sb_refill_inode_list(void)
{
    sb.locked         = true;
    sb.free_inode_idx = 0;
    uint32_t start    = sb.remembered_inode;

    for (uint32_t ino = start;
         ino <= MAX_INODES &&
         sb.free_inode_idx < SB_FREE_INODE_MAX; ino++) {

        BufEntry  *buf;
        DiskInode *di = inode_disk_read(ino, &buf);
        if (di && di->mode == 0) {      /* mode == 0 → free inode */
            sb.free_inodes[sb.free_inode_idx++] = ino;
        }
        if (di) brelse(buf);
    }

    sb.locked = false;
    printf("[ialloc] refilled inode list: %d entries\n",
           sb.free_inode_idx);
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm ialloc  (§7)
 * ───────────────────────────────────────────────────────────── */
InCoreInode *ialloc(FileType ftype, uint16_t perm,
                     uint16_t uid, uint16_t gid)
{
    while (1) {
        while (sb.locked) {
            printf("[ialloc] superblock locked — waiting\n");
            break; /* sim: proceed */
        }

        /* Refill in-core free-inode list from disk if empty */
        if (sb.free_inode_idx == 0) {
            sb_refill_inode_list();
            if (sb.free_inode_idx == 0) {
                fprintf(stderr, "[ialloc] no free inodes\n");
                return NULL;
            }
        }

        /* Take an inode number from the list */
        uint32_t ino = sb.free_inodes[--sb.free_inode_idx];

        InCoreInode *ip = iget(ino);
        if (!ip) continue;

        /* Verify it is actually free (mode == 0 on disk) */
        BufEntry  *buf;
        DiskInode *di = inode_disk_read(ino, &buf);
        if (!di || di->mode != 0) {
            /* Not free after all — write inode and release */
            if (di) {
                buf->dirty = true;
                bwrite(buf);
                brelse(buf);
            }
            iput(ip);
            continue; /* retry */
        }
        brelse(buf);

        /* Initialise inode */
        ip->mode   = (uint16_t)((ftype << 12) | (perm & 0x1FF));
        ip->nlink  = 0;
        ip->uid    = uid;
        ip->gid    = gid;
        ip->size   = 0;
        ip->flags  = IFLAG_CHANGED;
        ip->atime  = ip->mtime = ip->ctime = time(NULL);
        memset(ip->addr, 0, sizeof ip->addr);

        /* Write initialised inode to disk */
        iupdate(ip);

        sb.free_inode_count--;
        sb.modified = true;

        printf("[ialloc] ino=%u  type=%d  perm=0%o  uid=%u\n",
               ino, ftype, perm, uid);
        return ip;
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm ifree  (§8)
 * ───────────────────────────────────────────────────────────── */
void ifree(uint32_t ino)
{
    sb.free_inode_count++;

    if (sb.locked) {
        printf("[ifree] superblock locked — inode %u not cached\n", ino);
        return;
    }

    if (sb.free_inode_idx >= SB_FREE_INODE_MAX) {
        /* List full — keep remembered inode as the lowest known free */
        if (ino < sb.remembered_inode)
            sb.remembered_inode = ino;
    } else {
        sb.free_inodes[sb.free_inode_idx++] = ino;
    }

    sb.modified = true;
    printf("[ifree] ino=%u freed  free_inodes=%u\n",
           ino, sb.free_inode_count);
}

/* ─────────────────────────────────────────────────────────────
 * fs_free_inode_blocks
 * Release all data blocks (direct + indirect) of an inode.
 * ───────────────────────────────────────────────────────────── */
void fs_free_inode_blocks(InCoreInode *ip)
{
    /* Free direct blocks */
    for (int i = 0; i < NDIRECT; i++) {
        if (ip->addr[i]) {
            fs_free(ip->addr[i]);
            ip->addr[i] = 0;
        }
    }

    /* Free single-indirect block */
    int si = NDIRECT;
    if (ip->addr[si]) {
        BufEntry *buf = bread(ip->addr[si]);
        if (buf) {
            uint32_t *ptrs = (uint32_t *)buf->data;
            for (int j = 0; j < (int)PTRS_PER_BLOCK; j++)
                if (ptrs[j]) fs_free(ptrs[j]);
            brelse(buf);
        }
        fs_free(ip->addr[si]);
        ip->addr[si] = 0;
    }

    ip->size = 0;
    ip->flags |= IFLAG_CHANGED;
    printf("[fs_free_inode_blocks] ino=%u all blocks freed\n", ip->ino);
}

/* ─────────────────────────────────────────────────────────────
 * sb_print
 * ───────────────────────────────────────────────────────────── */
void sb_print(void)
{
    printf("[sb] free_blocks=%u  free_inodes=%u  "
           "remembered_ino=%u  modified=%d\n",
           sb.free_block_count, sb.free_inode_count,
           sb.remembered_inode, sb.modified);
}
