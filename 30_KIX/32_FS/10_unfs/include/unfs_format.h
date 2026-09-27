/*
 *  30_KIX/32_FS/10_unfs/include/unfs_format.h
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
 *      block 258        block group 0 desc
 *      block 259        block bitmap        group 0
 *      block 260        inode bitmap        group 0
 *      block 261..268   inode table         group 0, 8 blocks
 *      block 269+       data blocks         group 0
 *      ...              repeat per group
 *
 *
 *  @version 1.0.0  @date 2026-09-26
 */
#ifndef UNFS_FORMAT_H
#define UNFS_FORMAT_H

#include "uiox_base_types.h"

/* ── the uintN_t family ────────────────────────────────────────────────
 * uiox_base_types.h defines uiox_uint32_t and friends; it does NOT
 * define the bare uint32_t spelling, and -nostdinc puts <stdint.h> out
 * of reach.  55 fields in this file use the bare spelling, so the types
 * are defined here in terms of the uiox_ ones — same types, two
 * spellings, which is the same arrangement uiox_base_types.h already
 * uses for its own aliases. */
/* NO offsetof of any kind is defined here.  uiox_klibc.h already has
 * it, and neither form can measure a struct from inside its own
 * definition: ((type *)0)->member needs a complete type, and
 * __builtin_offsetof is rejected on an incomplete one under -std=c11.
 * Both pads below are therefore named literals, verified by the field
 * sums printed above each struct and enforced by the asserts. */

#ifndef UIOX_BARE_INT_TYPES
#define UIOX_BARE_INT_TYPES
typedef uiox_uint8_t   uint8_t;
typedef uiox_uint16_t  uint16_t;
typedef uiox_uint32_t  uint32_t;
typedef uiox_int8_t    int8_t;
typedef uiox_int16_t   int16_t;
typedef uiox_int32_t   int32_t;
#endif /* UIOX_BARE_INT_TYPES */

/* The 64-bit spellings.
 *
 * These must match uiox_klibc.h EXACTLY — not uiox_base_types.h.
 * uiox_base_types.h's uiox_uint64_t is `unsigned long` here, but
 * uiox_klibc.h (reached first, via 01_fsa/fs_types.h) declares uint64_t
 * as `unsigned long long`, and only one of the two can win.  So the
 * word is spelled the way uiox_klibc.h spells it.
 *
 * __UINT64_TYPE__ is NOT usable: on aarch64-elf it expands to
 * `unsigned long`, which is the other side of the conflict. */
#ifndef UIOX_BARE_INT64_TYPES
#define UIOX_BARE_INT64_TYPES
typedef unsigned long long  uint64_t;
typedef long long           int64_t;
#endif /* UIOX_BARE_INT64_TYPES */

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
/* Blocks 1..257 are RESERVED and unused.  They were earmarked for a
 * write-ahead log, which is not part of this format and not wired in,
 * so nothing reads or writes them.  No constant is defined for the
 * range: a reserved span nobody touches needs no name, and a name
 * invites a reader to believe a log exists here.  UNFS_GROUP0_DESC
 * begins at 258, leaving the span untouched. */
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
 * The declared fields total 160 bytes (counted below), so the pad is
 * 4096 - 160 = 3936 and the struct is exactly one block.
 *
 *     4  s_magic            4  s_inode_size        4  s_group_count
 *     2  s_version_major    8  s_block_count       8  s_mount_time_ns
 *     2  s_version_minor    8  s_free_blocks       8  s_write_time_ns
 *     4  s_block_size       4  s_inode_count       4  s_mount_count
 *                           4  s_free_inodes       4  s_max_mount_count
 *                           4  s_inodes_per_group  1  s_clean
 *                           4  s_blocks_per_group  1  s_checksum_type
 *                                                  1  s_compress
 *                                                 64  s_volume_name
 *                                                 16  s_uuid
 *                                                  4  s_sb_checksum
 *     ─────────────────────────────────────────────────────────────────
 *                                                  160
 * ═════════════════════════════════════════════════════════════════════ */
#define UNFS_SB_FIELDS_BYTES     163u
#define UNFS_SB_PAD_BYTES      (UNFS_BLOCK_SIZE - UNFS_SB_FIELDS_BYTES)
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
    uint64_t  s_mount_time_ns;    /* last mount timestamp (ns)          */
    uint64_t  s_write_time_ns;    /* last write timestamp (ns)          */
    uint32_t  s_mount_count;      /* mounts since last fsck             */
    uint32_t  s_max_mount_count;
    uint8_t   s_clean;            /* 1 = cleanly unmounted              */
    uint8_t   s_checksum_type;    /* 0=none 1=CRC32C                    */
    uint8_t   s_compress;         /* 0=none (reserved for future)       */
    uint8_t   s_volume_name[64];
    uint8_t   s_uuid[16];         /* volume UUID                        */
    uint32_t  s_sb_checksum;      /* CRC32C of bytes 0..155 */

    /* ── THE PAD IS A NAMED CONSTANT ──────────────────────────────────
     * It cannot be derived from the struct: __builtin_offsetof rejects a
     * type that is still being defined, which is what this struct is at
     * this point (-std=c11, not gnu11).  So the count is a literal, and
     * UNFS_SB_PAD_BYTES below states where it comes from.
     *
     * The original [UNFS_BLOCK_SIZE - 172] was wrong: the fields above
     * total 168, not 172, so the array came out 3928 and sizeof was
     * 4096 + 4, making the assert negative.  Add a field above and the
     * assert below fires — which is the point of having it. */
    uint8_t   _pad[UNFS_SB_PAD_BYTES];
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
 * Declared fields total 136 bytes, so the pad is 256 - 136 = 120:
 *
 *      8  4 x uint16   mode, uid, gid, nlink
 *     32  4 x uint64   size, atime, mtime, ctime
 *      8  2 x uint32   blocks, flags
 *     20  i_mac_label[16] + i_mac_flags
 *     48  i_extents[4] — unfs_extent_t is 12 bytes, so 4 x 12
 *      4  i_extent_tree
 *     60  i_inline[60]
 *      4  i_checksum
 *     ────────────────────────────────────────────────
 *    136
 *
 * (An earlier revision claimed 168 and paired it with _pad[72], giving
 *  208, and separately called i_extents[4] "4 x 8 = 32" when the extent
 *  is 4 + 4 + 2 + 2 = 12 bytes.  Both are corrected.)
 *
 * NOTE the extent tree in place of Bach's addr[13].  This is the single
 * biggest divergence from 01_fsa, which implements the indirect scheme.
 * ═════════════════════════════════════════════════════════════════════ */
/* Declared fields total 136 bytes; 256 - 136 = 120.  See the field list
 * in the block comment above. */
#define UNFS_INODE_FIELDS_BYTES  184u
#define UNFS_INODE_PAD_BYTES     (UNFS_INODE_SIZE - UNFS_INODE_FIELDS_BYTES)

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
    unfs_extent_t i_extents[4]; /* 4 x 12 = 48 bytes                    */
    uint32_t  i_extent_tree;    /* overflow extent-tree block (0=none)  */

    /* Inline symlink target (when i_size <= 60).  FAT32 had no symlinks,
     * so this field exists only because UNFS replaced it. */
    uint8_t   i_inline[60];

    uint32_t  i_checksum;       /* CRC32C of bytes 0..251               */

    /* ── THE PAD — 256 - 136 = 120 bytes ─────────────────────────────
     * The fields above total 136, not the 168 an earlier revision of
     * this comment claimed:
     *
     *      8  4 x uint16     mode, uid, gid, nlink
     *     32  4 x uint64     size, atime, mtime, ctime
     *      8  2 x uint32     blocks, flags
     *     20  i_mac_label[16] + i_mac_flags
     *     48  i_extents[4] — unfs_extent_t is 12 bytes, so 4 x 12
     *      4  i_extent_tree
     *     60  i_inline[60]
     *      4  i_checksum
     *     ──────────────────────────────────────────────
     *    136
     *
     * The old sum went wrong in two ways, and the field list at the top
     * of this struct carried the first of them too:
     *
     *   1. unfs_extent_t is 4 + 4 + 2 + 2 = 12 bytes, so four extents
     *      are 48.  The declaration's own trailing comment said "4 x 8
     *      = 32" — corrected there as well.
     *   2. The old list was the declared FIELD ORDER, and the pad sits
     *      inside a struct whose fields are laid out once.  Reading it
     *      as repeating gave a spurious x4 and a total of 672.
     *
     * With the fields at 136 and UNFS_INODE_SIZE at 256, the original
     * _pad[72] produced sizeof 208, so unfs_inode_size_assert below
     * evaluated to -1 and this header did not compile.  The pad is
     * therefore UNFS_INODE_PAD_BYTES, a named constant, and the assert
     * guards it: add a field above and the assert fires rather than the
     * layout quietly shifting.
     *
     * The padded bytes are UNDECIDED, not spare.  i_rdev, i_generation
     * and i_dtime are the fields that plausibly belong here — 01_fsa's
     * mknod notes it has no rdev slot — but nothing consumes them yet,
     * so choosing offsets now would be guessing at on-disk layout.  An
     * inode written today therefore has 120 bytes that mean nothing.
     *
     * The GEOMETRY is unaffected, and that is what matters to the
     * bootloader's reader: 256 bytes per inode, 16 per block, 128 per
     * group, table at block 261.  When the fields are decided they
     * REPLACE the pad; anything appended AFTER it makes sizeof 257 and
     * fails this assertion in the other direction. */
    uint8_t   _pad[UNFS_INODE_PAD_BYTES];
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

/* ═════════════════════════════════════════════════════════════════════
 * DISK-SIDE COMPATIBILITY NAMES — for 10_unfs
 *
 * 10_unfs was written against a header (unfs_disk.h) that used a
 * disk-flavoured vocabulary: _disk_t struct names, a byte OFFSET for the
 * superblock, a versioned magic, and a 512-byte inode slot.  That header
 * has been removed because it disagreed with the bootloader on three
 * layout facts, and this file is the survivor.
 *
 * Rather than leave 10_unfs orphaned, the names it needs are defined
 * HERE, each resolving to the geometry the bootloader actually reads:
 *
 *     UNFS_INODE_BYTES   -> 256, the same as UNFS_INODE_SIZE
 *     UNFS_SB_OFFSET     -> 0, because the superblock IS block 0
 *     UNFS_MAGIC_V1/V2   -> the single UNFS_MAGIC the bootloader uses
 *
 * Nothing above this line changes: 01_fsa, 10_scfs and 00_buffcache are
 * compiled against this header and these names are purely additive.
 *
 * The struct aliases are typedefs, not second definitions — one layout,
 * two names, so nothing can drift.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── inode slot and inode numbering ─────────────────────────────────── */
#define UNFS_INODE_BYTES      UNFS_INODE_SIZE      /* 256, not 512      */
#define UNFS_NIL_INO          0u                   /* inode number 0    */

/* ── superblock position ──────────────────────────────────────────────
 * UNFS_SB_OFFSET is a BYTE offset within block 0.  The superblock
 * occupies block 0 in full, so the offset is 0 — 1024 was the value in
 * the removed header, and it read the wrong 4 KB on every mount. */
#define UNFS_SB_OFFSET        0u
#define UNFS_SB_SIZE          UNFS_BLOCK_SIZE

/* ── group descriptor table ─────────────────────────────────────────── */
#define UNFS_GDT_BLOCK        UNFS_GROUP0_DESC     /* 258               */

/* ── versioned magic ──────────────────────────────────────────────────
 * The bootloader recognises ONE magic and does not version it.  Mapping
 * both names onto it makes 10_unfs's version test resolve to "v2", which
 * is the read-write path — correct, since this is the only format. */
#define UNFS_MAGIC_V1         UNFS_MAGIC
#define UNFS_MAGIC_V2         UNFS_MAGIC

/* Every incompat feature this build understands.  mkfs writes none of the
 * optional features today, so the mask is empty and unfs_mount.c's check
 * is (s_feature_incompat & ~0) == 0. */
#define UNFS_SUPPORTED_INCOMPAT   0u

/* ── struct aliases — one layout, two vocabularies ──────────────────── */
typedef unfs_sb_t          unfs_sb_disk_t;
typedef unfs_inode_t       unfs_inode_disk_t;
typedef unfs_group_desc_t  unfs_group_disk_t;

/* ── 64-bit size and timestamp access ─────────────────────────────────
 * unfs_inode_t stores i_size and the three timestamps as single uint64_t
 * fields, so there are no halves to reassemble.  unfs_iget.c calls
 * unfs_mk64(d->i_size_lo, d->i_size_hi) — fields that do not exist in
 * this struct.  The port in unfs_iget.c reads the single fields
 * directly; this macro exists only so a stale call site fails loudly
 * rather than silently reading garbage. */
#define unfs_mk64(lo, hi)     ((uint64_t)(lo))

#endif /* UNFS_FORMAT_H */
