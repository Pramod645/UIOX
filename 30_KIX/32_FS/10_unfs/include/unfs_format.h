/*
 *  30_KIX/32_FS/10_unfs/include/unfs_format.h
 *
 *  UIOX Native Filesystem (UNFS) — THE ON-DISK FORMAT.
 *
 *  ── who includes this ─────────────────────────────────────────────────
 *      10_unfs/        the kernel's read/write implementation
 *      01_fsa/         bmap/iget, which must know where inodes live
 *      01_uBoot/       the bootloader's read-only reader — via its own
 *                      -I to this directory (see the boot Makefile)
 *
 *  ── why this lives in 10_unfs and not in common ───────────────────────
 *  The format is UNFS's, so UNFS owns it.  The bootloader reaches it by
 *  adding one include path — it still depends on NO kernel source file,
 *  because this header carries layout only, no code and no kernel types.
 *
 *      # in 01_uBoot's Makefile
 *      BOOT_UNFS_INC := $(abspath $(MFDIR)../../30_KIX/32_FS/10_unfs/include)
 *      CFLAGS += -I$(BOOT_UNFS_INC)
 *
 *  SOURCE OF TRUTH: 01_uBoot/include/uiox_boot_unfs.h.  Every constant and
 *  struct below is taken from that file, because it is the one already
 *  written and already reading disks.  If this header and that file ever
 *  disagree, the bootloader is right and this is the defect.
 *
 *  ── what changed from the FAT32-era fs_types.h ────────────────────────
 *  01_fsa's fs_types.h was written when FAT32 was the plan.  Its values
 *  are FAT/simulator numbers and DO NOT describe UNFS:
 *
 *      fs_types.h (FAT era)      UNFS (actual)
 *      ────────────────────      ────────────────────────────
 *      BLOCK_SIZE        512     4096
 *      inode             64 B    256 B
 *      INODE_START_BLOCK 2       261 (group 0 inode table)
 *      MAX_NAME_LEN      28      255
 *      block map         addr[13] indirect tree   →  EXTENTS
 *      dir entry         fixed 32 B               →  variable, 4-aligned
 *
 *  The block-map difference is the important one: 01_fsa's bmap() walks
 *  addr[] with direct and indirect levels — Bach's scheme.  UNFS stores
 *  4 inline extents plus an overflow extent-tree block.  Those are
 *  incompatible on-disk layouts, so bmap() as written CANNOT read a UNFS
 *  inode: it would read bytes 10..60 of a 256-byte extent-based inode as
 *  though they were block pointers.
 *
 *  ── layout ────────────────────────────────────────────────────────────
 *      block 0          superblock          unfs_sb_t      (4096 B)
 *      block 1          journal superblock  uiox_jr_sb_disk_t
 *      block 2..257     journal log area    256 blocks = 1 MB
 *      block 258        block group 0 desc
 *      block 259        block bitmap        group 0
 *      block 260        inode bitmap        group 0
 *      block 261..268   inode table         group 0, 8 blocks
 *      block 269+       data blocks         group 0
 *      ...              repeat per group
 *
 *  At UNFS_BLOCK_SIZE 4096 the journal comment is self-consistent: 256
 *  blocks IS 1 MB.  The earlier confusion came from reading it against a
 *  512-byte block size.
 *
 *  @version 1.0.0  @date 2026-09-26
 */
#ifndef UNFS_FORMAT_H
#define UNFS_FORMAT_H

#include "uiox_base_types.h"

/* ═════════════════════════════════════════════════════════════════════
 * Magic and version
 *
 * A bootloader that cannot recognise the magic must refuse the volume
 * rather than guess — a wrong read here loads garbage as a kernel.
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_MAGIC            0x554E4653UL   /* "UNFS"                 */
#define UNFS_VERSION_MAJOR    1u
#define UNFS_VERSION_MINOR    0u

/* ═════════════════════════════════════════════════════════════════════
 * Geometry
 *
 * UNFS_BLOCK_SIZE is 4096 and everything else follows from it.  This is
 * NOT bcache's 512-byte sector — see the note under SECTOR vs BLOCK.
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_BLOCK_SIZE        4096u          /* always 4 KB            */
#define UNFS_INODE_SIZE        256u           /* bytes per inode        */
#define UNFS_INODES_PER_BLOCK  (UNFS_BLOCK_SIZE / UNFS_INODE_SIZE)   /* 16 */
#define UNFS_INODES_PER_GROUP  512u
#define UNFS_BLOCKS_PER_GROUP  8192u
#define UNFS_NAME_MAX          255u

/* ── where the predefined regions begin ────────────────────────────── */
#define UNFS_SB_BLOCK          0u             /* superblock             */
#define UNFS_JR_SB_BLOCK       1u             /* journal superblock     */
#define UNFS_JR_LOG_FIRST      2u             /* journal log area       */
#define UNFS_JR_LOG_BLOCKS     256u           /* 256 x 4 KB = 1 MB      */
#define UNFS_GROUP0_DESC       258u
#define UNFS_GROUP0_BBMAP      259u
#define UNFS_GROUP0_IBMAP      260u
#define UNFS_GROUP0_ITABLE     261u
#define UNFS_ITABLE_BLOCKS     8u             /* 8 x 16 = 128 inodes    */
#define UNFS_GROUP0_DATA       269u

/* ── reserved inode numbers ──────────────────────────────────────────
 * The root directory is inode 2, as in ext2: inode 1 is the traditional
 * "bad blocks" inode and is never a live file, so numbering starts at 2.
 *
 * MISSING from an earlier revision of this header, which is why
 * namei.c's `iget_dev(dev, ROOT_INO)` failed to compile — 01_fsa cannot
 * include uiox_boot_unfs.h (that is the bootloader's header, and the
 * dependency runs the other way), so every constant the kernel needs
 * has to be present HERE.
 *
 * These are the bootloader's values verbatim; if the two ever disagree,
 * the bootloader is right and this is the defect. */
#define UNFS_ROOT_INO          2u    /* root directory inode           */
#define UNFS_RESERVED_INOS     10u   /* inodes 1..10 reserved          */

/* ── SECTOR vs BLOCK — the mismatch that must not be conflated ────────
 * 00_buffcache moves 512-byte sectors (BCACHE_SECTOR_SIZE).  UNFS is
 * defined in 4096-byte blocks.  One UNFS block is therefore EIGHT
 * buffer-cache blocks, and the conversion is explicit here rather than
 * assumed anywhere:
 *
 *     device_sector = unfs_block * UNFS_SECTORS_PER_BLOCK
 *
 * A reader that passes a UNFS block number straight to bread() reads the
 * first 512 bytes of the wrong location — one eighth of the way in. */
#define UNFS_SECTORS_PER_BLOCK (UNFS_BLOCK_SIZE / 512u)   /* 8 */

/* ═════════════════════════════════════════════════════════════════════
 * Error codes
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_EOK             0
#define UNFS_EIO            -100
#define UNFS_EINVAL         -101
#define UNFS_ENOENT         -102
#define UNFS_ENOTSUP        -103
#define UNFS_ECORRUPT       -117   /* checksum mismatch                 */

/* ═════════════════════════════════════════════════════════════════════
 * File type and permission encoding
 *
 * i_mode carries both: type in the high bits, permissions in the low
 * nine.  These are the ON-DISK values and must not change.
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_IFMT        0170000u
#define UNFS_IFREG       0100000u
#define UNFS_IFDIR       0040000u
#define UNFS_IFCHR       0020000u
#define UNFS_IFBLK       0060000u
#define UNFS_IFIFO       0010000u
#define UNFS_IFLNK       0120000u

#define UNFS_ISUID       0004000u
#define UNFS_ISGID       0002000u
#define UNFS_ISVTX       0001000u
#define UNFS_IRUSR       0000400u
#define UNFS_IWUSR       0000200u
#define UNFS_IXUSR       0000100u
#define UNFS_IRGRP       0000040u
#define UNFS_IWGRP       0000020u
#define UNFS_IXGRP       0000010u
#define UNFS_IROTH       0000004u
#define UNFS_IWOTH       0000002u
#define UNFS_IXOTH       0000001u

/* ═════════════════════════════════════════════════════════════════════
 * Extent flags
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_EXT_LEAF         0x0001u  /* leaf extent (has data)       */
#define UNFS_EXT_HOLE         0x0002u  /* sparse / hole — reads zero   */
#define UNFS_EXT_COW          0x0004u  /* copy-on-write pending        */

/* ═════════════════════════════════════════════════════════════════════
 * Forward declarations
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct unfs_sb         unfs_sb_t;
typedef struct unfs_group_desc unfs_group_desc_t;
typedef struct unfs_inode      unfs_inode_t;
typedef struct unfs_extent     unfs_extent_t;
typedef struct unfs_dirent     unfs_dirent_t;

/* ═════════════════════════════════════════════════════════════════════
 * Extent — maps logical file blocks to physical device blocks
 *
 * UNFS uses extents, NOT Bach's addr[13] indirect tree.  01_fsa's bmap()
 * implements the indirect scheme and therefore cannot read a UNFS inode
 * without being rewritten to walk extents instead.
 * ═════════════════════════════════════════════════════════════════════ */
struct unfs_extent {
    uint32_t  e_logical;    /* first logical block in file            */
    uint32_t  e_physical;   /* first physical block on device         */
    uint16_t  e_len;        /* number of blocks in this extent        */
    uint16_t  e_flags;      /* UNFS_EXT_* flags                       */
} __attribute__((packed));

/* ═════════════════════════════════════════════════════════════════════
 * Superblock — one 4096-byte block
 *
 * Fields before _pad total exactly 172 bytes; the pad takes it to the
 * full block so the superblock is always one readable unit.
 * ═════════════════════════════════════════════════════════════════════ */
struct unfs_sb {
    uint32_t  s_magic;            /* UNFS_MAGIC = 0x554E4653            */
    uint16_t  s_version_major;
    uint16_t  s_version_minor;
    uint32_t  s_block_size;       /* always 4096                        */
    uint32_t  s_inode_size;       /* always 256                         */
    uint64_t  s_block_count;      /* total blocks on volume             */
    uint64_t  s_free_blocks;
    uint32_t  s_inode_count;      /* total inodes                       */
    uint32_t  s_free_inodes;
    uint32_t  s_inodes_per_group;
    uint32_t  s_blocks_per_group;
    uint32_t  s_group_count;      /* number of block groups             */
    uint32_t  s_jr_block;         /* journal superblock block number    */
    uint32_t  s_jr_size;          /* journal log size in blocks         */
    uint64_t  s_mount_time_ns;    /* last mount timestamp (ns)          */
    uint64_t  s_write_time_ns;    /* last write timestamp (ns)          */
    uint32_t  s_mount_count;      /* mounts since last fsck             */
    uint32_t  s_max_mount_count;
    uint8_t   s_clean;            /* 1 = cleanly unmounted              */
    uint8_t   s_cow_enabled;      /* 1 = copy-on-write active           */
    uint8_t   s_checksum_type;    /* 0=none 1=CRC32C                    */
    uint8_t   s_compress;         /* 0=none (reserved for future)       */
    uint8_t   s_volume_name[64];
    uint8_t   s_uuid[16];         /* volume UUID                        */
    uint32_t  s_sb_checksum;      /* CRC32C of bytes 0..172-4           */
    uint8_t   _pad[UNFS_BLOCK_SIZE - 172];
} __attribute__((packed));

typedef char unfs_sb_size_assert[
   (sizeof(unfs_sb_t) == UNFS_BLOCK_SIZE) ? 1 : -1];

/* ═════════════════════════════════════════════════════════════════════
 * Block group descriptor
 * ═════════════════════════════════════════════════════════════════════ */
struct unfs_group_desc {
    uint32_t  bg_block_bitmap;  /* block number of block bitmap         */
    uint32_t  bg_inode_bitmap;  /* block number of inode bitmap         */
    uint32_t  bg_inode_table;   /* first block of inode table           */
    /* The bootloader reads only these three; the kernel appends free
     * counts and a checksum.  Anything added must go AFTER them, or the
     * bootloader's fixed offsets break. */
} __attribute__((packed));

/* ═════════════════════════════════════════════════════════════════════
 * Inode — 256 bytes on disk
 *
 * NOTE the extent tree in place of Bach's addr[13].  This is the single
 * biggest divergence from 01_fsa, which implements the indirect scheme.
 * ═════════════════════════════════════════════════════════════════════ */
struct unfs_inode {
    uint16_t  i_mode;           /* file type + permissions              */
    uint16_t  i_uid;
    uint16_t  i_gid;
    uint16_t  i_nlink;          /* hard link count                      */
    uint64_t  i_size;           /* file size in bytes                   */
    uint64_t  i_atime_ns;       /* last access (ns since epoch)         */
    uint64_t  i_mtime_ns;       /* last modification                    */
    uint64_t  i_ctime_ns;       /* last status change                   */
    uint32_t  i_blocks;         /* 512-byte blocks allocated            */
    uint32_t  i_flags;          /* misc flags                           */

    /* MAC security label — 33_PCS/05_sec */
    uint8_t   i_mac_label[16];
    uint32_t  i_mac_flags;

    /* Extent tree — 4 inline extents */
    unfs_extent_t i_extents[4]; /* 4 x 8 = 32 bytes                     */
    uint32_t  i_extent_tree;    /* overflow extent-tree block (0=none)  */

    /* Inline symlink target (when i_size <= 60).  FAT32 had no symlinks,
     * so this field exists only because UNFS replaced it. */
    uint8_t   i_inline[60];

    uint32_t  i_checksum;       /* CRC32C of bytes 0..251               */

    /* ── EXPLICIT PAD — the record is 256 B, the fields total 168 ─────
     * The sum above is exact:
     *
     *      4 x uint16                     8   (mode, uid, gid, nlink)
     *      4 x uint64                    32   (size, atime, mtime, ctime)
     *      2 x uint32                     8   (blocks, flags)
     *      i_mac_label[16] + i_mac_flags  20
     *      i_extents[4]  (4 x 12)           48
     *      i_extent_tree                  4
     *      i_inline[60]                  60
     *      i_checksum                     4
     *      ---------------------------------
     *      declared                     184
     *
     * UNFS_INODE_SIZE is 256, so unfs_inode_size_assert below evaluated
     * to -1 and this header did not compile.  The identical struct in
     * 01_uBoot/include/uiox_boot_unfs.h — the source of truth named at
     * the top of this file — was short by the same 72 bytes, so this is
     * a format-definition gap, not a defect in either copy.
     *
     * The pad closes the gap WITHOUT deciding the format: i_rdev,
     * i_generation and i_dtime are the fields that plausibly belong
     * here, and 01_fsa's mknod explicitly notes it has no rdev slot —
     * but nothing consumes them yet, so choosing offsets now would be
     * guessing at on-disk layout.
     *
     * CONSEQUENCE, stated plainly: an inode written today has 72 bytes that mean nothing.  The geometry — block 261, 8 blocks, 16 inodes
     * per block, 128 per group — is unchanged, which is what the
     * bootloader's reader already assumes.  When the fields are decided
     * they REPLACE the pad; anything appended AFTER it makes sizeof 257
     * and fails this assertion in the other direction. */
    uint8_t   _pad[72];
} __attribute__((packed));

typedef char unfs_inode_size_assert[
   (sizeof(unfs_inode_t) == UNFS_INODE_SIZE) ? 1 : -1];

/* ═════════════════════════════════════════════════════════════════════
 * Directory entry — VARIABLE length, 4-byte aligned
 *
 * Not the fixed 32-byte record 01_fsa's namei.h declares.  A reader that
 * walks these as a fixed array mis-parses every entry after the first.
 * ═════════════════════════════════════════════════════════════════════ */
struct unfs_dirent {
    uint32_t  d_ino;            /* inode number                         */
    uint16_t  d_rec_len;        /* length of this record                */
    uint8_t   d_name_len;       /* length of name (no NUL)              */
    uint8_t   d_type;           /* file type, DT_-style                 */
    char      d_name[];         /* variable, NUL-terminated             */
} __attribute__((packed));

/* d_type values, so a walker needs no iget() per entry */
#define UNFS_DT_UNKNOWN  0u
#define UNFS_DT_FIFO     1u
#define UNFS_DT_CHR      2u
#define UNFS_DT_DIR      4u
#define UNFS_DT_BLK      6u
#define UNFS_DT_REG      8u
#define UNFS_DT_LNK     10u

/* ═════════════════════════════════════════════════════════════════════
 * Compatibility names
 *
 * 01_fsa's fs_types.h used Bach's names.  UNFS_INODE_START is provided so
 * a migrating file still compiles, and so the REMAP is visible.
 *
 * WARNING: BLOCK_SIZE was 512 in fs_types.h and is 4096 here.  Any code
 * that sized a buffer or computed a sector offset from it changes
 * meaning.  That is the one alias that can hurt, so it is not defined
 * here — use UNFS_BLOCK_SIZE explicitly and convert through
 * UNFS_SECTORS_PER_BLOCK.
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_INODE_START  UNFS_GROUP0_ITABLE

#endif /* UNFS_FORMAT_H */
