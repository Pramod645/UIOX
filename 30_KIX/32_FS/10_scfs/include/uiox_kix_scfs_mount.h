/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_mount.h   — v3.0.0
 *
 * UIOX mount table + superblock (Bach Ch.4/9), with allocation groups.
 *
 * GAP FIXES APPLIED (#2 groups, #7 64-bit superblock time)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_MOUNT_H
#define UIOX_KIX_SCFS_MOUNT_H

#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_buf.h"
#include "uiox_klibc.h"

#define NMOUNT  20

/* GAP #2: one descriptor per allocation group (FFS cylinder group). */
typedef struct uiox_group_desc {
    uint32_t g_first_block;
    uint32_t g_nblocks;
    uint32_t g_free_blocks;
    uint32_t g_first_ino;
    uint32_t g_ninodes;
    uint32_t g_free_inodes;
    uint32_t g_bitmap_blk;
    uint32_t g_inode_bmp_blk;
} uiox_group_desc_t;

#define UIOX_MAX_GROUPS 64

typedef struct super_block {
    uint32_t s_isize;
    uint32_t s_fsize;
    uint32_t s_nfree;
    uint32_t s_free[50];
    uint16_t s_ninode;
    uint32_t s_inode[100];
    uint8_t  s_flock;
    uint8_t  s_ilock;
    uint8_t  s_fmod;
    uint8_t  s_ronly;

    uiox_time64_t s_time;                     /* GAP #7 */

    uint32_t s_tfree;
    uint32_t s_tinode;
    uint16_t s_m;
    uint16_t s_n;
    char     s_fname[16];
    char     s_fpack[16];

    /* GAP #2: allocation groups */
    uint32_t          s_ncg;
    uint32_t          s_bsize;
    uiox_group_desc_t s_groups[UIOX_MAX_GROUPS];

    /* feature flags — refuse an incompatible on-disk layout */
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
} super_block_t;

#define UIOX_FEAT_INCOMPAT_64BIT   0x0001u
#define UIOX_FEAT_INCOMPAT_EXTENTS 0x0002u
#define UIOX_FEAT_INCOMPAT_XATTR   0x0004u

typedef struct mount {
    uint16_t      m_dev;
    buf_t        *m_bufp;
    inode_t      *m_inodp;
    inode_t      *m_mount_root;
    super_block_t m_sb;
    int           m_flags;
    uint8_t       m_mounted;
    const char   *m_fsname;
} mount_t;

#define MNT_RDONLY   0x01
#define MNT_FORCE    0x02
#define MNT_NOSUID   0x04
#define MNT_NOEXEC   0x08
#define MNT_RELATIME 0x10

extern mount_t mount_table[NMOUNT];

mount_t *getmount(uint16_t dev);
mount_t *mount_alloc(void);
void     mount_free(mount_t *mp);
inode_t *mount_find_root(inode_t *covered);

#endif /* UIOX_KIX_SCFS_MOUNT_H */
