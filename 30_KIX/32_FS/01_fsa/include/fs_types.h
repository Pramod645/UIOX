/*
 *  30_KIX/32_FS/01_fsa/include/fs_types.h
 *
 *  Freestanding FS type definitions — no system headers.
 *
 *  ── what this header is, and what it is not ───────────────────────────
 *  This is the GEOMETRY contract shared by 01_fsa, 10_scfs and the mkfs
 *  path.  It is the de-facto UNFS format today: every constant below is a
 *  layout decision, and both readers (01_fsa in the kernel, 01_uBoot at
 *  boot time) must agree on all of them or they read different bytes.
 *
 *  It is NOT a substitute for a shared format header.  When 10_unfs lands
 *  the superblock, group descriptors and journal offsets move into a
 *  unfs_format.h that 01_uBoot ALSO includes — so the bootloader and the
 *  kernel cannot drift.  Until then, this file is that contract by
 *  itself, which is why the constants below carry their units and their
 *  layout position in comments.
 *
 *  ── CHANGED in this revision (v2.0.0) ─────────────────────────────────
 *   · MAX_BLOCKS / MAX_INODES raised from the old simulation sizes.
 *     1024 blocks at 512 B is 512 KB — the UNFS journal alone occupies
 *     blocks 2..257, so the previous geometry could not hold a real
 *     volume.  524288 blocks = 256 MiB at 512 B.
 *   · INODE_START_BLOCK corrected from 2 to the UNFS group-0 position.
 *     Block 2 is the JOURNAL LOG AREA, not the inode list — the old
 *     value made inode_disk_read() read journal bytes as inodes.
 *   · DATA_START_BLOCK corrected to the first block after group 0's
 *     inode table.
 *   · GROUP_* constants added: the block-group structure 01_fsa needs to
 *     know about, and the bootloader already assumes.
 *   · MAX_DIR_ENTRIES is now COMPUTED from sizeof(DirEntry) rather than
 *     hardcoded to 16, which was only true while BLOCK_SIZE was 512 and
 *     the entry was 32 bytes.
 *   · MAX_BUFS removed — the old buffer.c sized its own cache with it;
 *     00_buffcache uses NUM_BUFFERS from bcache_types.h instead.
 *   · A _Static_assert pins sizeof(DiskInode) so a future field addition
 *     cannot silently change INODES_PER_BLOCK and with it every inode
 *     offset on disk.
 *
 *  ── the UNFS on-disk layout these constants describe ──────────────────
 *      block 0          superblock            unfs_sb_t
 *      block 1          journal superblock    uiox_jr_sb_disk_t
 *      block 2..257     journal log area      256 blocks
 *      block 258        block group 0 desc
 *      block 259        block bitmap          group 0
 *      block 260        inode bitmap          group 0
 *      block 261..268   inode table           group 0, 8 blocks
 *      block 269+       data blocks           group 0
 *
 *  @version 2.0.0  @date 2026-09-26
 */
#ifndef UIOX_FS_TYPES_H
#define UIOX_FS_TYPES_H

#include "uiox_klibc.h"   /* replaces <stdint.h> <stdbool.h> <stddef.h> */

/* ═════════════════════════════════════════════════════════════════════
 * Filesystem geometry
 *
 * BLOCK_SIZE is the SECTOR size, and it must match
 * BCACHE_SECTOR_SIZE in bcache_types.h — 01_fsa's every buffer call
 * takes a blkno counted in these units.
 * ═════════════════════════════════════════════════════════════════════ */
#define BLOCK_SIZE          512u          /* bytes per disk block         */

/* 524288 × 512 B = 256 MiB.  The old 1024 (= 512 KB) could not hold the
 * UNFS journal, which alone spans 256 blocks. */
#define MAX_BLOCKS          524288u       /* total volume blocks          */

/* 4096 inodes × 64 B = 256 KB of inode table, 8 groups of 8 blocks. */
#define MAX_INODES          4096u         /* total inodes on disk         */

/* ═════════════════════════════════════════════════════════════════════
 * The block-group structure
 *
 * UNFS is ext2-shaped: the volume is divided into groups, each holding
 * its own descriptor, block bitmap, inode bitmap and inode table.  The
 * boot path needs these numbers to find inode 1 before any cache exists,
 * which is why they are layout constants rather than 10_unfs internals.
 * ═════════════════════════════════════════════════════════════════════ */
#define GROUP0_DESC_BLOCK   258u          /* block group 0 descriptor     */
#define GROUP0_BBMAP_BLOCK  259u          /* block bitmap, group 0        */
#define GROUP0_IBMAP_BLOCK  260u          /* inode bitmap, group 0        */
#define GROUP0_ITABLE_BLOCK 261u          /* inode table begins, group 0  */
#define ITABLE_BLOCKS       8u            /* blocks of inode table /group */
#define GROUP_DATA_BLOCKS   2048u         /* data blocks per group        */

/* Bach's names, kept because 01_fsa's sources already use them.
 *
 * INODE_START_BLOCK is the group-0 inode table position — NOT 2, which
 * is the journal log area.  inode_disk_read() computes
 *     blkno = ((ino-1) / INODES_PER_BLOCK) + INODE_START_BLOCK
 * so this constant is what makes inode 1 resolve to block 261. */
#define INODE_START_BLOCK   GROUP0_ITABLE_BLOCK
#define DATA_START_BLOCK    (GROUP0_ITABLE_BLOCK + ITABLE_BLOCKS)

/* ── journal, occupying the front of the volume ───────────────────────
 * Fixed position so a recovery pass can run before anything else parses. */
#define JR_SB_BLOCK         1u
#define JR_LOG_FIRST_BLOCK  2u
#define JR_LOG_BLOCKS       256u

/* ── caches ───────────────────────────────────────────────────────────
 * MAX_INCACHE is 01_fsa's in-core inode cache (inode.h: icache[]).
 * The buffer pool is NOT sized here — 00_buffcache owns NUM_BUFFERS. */
#define MAX_INCACHE         32u           /* in-core inode cache size     */

/* ── directory entries ────────────────────────────────────────────────
 * DirEntry is { uint32_t ino; char name[MAX_NAME_LEN]; } = 32 bytes at
 * MAX_NAME_LEN 28.  Computed, not hardcoded: changing MAX_NAME_LEN must
 * change this, and a literal 16 would silently stop being true. */
#define MAX_NAME_LEN        28u           /* max filename component       */
#define MAX_PATH_LEN        256u          /* max full path                */
#define DIRENT_SIZE         (sizeof(uint32_t) + MAX_NAME_LEN)

/* ═════════════════════════════════════════════════════════════════════
 * Inode block address layout — Bach Ch.4 §3
 *
 * addr[] is 13 entries: 10 direct, then single-, double- and
 * triple-indirect.  bmap() walks all four levels.
 * ═════════════════════════════════════════════════════════════════════ */
#define NDIRECT     10u                   /* direct block pointers        */
#define NINDIRECT   1u                    /* single-indirect pointer      */
#define NDINDIRECT  1u                    /* double-indirect pointer      */
#define NTINDIRECT  1u                    /* triple-indirect pointer      */
#define PTRS_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))   /* 128          */

/* ═════════════════════════════════════════════════════════════════════
 * File types
 *
 * The enum VALUE is the nibble stored in mode = (ftype << 12) | perm.
 * inode_type() reads it back; SCFS's SCFS_IS_* macros test it the same
 * way.  Changing a value here reinterprets every inode already on disk.
 * ═════════════════════════════════════════════════════════════════════ */
typedef enum {
    FT_FREE    = 0,
    FT_REGULAR = 1,
    FT_DIR     = 2,
    FT_CHAR    = 3,
    FT_BLOCK   = 4,
    FT_FIFO    = 5,
    FT_SYMLINK = 6
} FileType;

/* ═════════════════════════════════════════════════════════════════════
 * Permission bits
 * ═════════════════════════════════════════════════════════════════════ */
#define PERM_UR  0400
#define PERM_UW  0200
#define PERM_UX  0100
#define PERM_GR  0040
#define PERM_GW  0020
#define PERM_GX  0010
#define PERM_OR  0004
#define PERM_OW  0002
#define PERM_OX  0001

/* ═════════════════════════════════════════════════════════════════════
 * Inode dirty flags — Bach's three conditions
 *
 *   ACCESSED  a read happened
 *   CHANGED   the inode's metadata moved
 *   MODIFIED  the file's DATA moved
 *
 * fdatasync() tests MODIFIED to decide whether the data must reach disk;
 * a pure timestamp update sets only CHANGED and is skipped.
 * ═════════════════════════════════════════════════════════════════════ */
#define IFLAG_ACCESSED  0x01
#define IFLAG_CHANGED   0x02
#define IFLAG_MODIFIED  0x04

/* ═════════════════════════════════════════════════════════════════════
 * Super block free lists — Bach Ch.4 §5..§8
 * ═════════════════════════════════════════════════════════════════════ */
#define SB_FREE_INODE_MAX   32
#define SB_FREE_BLOCK_MAX   50

/* ═════════════════════════════════════════════════════════════════════
 * Geometry that cannot be derived until the inode layout is known
 *
 * INODES_PER_BLOCK depends on sizeof(DiskInode), which inode.h supplies.
 * The macro expands at USE, not here, so ordering is not a problem — but
 * the arithmetic only holds if DiskInode is exactly 64 bytes, so v2.0.0
 * pins it.  If a field is added and this assert fires, INODE_START_BLOCK,
 * the group table size and every inode offset on disk change with it.
 * ═════════════════════════════════════════════════════════════════════ */
#define INODES_PER_BLOCK    (BLOCK_SIZE / sizeof(DiskInode))

#endif /* UIOX_FS_TYPES_H */
