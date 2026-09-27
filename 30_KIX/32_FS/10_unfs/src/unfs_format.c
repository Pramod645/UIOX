/*
 * 30_KIX/32_FS/10_unfs/src/unfs_format.c
 *
 * UNFS — mkfs.  Writes a fresh v2 filesystem onto a block device.
 *
 * Writes, in order:
 *   block 0                  boot area (left zero), then UNFS_BLOCK_SIZE
 *                            with the superblock at UNFS_SB_OFFSET
 *   UNFS_GROUP0_DESC         group descriptor table
 *   UNFS_GROUP0_BBMAP        group 0 block bitmap
 *   UNFS_GROUP0_BBMAP + 1    group 0 inode bitmap
 *   UNFS_GROUP0_ITABLE       group 0 inode table
 *   UNFS_GROUP0_DATA         root directory data block
 *   root inode               UNFS_ROOT_INO, a directory
 *   root "." / ".."          the two mandatory directory entries
 *
 * Layout constants come from unfs_format.h, which the bootloader shares.
 *
 * Feature word: s_feature_incompat stays 0.  UNFS_SUPPORTED_INCOMPAT is 0,
 * so writing any bit would make the volume refuse to mount.
 *
 * Group descriptor fields are DERIVED, not stored.  unfs_group_desc_t holds
 * exactly three fields (the three the bootloader reads); first block, block
 * count, first inode and node count are computed from the layout constants,
 * and the free counts live in the superblock where unfs_mount.c reads them.
 *
 * @version 2.1.0  @date 2026-09-27
 */
#include "unfs_format.h"   /* shared with the bootloader */
#include "unfs_io.h"       /* unfs_bdev_read / unfs_bdev_write, crc32 */
#include "unfs_errno.h"

#ifndef UNFS_DEFAULT_BLOCKS_PER_GROUP
#define UNFS_DEFAULT_BLOCKS_PER_GROUP   8192u   /* 32 MiB at 4 KiB     */
#endif
#ifndef UNFS_DEFAULT_INODES_PER_GROUP
#define UNFS_DEFAULT_INODES_PER_GROUP    512u
#endif

static void mem_zero(void *p, uiox_uint32_t n)
{
    uiox_uint8_t *d = (uiox_uint8_t *)p;
    while (n--) *d++ = 0u;
}

static int write_block(uiox_uint32_t dev, uiox_uint32_t blk, const void *buf)
{
    return unfs_bdev_write(dev, blk, buf);
}

static void bitmap_set_free(uiox_uint8_t *bm, uiox_uint32_t bit)
{
    bm[bit >> 3] &= (uiox_uint8_t)~(1u << (bit & 7u));
}

static void bitmap_set_used(uiox_uint8_t *bm, uiox_uint32_t bit)
{
    bm[bit >> 3] |= (uiox_uint8_t)(1u << (bit & 7u));
}

int unfs_format(uiox_uint32_t dev, uiox_uint32_t total_blocks,
                uiox_uint32_t *root_ino_out)
{
    unfs_sb_t     sb;
    uiox_uint32_t i;

    if (total_blocks < 64u) return UNFS_EINVAL;

    mem_zero(&sb, sizeof(sb));

    uiox_uint32_t blocks_per_group = UNFS_DEFAULT_BLOCKS_PER_GROUP;
    uiox_uint32_t inodes_per_group = UNFS_DEFAULT_INODES_PER_GROUP;

    uiox_uint32_t ncg = (total_blocks + blocks_per_group - 1u) / blocks_per_group;
    if (ncg == 0u) ncg = 1u;

    uiox_uint32_t inodes_total = ncg * inodes_per_group;

    uiox_uint32_t groups_blk = 1u;
    uiox_uint32_t groups_blocks =
        (ncg * (uiox_uint32_t)sizeof(unfs_group_desc_t) + UNFS_BLOCK_SIZE - 1u)
        / UNFS_BLOCK_SIZE;
    if (groups_blocks == 0u) groups_blocks = 1u;

    uiox_uint32_t inode_first_blk = UNFS_GROUP0_ITABLE;

    /* ── superblock ──────────────────────────────────────────────────── */
    sb.s_magic          = UNFS_MAGIC;
    sb.s_version_major  = UNFS_VERSION_MAJOR;
    sb.s_version_minor  = UNFS_VERSION_MINOR;
    sb.s_block_size     = UNFS_BLOCK_SIZE;
    sb.s_block_count    = total_blocks;
    sb.s_free_blocks    = total_blocks;
    sb.s_inode_count    = inodes_total;
    sb.s_free_inodes    = inodes_total;
    sb.s_inode_size     = UNFS_INODE_SIZE;
    sb.s_inodes_per_group  = inodes_per_group;
    sb.s_blocks_per_group  = blocks_per_group;
    sb.s_group_count    = ncg;
    sb.s_clean          = 1u;
    sb.s_mount_time_ns  = 0u;
    sb.s_write_time_ns  = 0u;
    sb.s_mount_count    = 0u;
    sb.s_max_mount_count = 0u;
    sb.s_checksum_type  = 1u;   /* CRC32 */
    sb.s_compress       = 0u;
    mem_zero(sb.s_uuid, (uiox_uint32_t)sizeof(sb.s_uuid));
    mem_zero(sb.s_volume_name, (uiox_uint32_t)sizeof(sb.s_volume_name));

    uiox_uint32_t meta_blocks = UNFS_GROUP0_DATA;
    sb.s_free_blocks = total_blocks - meta_blocks;

    /* ── group descriptors + bitmaps + inode table ───────────────────── */
    uiox_uint32_t blk = inode_first_blk;
    uiox_uint32_t ino = 1u;
    uiox_uint8_t  bmbuf[UNFS_BLOCK_SIZE];

    for (i = 0u; i < ncg; i++) {
        unfs_group_desc_t gd;
        mem_zero(&gd, sizeof(gd));

        uiox_uint32_t first_blk = (i == 0u) ? blk : (i * blocks_per_group);
        uiox_uint32_t nblocks   = (i == ncg - 1u)
                                ? (total_blocks - first_blk)
                                : blocks_per_group;

        gd.bg_block_bitmap = (i == 0u) ? (first_blk + 1u) : first_blk;
        gd.bg_inode_bitmap = gd.bg_block_bitmap + 1u;
        gd.bg_inode_table  = UNFS_GROUP0_ITABLE
                             + ((ino - 1u) * UNFS_INODE_BYTES) / UNFS_BLOCK_SIZE;

        mem_zero(bmbuf, UNFS_BLOCK_SIZE);
        for (uiox_uint32_t b = 0u; b < nblocks && b < UNFS_BLOCK_SIZE * 8u; b++)
            bitmap_set_free(bmbuf, b);
        if (i == 0u) {
            for (uiox_uint32_t b = 0u; b < (meta_blocks - 2u * ncg); b++)
                bitmap_set_used(bmbuf, b);
        }
        if (write_block(dev, gd.bg_block_bitmap, bmbuf) != UNFS_OK)
            return UNFS_EIO;

        mem_zero(bmbuf, UNFS_BLOCK_SIZE);
        for (uiox_uint32_t n = 0u; n < inodes_per_group; n++)
            bitmap_set_free(bmbuf, n);
        bitmap_set_used(bmbuf, 0u);
        if (i == 0u) bitmap_set_used(bmbuf, UNFS_ROOT_INO - 1u);
        if (write_block(dev, gd.bg_inode_bitmap, bmbuf) != UNFS_OK)
            return UNFS_EIO;

        /* Free counts live in the SUPERBLOCK — unfs_group_desc_t has no
         * field for them, and this is where unfs_mount.c reads them. */
        uiox_uint32_t grp_free_blocks = nblocks - 2u;
        if (i == 0u)
            grp_free_blocks -= (meta_blocks - 2u * ncg) - 2u;

        uiox_uint32_t grp_free_inodes = inodes_per_group;
        if (i == 0u) grp_free_inodes -= 2u;   /* "." and ".." */

        sb.s_free_blocks += grp_free_blocks;
        sb.s_free_inodes += grp_free_inodes;

        uiox_uint8_t gblock[UNFS_BLOCK_SIZE];
        mem_zero(gblock, UNFS_BLOCK_SIZE);
        (void)unfs_bdev_read(dev, groups_blk, gblock);
        uiox_uint32_t off = (i * (uiox_uint32_t)sizeof(unfs_group_desc_t))
                          % UNFS_BLOCK_SIZE;
        uiox_uint8_t *dst = gblock + off;
        uiox_uint8_t *src = (uiox_uint8_t *)&gd;
        for (uiox_uint32_t k = 0u; k < (uiox_uint32_t)sizeof(gd); k++)
            dst[k] = src[k];
        if (write_block(dev, groups_blk, gblock) != UNFS_OK)
            return UNFS_EIO;

        ino += inodes_per_group;
        blk  = first_blk + nblocks;
    }

    /* ── root inode + its data block ─────────────────────────────────── */
    uiox_uint32_t root_data_blk = 0u;
    uiox_uint8_t  bm[UNFS_BLOCK_SIZE];

    {
        unfs_inode_t ri;
        mem_zero(&ri, sizeof(ri));
        ri.i_mode     = UNFS_IFDIR | 0755u;
        ri.i_nlink    = 2u;
        ri.i_uid      = 0u;
        ri.i_gid      = 0u;
        ri.i_size     = UNFS_BLOCK_SIZE;
        ri.i_blocks   = UNFS_BLOCK_SIZE / 512u;
        ri.i_flags    = 0u;

        /* Group 0's block bitmap is the constant UNFS_GROUP0_BBMAP.  The
         * first FREE bit is the group's first data block. */
        if (unfs_bdev_read(dev, UNFS_GROUP0_BBMAP, bm) != UNFS_OK)
            return UNFS_EIO;

        for (uiox_uint32_t b = 0u; b < UNFS_GROUP0_DATA; b++) {
            if (bm[b >> 3] & (uiox_uint8_t)(1u << (b & 7u))) {
                root_data_blk = UNFS_GROUP0_DATA + b;
                bitmap_set_used(bm, b);
                break;
            }
        }
        if (root_data_blk == 0u) return UNFS_ENOSPC;
        if (write_block(dev, UNFS_GROUP0_BBMAP, bm) != UNFS_OK) return UNFS_EIO;

        ri.i_extents[0].e_logical  = 0u;
        ri.i_extents[0].e_physical = root_data_blk;
        ri.i_extents[0].e_len      = 1u;
        ri.i_extents[0].e_flags    = UNFS_EXT_LEAF;

        uiox_uint8_t dbuf[UNFS_BLOCK_SIZE];
        mem_zero(dbuf, UNFS_BLOCK_SIZE);
        unfs_dirent_t *d0 = (unfs_dirent_t *)dbuf;
        d0->d_ino      = UNFS_ROOT_INO;
        d0->d_name_len = 1u;
        d0->d_type     = UNFS_DT_DIR;
        d0->d_name[0]  = '.';
        d0->d_rec_len  = (uiox_uint16_t)((sizeof(unfs_dirent_t) + 1u + 3u) & ~3u);

        unfs_dirent_t *d1 = (unfs_dirent_t *)(dbuf + d0->d_rec_len);
        d1->d_ino      = UNFS_ROOT_INO;
        d1->d_name_len = 2u;
        d1->d_type     = UNFS_DT_DIR;
        d1->d_name[0]  = '.';
        d1->d_name[1]  = '.';
        d1->d_rec_len  = (uiox_uint16_t)(UNFS_BLOCK_SIZE - d0->d_rec_len);

        if (write_block(dev, root_data_blk, dbuf) != UNFS_OK) return UNFS_EIO;

        uiox_uint32_t root_slot_blk =
            UNFS_GROUP0_ITABLE + ((UNFS_ROOT_INO - 1u) * UNFS_INODE_BYTES)
                                / UNFS_BLOCK_SIZE;
        uiox_uint32_t root_slot_off =
            ((UNFS_ROOT_INO - 1u) * UNFS_INODE_BYTES) % UNFS_BLOCK_SIZE;

        uiox_uint8_t ibuf[UNFS_BLOCK_SIZE];
        if (unfs_bdev_read(dev, root_slot_blk, ibuf) != UNFS_OK)
            mem_zero(ibuf, UNFS_BLOCK_SIZE);
        uiox_uint8_t *ip_dst = ibuf + root_slot_off;
        uiox_uint8_t *ip_src = (uiox_uint8_t *)&ri;
        for (uiox_uint32_t k = 0u; k < sizeof(ri) && k < UNFS_INODE_BYTES; k++)
            ip_dst[k] = ip_src[k];
        if (write_block(dev, root_slot_blk, ibuf) != UNFS_OK) return UNFS_EIO;
    }

    /* ── superblock into block 0 ─────────────────────────────────────── */
    {
        uiox_uint8_t sblk[UNFS_SB_SIZE];
        mem_zero(sblk, UNFS_SB_SIZE);
        uiox_uint8_t *p = (uiox_uint8_t *)&sb;
        for (uiox_uint32_t k = 0u; k < sizeof(sb) && k < UNFS_SB_SIZE; k++)
            sblk[k] = p[k];
        sb.s_sb_checksum = unfs_crc32(sblk, UNFS_SB_SIZE);
        p = (uiox_uint8_t *)&sb;
        for (uiox_uint32_t k = 0u; k < sizeof(sb) && k < UNFS_SB_SIZE; k++)
            sblk[k] = p[k];

        uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
        mem_zero(blk0, UNFS_BLOCK_SIZE);
        for (uiox_uint32_t k = 0u; k < UNFS_SB_SIZE; k++)
            blk0[UNFS_SB_OFFSET + k] = sblk[k];
        if (write_block(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
    }

    if (root_ino_out) *root_ino_out = UNFS_ROOT_INO;
    return UNFS_OK;
}
