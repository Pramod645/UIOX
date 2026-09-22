/*
 * 30_KIX/32_FS/10_unfs/include/unfs_disk.h   — v2.1.0
 *
 * UIOX Native Filesystem (UNFS) — ON-DISK FORMAT.
 *
 * Shared by the kernel (32_FS/10_unfs) and the bootloader reader (01_uBoot).
 * Defines the bytes on the block device, NOT the in-core layout — that is
 * uiox_kix_scfs_inode.h / mount.h.  unfs_iget() converts one to the other.
 *
 * ── v2.1.0 fixes ───────────────────────────────────────────────────────
 *   FIX 1  UNFS_INODE_SIZE was 256 while UNFS_INODE_BYTES was 512; the
 *          struct with padding is ~508 bytes, so the two disagreed and the
 *          superblock advertised the wrong slot size.  Now ONE constant:
 *          UNFS_INODE_BYTES == UNFS_INODE_SIZE == 512.
 *   FIX 2  i_xattr_blk is a DISK pointer; inode_t.i_xattr is an IN-CORE
 *          chain.  inflate does NOT populate i_xattr — it stays NULL and
 *          UNFS reads the on-disk chain on demand in getxattr().
 *
 * ── on-disk versioning ────────────────────────────────────────────────
 *   UNFS_MAGIC_V1 0x554E4653 ("UNFS")  legacy 32-bit
 *   UNFS_MAGIC_V2 0x554E4654 ("UNFT")  current
 *
 * A v2 kernel MOUNTS v1 read-only and REFUSES to write it.  A v1 kernel
 * refuses v2 outright.  That is the point of the versioned magic.
 *
 * ── sizes as two 32-bit halves, not one uint64_t ──────────────────────
 * Keeps the on-disk inode 4-byte aligned for arm32 / riscv32 readers.
 * Reassembled with unfs_mk64() at iget time.
 *
 * @version 2.1.0  @date 2026-09-21
 */
#ifndef UNFS_DISK_H
#define UNFS_DISK_H

#include "uiox_base_types.h"     /* uiox_uint8/16/32/64_t, freestanding */

#define UNFS_MAGIC_V1   0x554E4653u   /* "UNFS" — legacy, 32-bit    */
#define UNFS_MAGIC_V2   0x554E4654u   /* "UNFT" — current           */
#define UNFS_MAGIC      UNFS_MAGIC_V2

#define UNFS_BLOCK_SIZE      4096u    /* UNFS uses 4 KiB, not Bach's 512 */
#define UNFS_MAX_EXTENTS        12u   /* inline extents per inode        */
#define UNFS_MAX_GROUPS         64u
#define UNFS_NAME_MAX          255u
#define UNFS_ROOT_INO            2u
#define UNFS_NIL_INO             0u

/* ── FIX 1: inode slot size — ONE value ─────────────────────────────── */
/* The on-disk slot is 512 bytes: 8 inodes fit in a 4096-byte block, and the
 * padded unfs_inode_disk_t (~508 bytes) sits inside it.  Both names expand
 * to the same number so superblock arithmetic, table offsets, and struct
 * padding agree.  (UNFS_INODE_SIZE was 256 — that was the bug.) */
#define UNFS_INODE_BYTES     512u   /* ON-DISK inode slot size       */
#define UNFS_INODE_SIZE      UNFS_INODE_BYTES   /* alias — was 256, now 512 */

/* ── feature flags (superblock s_feature_*) ────────────────────────── */
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
    uiox_uint32_t s_inode_size;         /* == UNFS_INODE_SIZE (512) now */
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
/* Byte layout (sums to 508, padded to the 512-byte slot):
 *   mode 2 + nlink 2 + uid 4 + gid 4              = 12
 *   size_lo 4 + size_hi 4                         =  8
 *   4 x 64-bit times  (8 x uint32)                = 32
 *   blocks+flags+gen+seq+xattr_blk+extent_leaf    = 24
 *   fastlink[60]                                  = 60
 *   i_ext[12] x 16                                = 192
 *   i_reserved[176]                               = 176
 *   i_checksum                                    =  4
 *                                          total  = 508  (+4 pad = 512)
 */
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
    uiox_uint32_t i_xattr_blk;          /* FIX 2: DISK block of the xattr
                                         * chain, 0 = none.  NOT loaded
                                         * into inode_t.i_xattr; getxattr()
                                         * reads it on demand.            */
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
/* FIX 2: this is the DISK form.  The in-core form (uiox_xattr_node_t,
 * declared in uiox_kix_scfs_ops.h) is populated only when a caller asks via
 * getxattr/listxattr, and freed afterwards — inode_t.i_xattr stays NULL. */
typedef struct unfs_xattr_disk {
    uiox_uint32_t x_next;               /* next xattr block, 0 = last  */
    uiox_uint8_t  x_namelen;
    uiox_uint8_t  x_valuelen;
    uiox_uint16_t x_flags;
    uiox_uint8_t  x_name[64];
    uiox_uint8_t  x_value[180];         /* inline value                */
} unfs_xattr_disk_t;                    /* 256 bytes                    */

/* ── layout constants ──────────────────────────────────────────────── */
#define UNFS_SB_OFFSET       1024u
#define UNFS_SB_SIZE         1024u
/* UNFS_INODE_BYTES defined above, and it IS the slot size. */

/* ── endian helpers — on-disk format is little-endian on all four
 *    targets; the freestanding build has no <endian.h>. ────────────── */
static inline uiox_uint32_t unfs_lo32(uiox_uint64_t v)
{ return (uiox_uint32_t)(v & 0xFFFFFFFFu); }
static inline uiox_uint32_t unfs_hi32(uiox_uint64_t v)
{ return (uiox_uint32_t)(v >> 32); }
static inline uiox_uint64_t unfs_mk64(uiox_uint32_t lo, uiox_uint32_t hi)
{ return ((uiox_uint64_t)hi << 32) | (uiox_uint64_t)lo; }

#endif /* UNFS_DISK_H */
