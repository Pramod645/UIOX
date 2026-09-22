/*
 * 30_KIX/32_FS/10_unfs/include/unfs_disk.h   — v2.0.0
 *
 * UIOX Native Filesystem (UNFS) — ON-DISK FORMAT.
 *
 * Shared by the kernel (32_FS/10_unfs) and the bootloader reader (01_uBoot).
 * Defines the bytes on the block device, NOT the in-core layout — that is
 * uiox_kix_scfs_inode.h / mount.h.  unfs_iget() converts one to the other.
 *
 * ── v2.0.0 on-disk bump ─────────────────────────────────────────────
 * v1 was 32-bit: uint32 i_size, no groups, no xattr, 32-bit time.  All four
 * changed, so the format carries a versioned MAGIC and a feature word:
 *
 *   UNFS_MAGIC_V1 0x554E4653 ("UNFS")
 *   UNFS_MAGIC_V2 0x554E4654 ("UNFT")   <- current
 *
 * A v2 kernel MOUNTS v1 read-only; refuses to write it.  A v1 kernel
 * refuses v2 outright.  That is the point of the versioned magic.
 *
 * ── size >4 GB without 64-bit inode fields everywhere ───────────────
 * UNFS uses EXTENTS, not Bach's i_addr[13].  The 64-bit size is stored as
 * two 32-bit halves so the on-disk inode stays 4-byte aligned for arm32 /
 * riscv32 readers; the kernel reassembles into inode_t.i_size at iget time.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UNFS_DISK_H
#define UNFS_DISK_H

#include "uiox_base_types.h"     /* uiox_uint8/16/32/64_t, freestanding */

#define UNFS_MAGIC_V1   0x554E4653u   /* "UNFS" — legacy, 32-bit    */
#define UNFS_MAGIC_V2   0x554E4654u   /* "UNFT" — current           */
#define UNFS_MAGIC      UNFS_MAGIC_V2

#define UNFS_BLOCK_SIZE      4096u    /* UNFS uses 4 KiB, not Bach's 512 */
#define UNFS_INODE_SIZE        256u   /* bytes per on-disk inode         */
#define UNFS_MAX_EXTENTS        12u   /* inline extents per inode        */
#define UNFS_MAX_GROUPS         64u
#define UNFS_NAME_MAX          255u
#define UNFS_ROOT_INO            2u
#define UNFS_NIL_INO             0u

/* ── feature flags ─────────────────────────────────────────────────── */
#define UNFS_FEAT_COMPAT_DIR_INDEX     0x00000001u
#define UNFS_FEAT_INCOMPAT_64BIT       0x00000001u   /* 64-bit sizes   */
#define UNFS_FEAT_INCOMPAT_EXTENTS     0x00000002u   /* extent tree    */
#define UNFS_FEAT_INCOMPAT_XATTR       0x00000004u   /* extended attrs */
#define UNFS_FEAT_INCOMPAT_GROUPS      0x00000008u   /* block groups   */
#define UNFS_FEAT_RO_COMPAT_SPARSE     0x00000001u   /* sparse files   */

/* A kernel can write a volume only if it understands every INCOMPAT bit. */
#define UNFS_SUPPORTED_INCOMPAT \
    (UNFS_FEAT_INCOMPAT_64BIT | UNFS_FEAT_INCOMPAT_EXTENTS | \
     UNFS_FEAT_INCOMPAT_XATTR | UNFS_FEAT_INCOMPAT_GROUPS)

/* ── superblock — 1024 bytes at offset 1024 ────────────────────────── */
typedef struct unfs_sb_disk {
    uiox_uint32_t s_magic;              /* UNFS_MAGIC_V1 / V2           */
    uiox_uint32_t s_version;            /* 2                            */
    uiox_uint32_t s_block_size;         /* UNFS_BLOCK_SIZE              */
    uiox_uint32_t s_blocks_total;       /* device size in blocks        */
    uiox_uint32_t s_blocks_free;        /* free blocks (statfs)         */
    uiox_uint32_t s_inodes_total;
    uiox_uint32_t s_inodes_free;
    uiox_uint32_t s_inode_size;         /* UNFS_INODE_SIZE              */
    uiox_uint32_t s_inode_first_blk;    /* first block of the inode tbl */
    uiox_uint32_t s_inode_blocks;       /* blocks in the inode table    */
    uiox_uint32_t s_ncg;                /* allocation groups            */
    uiox_uint32_t s_groups_blk;         /* first block of group descs   */
    uiox_uint32_t s_root_ino;           /* usually UNFS_ROOT_INO        */
    uiox_uint32_t s_state;              /* UNFS_STATE_*                 */
    uiox_uint32_t s_mtime_lo;           /* last mount time  (low 32)    */
    uiox_uint32_t s_mtime_hi;           /*                   high 32    */
    uiox_uint32_t s_wtime_lo;           /* last write time  (low 32)    */
    uiox_uint32_t s_wtime_hi;
    uiox_uint32_t s_feature_compat;
    uiox_uint32_t s_feature_incompat;
    uiox_uint32_t s_feature_ro_compat;
    uiox_uint32_t s_uuid[4];            /* volume identifier            */
    uiox_uint8_t  s_volume_name[16];
    uiox_uint32_t s_checksum;           /* CRC32 over the block         */
    uiox_uint8_t  s_reserved[960];      /* pad to 1024                  */
} unfs_sb_disk_t;

#define UNFS_STATE_CLEAN     0x0001u
#define UNFS_STATE_ERRORS    0x0002u
#define UNFS_STATE_READONLY  0x0004u

/* ── group descriptor — one per allocation group ───────────────────── */
typedef struct unfs_group_disk {
    uiox_uint32_t g_first_block;
    uiox_uint32_t g_nblocks;
    uiox_uint32_t g_free_blocks;
    uiox_uint32_t g_first_ino;
    uiox_uint32_t g_ninodes;
    uiox_uint32_t g_free_inodes;
    uiox_uint32_t g_bitmap_blk;         /* block bitmap location        */
    uiox_uint32_t g_inode_bmp_blk;      /* inode bitmap location        */
    uiox_uint32_t g_inode_tbl_blk;      /* start of this group's inodes */
    uiox_uint8_t  g_reserved[28];       /* pad to 64 bytes              */
} unfs_group_disk_t;

/* ── extent — a contiguous run of blocks ───────────────────────────── */
/* One extent covers up to (2^16 - 1) blocks = 256 MiB at 4 KiB. */
typedef struct unfs_extent_disk {
    uiox_uint32_t e_start_lo;   /* logical block offset, low 32         */
    uiox_uint32_t e_start_hi;   /*                 high 32 (sparse fwd) */
    uiox_uint32_t e_phys;       /* physical block, low 32               */
    uiox_uint16_t e_len;        /* run length in blocks (0 = unused)    */
    uiox_uint16_t e_flags;      /* UNFS_EXT_*                           */
} unfs_extent_disk_t;           /* 16 bytes */

#define UNFS_EXT_HOLE   0x0001u   /* logical run with no blocks (sparse)  */
#define UNFS_EXT_LAST   0x8000u   /* last extent in a leaf block          */

/* ── inode — 512-byte slot on disk ─────────────────────────────────── */
typedef struct unfs_inode_disk {
    uiox_uint16_t i_mode;               /* type + permissions (Bach IF*) */
    uiox_uint16_t i_nlink;
    uiox_uint32_t i_uid;
    uiox_uint32_t i_gid;

    /* 64-bit size as two halves — see header note */
    uiox_uint32_t i_size_lo;
    uiox_uint32_t i_size_hi;

    uiox_uint32_t i_atime_lo;  uiox_uint32_t i_atime_hi;   /* 64-bit time */
    uiox_uint32_t i_mtime_lo;  uiox_uint32_t i_mtime_hi;
    uiox_uint32_t i_ctime_lo;  uiox_uint32_t i_ctime_hi;
    uiox_uint32_t i_btime_lo;  uiox_uint32_t i_btime_hi;

    uiox_uint32_t i_blocks;             /* 512-byte sectors used (stat)  */
    uiox_uint32_t i_flags;              /* UNFS_IFLAG_*                  */
    uiox_uint32_t i_generation;         /* stable file handle            */
    uiox_uint32_t i_seq;                /* change counter                */
    uiox_uint32_t i_xattr_blk;          /* xattr chain block, 0 = none   */
    uiox_uint32_t i_extent_leaf;        /* overflow extent block, 0=inline*/

    /* fast symlink: target stored inline when short enough */
    uiox_uint8_t  i_fastlink[60];

    /* inline extents — the common case needs no leaf block */
    unfs_extent_disk_t i_ext[UNFS_MAX_EXTENTS];   /* 12 * 16 = 192 bytes */

    uiox_uint8_t  i_reserved[176];      /* pad the 512-byte slot        */
    uiox_uint32_t i_checksum;
} unfs_inode_disk_t;

#define UNFS_IFLAG_EXTENTS   0x00000001u
#define UNFS_IFLAG_XATTR     0x00000002u
#define UNFS_IFLAG_COMPRESS  0x00000004u   /* reserved, not implemented */

/* ── directory entry — variable length, name follows ───────────────── */
typedef struct unfs_dirent_disk {
    uiox_uint32_t d_ino;                /* 0 = end of block            */
    uiox_uint16_t d_reclen;             /* bytes to next entry         */
    uiox_uint8_t  d_namlen;
    uiox_uint8_t  d_type;               /* UNFS_DT_*                   */
    uiox_uint8_t  d_name[UNFS_NAME_MAX];/* d_namlen bytes valid        */
} unfs_dirent_disk_t;

#define UNFS_DT_UNKNOWN 0
#define UNFS_DT_REG     1
#define UNFS_DT_DIR     2
#define UNFS_DT_CHR     3
#define UNFS_DT_BLK     4
#define UNFS_DT_FIFO    5
#define UNFS_DT_LNK     6

/* ── xattr — one node per attribute, chained off i_xattr_blk ───────── */
typedef struct unfs_xattr_disk {
    uiox_uint32_t x_next;               /* next xattr block, 0 = last  */
    uiox_uint8_t  x_namelen;
    uiox_uint8_t  x_valuelen;
    uiox_uint16_t x_flags;
    uiox_uint8_t  x_name[64];
    uiox_uint8_t  x_value[180];         /* inline value                */
} unfs_xattr_disk_t;

/* ── layout constants ──────────────────────────────────────────────── */
#define UNFS_SB_OFFSET       1024u
#define UNFS_SB_SIZE         1024u
#define UNFS_INODE_BYTES     512u   /* ON-DISK inode slot size       */

/* ── endian helpers — on-disk format is little-endian on all four
 *    targets; the freestanding build has no <endian.h>. ────────────── */
static inline uiox_uint32_t unfs_lo32(uiox_uint64_t v) { return (uiox_uint32_t)(v & 0xFFFFFFFFu); }
static inline uiox_uint32_t unfs_hi32(uiox_uint64_t v) { return (uiox_uint32_t)(v >> 32); }
static inline uiox_uint64_t unfs_mk64(uiox_uint32_t lo, uiox_uint32_t hi)
{
    return ((uiox_uint64_t)hi << 32) | (uiox_uint64_t)lo;
}

#endif /* UNFS_DISK_H */
