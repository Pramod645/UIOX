/*
 * 30_KIX/32_FS/10_unfs/src/unfs_mount.c
 *
 * UNFS — mount / unmount.
 *
 *   magic == UNFS_MAGIC_V2  -> read-write; every INCOMPAT bit must be known
 *   magic == UNFS_MAGIC_V1  -> read-only; v1 had no groups/extents/xattr
 *   anything else           -> refuse (not a UNFS volume)
 *
 * The check is  (sb.s_feature_incompat & ~UNFS_SUPPORTED_INCOMPAT) == 0.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#include "unfs_disk.h"
#include "unfs_io.h"
#include "unfs_errno.h"
#include "uiox_kix_scfs_mount.h"
#include "uiox_kix_scfs_inode.h"

extern const uiox_inode_ops_t unfs_inode_ops;
extern const uiox_file_ops_t  unfs_file_ops;
extern int unfs_sync(super_block_t *sb);

static void mem_zero(void *p, uiox_uint32_t n)
{
    uiox_uint8_t *d = (uiox_uint8_t *)p;
    while (n--) *d++ = 0u;
}
static void mem_copy(void *d, const void *s, uiox_uint32_t n)
{
    uiox_uint8_t *dd = (uiox_uint8_t *)d;
    const uiox_uint8_t *ss = (const uiox_uint8_t *)s;
    while (n--) *dd++ = *ss++;
}

static void sb_inflate(const unfs_sb_disk_t *d, super_block_t *k)
{
    mem_zero(k, sizeof(*k));
    k->s_isize           = d->s_inode_blocks;
    k->s_fsize           = d->s_blocks_total;
    k->s_nfree           = 0u;
    k->s_ninode          = 0u;
    k->s_time            = unfs_mk64(d->s_wtime_lo, d->s_wtime_hi);
    k->s_tfree           = d->s_blocks_free;
    k->s_tinode          = d->s_inodes_free;
    k->s_fname[0]='U'; k->s_fname[1]='N'; k->s_fname[2]='F';
    k->s_fname[3]='S'; k->s_fname[4]=0;
    k->s_ncg             = d->s_ncg;
    k->s_bsize           = d->s_block_size;
    k->s_feature_compat    = d->s_feature_compat;
    k->s_feature_incompat  = d->s_feature_incompat;
    k->s_feature_ro_compat = d->s_feature_ro_compat;
}

static int read_groups(uiox_uint32_t dev, const unfs_sb_disk_t *d,
                       super_block_t *k)
{
    if (d->s_ncg > UIOX_MAX_GROUPS) return UNFS_EINVAL;

    uiox_uint8_t gblock[UNFS_BLOCK_SIZE];
    uiox_uint32_t per_blk = UNFS_BLOCK_SIZE / (uiox_uint32_t)sizeof(unfs_group_disk_t);

    for (uiox_uint32_t i = 0u; i < d->s_ncg; i++) {
        uiox_uint32_t blk = d->s_groups_blk + (i / per_blk);
        uiox_uint32_t off = (i % per_blk) * (uiox_uint32_t)sizeof(unfs_group_disk_t);

        if (unfs_bdev_read(dev, blk, gblock) != UNFS_OK) return UNFS_EIO;

        unfs_group_disk_t gd;
        mem_copy(&gd, gblock + off, sizeof(gd));

        k->s_groups[i].g_first_block   = gd.g_first_block;
        k->s_groups[i].g_nblocks       = gd.g_nblocks;
        k->s_groups[i].g_free_blocks   = gd.g_free_blocks;
        k->s_groups[i].g_first_ino     = gd.g_first_ino;
        k->s_groups[i].g_ninodes       = gd.g_ninodes;
        k->s_groups[i].g_free_inodes   = gd.g_free_inodes;
        k->s_groups[i].g_bitmap_blk    = gd.g_bitmap_blk;
        k->s_groups[i].g_inode_bmp_blk = gd.g_inode_bmp_blk;
    }
    return UNFS_OK;
}

int unfs_kern_mount(super_block_t *sb, uiox_uint32_t dev)
{
    if (!sb) return UNFS_EINVAL;

    uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
    if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;

    unfs_sb_disk_t d;
    mem_copy(&d, blk0 + UNFS_SB_OFFSET, sizeof(d));

    if (d.s_magic != UNFS_MAGIC_V1 && d.s_magic != UNFS_MAGIC_V2)
        return UNFS_EBADMAGIC;

    {
        uiox_uint32_t stored = d.s_checksum;
        d.s_checksum = 0u;
        uiox_uint8_t tmp[UNFS_SB_SIZE];
        mem_zero(tmp, UNFS_SB_SIZE);
        mem_copy(tmp, &d, sizeof(d));
        uiox_uint32_t calc = unfs_crc32(tmp, UNFS_SB_SIZE);
        d.s_checksum = stored;
        if (stored != 0u && calc != stored) return UNFS_EBADCRC;
    }

    if (d.s_block_size != UNFS_BLOCK_SIZE)   return UNFS_EBADSB;
    if (d.s_inode_size != UNFS_INODE_SIZE &&
        d.s_inode_size != UNFS_INODE_BYTES)  return UNFS_EBADSB;
    if (d.s_blocks_total == 0u)              return UNFS_EBADSB;
    if (d.s_root_ino == UNFS_NIL_INO)        return UNFS_EBADSB;

    int ro = 0;

    if (d.s_magic == UNFS_MAGIC_V1) {
        ro = 1;
    } else {
        if (d.s_feature_incompat & ~(uiox_uint32_t)UNFS_SUPPORTED_INCOMPAT)
            return UNFS_ENOTSUP;
    }

    sb_inflate(&d, sb);
    if (ro) sb->s_ronly = 1u;

    if (d.s_magic == UNFS_MAGIC_V2) {
        int rc = read_groups(dev, &d, sb);
        if (rc != UNFS_OK) return rc;
    }

    mount_t *mp = mount_alloc();
    if (!mp) return UNFS_ENOSPC;

    mp->m_dev        = (uiox_uint16_t)dev;
    mp->m_bufp       = (buf_t *)0;
    mp->m_inodp      = (inode_t *)0;
    mp->m_mount_root = (inode_t *)0;
    mp->m_sb         = *sb;
    mp->m_flags      = ro ? MNT_RDONLY : 0;
    mp->m_mounted    = 1u;
    mp->m_fsname     = "unfs";

    (void)unfs_inode_ops.fs_name;
    (void)unfs_file_ops;

    return UNFS_OK;
}

int unfs_kern_unmount(super_block_t *sb)
{
    if (!sb) return UNFS_EINVAL;

    mount_t *mp = (mount_t *)0;
    for (uiox_uint32_t i = 0u; i < NMOUNT; i++) {
        if (mount_table[i].m_mounted && &mount_table[i].m_sb == sb) {
            mp = &mount_table[i];
            break;
        }
    }
    if (!mp) return UNFS_EINVAL;
    if (mp->m_flags & MNT_RDONLY) { mount_free(mp); return UNFS_OK; }

    int rc = unfs_sync(sb);
    mount_free(mp);
    return rc;
}
