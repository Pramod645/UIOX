/*
 * 30_KIX/32_FS/10_unfs/src/unfs_format.c
 *
 * UNFS — mkfs.  Writes a fresh v2 filesystem onto a block device.
 *
 * Writes, in order:
 *   block 0            boot area (left zero)
 *   offset 1024        superblock (1024 bytes)  s_magic = V2
 *   s_groups_blk       group descriptor table
 *   each group         block bitmap, inode bitmap, inode table
 *   root inode         UNFS_ROOT_INO, a directory
 *   root "." / ".."    the two mandatory directory entries
 *
 * Feature word written: 64BIT | EXTENTS | XATTR | GROUPS.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#include "unfs_disk.h"
#include "unfs_io.h"        /* unfs_bdev_write / unfs_bdev_read, crc32 */
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

static int zero_block(uiox_uint32_t dev, uiox_uint32_t blk)
{
    static uiox_uint8_t zero[UNFS_BLOCK_SIZE];
    mem_zero(zero, UNFS_BLOCK_SIZE);
    return unfs_bdev_write(dev, blk, zero);
}

static int write_block(uiox_uint32_t dev, uiox_uint32_t blk, const void *buf)
{
    return unfs_bdev_write(dev, blk, buf);
}

static void bitmap_set_free(uiox_uint8_t *bm, uiox_uint32_t bit)
{
    bm[bit >> 3] |= (uiox_uint8_t)(1u << (bit & 7u));   /* 1 = free */
}
static void bitmap_set_used(uiox_uint8_t *bm, uiox_uint32_t bit)
{
    bm[bit >> 3] &= (uiox_uint8_t)~(1u << (bit & 7u));  /* 0 = used */
}

int unfs_format(uiox_uint32_t dev, uiox_uint32_t total_blocks,
                uiox_uint32_t *root_ino_out)
{
    unfs_sb_disk_t sb;
    uiox_uint32_t  i;

    if (total_blocks < 64u) return UNFS_EINVAL;

    mem_zero(&sb, sizeof(sb));

    uiox_uint32_t blocks_per_group = UNFS_DEFAULT_BLOCKS_PER_GROUP;
    uiox_uint32_t inodes_per_group = UNFS_DEFAULT_INODES_PER_GROUP;

    uiox_uint32_t ncg = (total_blocks + blocks_per_group - 1u) / blocks_per_group;
    if (ncg == 0u)  ncg = 1u;
    if (ncg > UNFS_MAX_GROUPS) {
        blocks_per_group = (total_blocks + UNFS_MAX_GROUPS - 1u) / UNFS_MAX_GROUPS;
        ncg = (total_blocks + blocks_per_group - 1u) / blocks_per_group;
    }

    uiox_uint32_t inodes_total = ncg * inodes_per_group;
    uiox_uint32_t inode_blocks =
        (inodes_total * UNFS_INODE_BYTES + UNFS_BLOCK_SIZE - 1u) / UNFS_BLOCK_SIZE;

    uiox_uint32_t groups_blk = 1u;
    uiox_uint32_t groups_blocks =
        (ncg * (uiox_uint32_t)sizeof(unfs_group_disk_t) + UNFS_BLOCK_SIZE - 1u)
        / UNFS_BLOCK_SIZE;
    if (groups_blocks == 0u) groups_blocks = 1u;

    uiox_uint32_t inode_first_blk = groups_blk + groups_blocks;

    sb.s_magic          = UNFS_MAGIC;      /* V2 = "UNFT" */
    sb.s_version        = 2u;
    sb.s_block_size     = UNFS_BLOCK_SIZE;
    sb.s_blocks_total   = total_blocks;
    sb.s_blocks_free    = total_blocks;
    sb.s_inodes_total   = inodes_total;
    sb.s_inodes_free    = inodes_total;
    sb.s_inode_size     = UNFS_INODE_SIZE;
    sb.s_inode_first_blk= inode_first_blk;
    sb.s_inode_blocks   = inode_blocks;
    sb.s_ncg            = ncg;
    sb.s_groups_blk     = groups_blk;
    sb.s_root_ino       = UNFS_ROOT_INO;
    sb.s_state          = UNFS_STATE_CLEAN;
    sb.s_mtime_lo = sb.s_mtime_hi = 0u;
    sb.s_wtime_lo = sb.s_wtime_hi = 0u;
    sb.s_feature_compat    = 0u;
    sb.s_feature_incompat  = UNFS_FEAT_INCOMPAT_64BIT
                           | UNFS_FEAT_INCOMPAT_EXTENTS
                           | UNFS_FEAT_INCOMPAT_XATTR
                           | UNFS_FEAT_INCOMPAT_GROUPS;
    sb.s_feature_ro_compat = 0u;
    for (i = 0u; i < 4u; i++) sb.s_uuid[i] = 0u;
    mem_zero(sb.s_volume_name, 16);

    uiox_uint32_t meta_blocks = 1u + groups_blocks;
    for (i = 0u; i < ncg; i++) meta_blocks += 2u;
    meta_blocks += inode_blocks;
    if (meta_blocks >= total_blocks) return UNFS_ENOSPC;
    sb.s_blocks_free = total_blocks - meta_blocks;

    uiox_uint32_t blk   = inode_first_blk;
    uiox_uint32_t ino   = 1u;
    uiox_uint8_t  bmbuf[UNFS_BLOCK_SIZE];

    for (i = 0u; i < ncg; i++) {
        unfs_group_disk_t gd;
        mem_zero(&gd, sizeof(gd));

        uiox_uint32_t first_blk = (i == 0u) ? blk : (i * blocks_per_group);
        uiox_uint32_t nblocks   = (i == ncg - 1u)
                                ? (total_blocks - first_blk)
                                : blocks_per_group;

        gd.g_first_block = first_blk;
        gd.g_nblocks     = nblocks;
        gd.g_first_ino   = ino;
        gd.g_ninodes     = inodes_per_group;
        gd.g_free_inodes = inodes_per_group;
        gd.g_inode_tbl_blk = inode_first_blk + ((ino - 1u) * UNFS_INODE_BYTES)
                             / UNFS_BLOCK_SIZE;

        gd.g_bitmap_blk = (i == 0u) ? (first_blk + 1u) : first_blk;
        mem_zero(bmbuf, UNFS_BLOCK_SIZE);
        for (uiox_uint32_t b = 0u; b < nblocks && b < UNFS_BLOCK_SIZE * 8u; b++)
            bitmap_set_free(bmbuf, b);
        if (i == 0u) {
            for (uiox_uint32_t b = 0u; b < (meta_blocks - 2u * ncg); b++)
                bitmap_set_used(bmbuf, b);
        }
        if (write_block(dev, gd.g_bitmap_blk, bmbuf) != UNFS_OK)
            return UNFS_EIO;

        gd.g_inode_bmp_blk = gd.g_bitmap_blk + 1u;
        mem_zero(bmbuf, UNFS_BLOCK_SIZE);
        for (uiox_uint32_t n = 0u; n < inodes_per_group; n++)
            bitmap_set_free(bmbuf, n);
        bitmap_set_used(bmbuf, 0u);
        if (i == 0u) bitmap_set_used(bmbuf, UNFS_ROOT_INO - 1u);
        if (write_block(dev, gd.g_inode_bmp_blk, bmbuf) != UNFS_OK)
            return UNFS_EIO;

        gd.g_free_blocks = nblocks - 2u;
        if (i == 0u)
            gd.g_free_blocks -= (meta_blocks - 2u * ncg) - 2u;
        if (i == 0u) gd.g_free_inodes = inodes_per_group - 2u;
        else         gd.g_free_inodes = inodes_per_group;

        uiox_uint8_t gblock[UNFS_BLOCK_SIZE];
        mem_zero(gblock, UNFS_BLOCK_SIZE);
        (void)unfs_bdev_read(dev, groups_blk, gblock);
        uiox_uint32_t off = (i * (uiox_uint32_t)sizeof(unfs_group_disk_t))
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

    {
        unfs_inode_disk_t ri;
        mem_zero(&ri, sizeof(ri));
        ri.i_mode     = IFDIR | 0755u;
        ri.i_nlink    = 2u;
        ri.i_uid      = 0u;
        ri.i_gid      = 0u;
        ri.i_size_lo  = UNFS_BLOCK_SIZE;
        ri.i_size_hi  = 0u;
        ri.i_blocks   = UNFS_BLOCK_SIZE / 512u;
        ri.i_flags    = UNFS_IFLAG_EXTENTS;
        ri.i_seq      = 1u;

        uiox_uint32_t root_data_blk = 0u;
        uiox_uint8_t  bm[UNFS_BLOCK_SIZE];
        uiox_group_disk_t gd0;
        {
            uiox_uint8_t gblock[UNFS_BLOCK_SIZE];
            if (unfs_bdev_read(dev, groups_blk, gblock) != UNFS_OK)
                return UNFS_EIO;
            uiox_uint8_t *p = gblock;
            uiox_uint8_t *q = (uiox_uint8_t *)&gd0;
            for (uiox_uint32_t k = 0u; k < (uiox_uint32_t)sizeof(gd0); k++)
                q[k] = p[k];
        }
        if (unfs_bdev_read(dev, gd0.g_bitmap_blk, bm) != UNFS_OK)
            return UNFS_EIO;
        for (uiox_uint32_t b = 0u; b < gd0.g_nblocks; b++) {
            if (bm[b >> 3] & (uiox_uint8_t)(1u << (b & 7u))) {
                root_data_blk = gd0.g_first_block + b;
                bitmap_set_used(bm, b);
                break;
            }
        }
        if (root_data_blk == 0u) return UNFS_ENOSPC;
        if (write_block(dev, gd0.g_bitmap_blk, bm) != UNFS_OK) return UNFS_EIO;

        ri.i_ext[0].e_start_lo = 0u;
        ri.i_ext[0].e_start_hi = 0u;
        ri.i_ext[0].e_phys     = root_data_blk;
        ri.i_ext[0].e_len      = 1u;
        ri.i_ext[0].e_flags    = UNFS_EXT_LAST;

        uiox_uint8_t dbuf[UNFS_BLOCK_SIZE];
        mem_zero(dbuf, UNFS_BLOCK_SIZE);
        unfs_dirent_disk_t *d0 = (unfs_dirent_disk_t *)dbuf;
        d0->d_ino    = UNFS_ROOT_INO;
        d0->d_namlen = 1u;
        d0->d_type   = UNFS_DT_DIR;
        d0->d_name[0]= '.';
        d0->d_reclen = (uiox_uint16_t)(sizeof(unfs_dirent_disk_t));

        unfs_dirent_disk_t *d1 = (unfs_dirent_disk_t *)(dbuf + d0->d_reclen);
        d1->d_ino    = UNFS_ROOT_INO;
        d1->d_namlen = 2u;
        d1->d_type   = UNFS_DT_DIR;
        d1->d_name[0]= '.'; d1->d_name[1] = '.';
        d1->d_reclen = (uiox_uint16_t)(UNFS_BLOCK_SIZE - d0->d_reclen);

        if (write_block(dev, root_data_blk, dbuf) != UNFS_OK) return UNFS_EIO;

        uiox_uint32_t root_slot_blk =
            inode_first_blk + ((UNFS_ROOT_INO - 1u) * UNFS_INODE_BYTES)
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

    {
        uiox_uint8_t sblk[UNFS_SB_SIZE];
        mem_zero(sblk, UNFS_SB_SIZE);
        uiox_uint8_t *p = (uiox_uint8_t *)&sb;
        for (uiox_uint32_t k = 0u; k < sizeof(sb) && k < UNFS_SB_SIZE; k++)
            sblk[k] = p[k];
        sb.s_checksum = unfs_crc32(sblk, UNFS_SB_SIZE);
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
