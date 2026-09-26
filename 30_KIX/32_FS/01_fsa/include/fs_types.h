/*
 *  30_KIX/32_FS/01_fsa/include/fs_types.h
 *
 *  Bach's inode-level types — the parts that are NOT the on-disk format.
 *
 *  ── why this file is now small ────────────────────────────────────────
 *  Everything it used to carry that described the DISK moved to
 *  10_unfs/include/unfs_format.h:
 *
 *      BLOCK_SIZE            → UNFS_BLOCK_SIZE (4096, not 512)
 *      MAX_BLOCKS            → unfs_sb_t.s_block_count
 *      MAX_INODES            → unfs_sb_t.s_inode_count
 *      INODE_START_BLOCK     → UNFS_GROUP0_ITABLE
 *      DATA_START_BLOCK      → UNFS_GROUP0_DATA
 *      GROUP0_*, ITABLE_*    → UNFS_GROUP0_*, UNFS_ITABLE_BLOCKS
 *      JR_*                  → UNFS_JR_*
 *      MAX_NAME_LEN          → UNFS_NAME_MAX (255, not 28)
 *      NDIRECT…NTINDIRECT    → EXTENTS (unfs_extent_t)
 *      PTRS_PER_BLOCK        → EXTENTS
 *      DIRENT_SIZE           → unfs_dirent_t (variable length)
 *
 *  One shared header means the bootloader and the kernel cannot disagree
 *  about layout — which was the whole reason for the move.
 *
 *  What remains below is what Bach's Ch.4 algorithms need and the disk
 *  does not care about: the in-core cache size, the dirty flags, the
 *  permission bits, and the free-list sizes.  A bootloader has no use for
 *  any of them, which is the test for whether something belongs here.
 *
 *  ── THE TYPE-ENCODING CONFLICT IS RESOLVED ───────────────────────────
 *  This file used to define FileType as an enum; it no longer does, and
 *  inode_type() is gone from inode.h with it.
 *
 *  The reason is not style.  FileType packed into the SAME bits as the
 *  format's type field, so the two encodings COLLIDED:
 *
 *      FT_FREE 0  FT_REGULAR 1  FT_DIR 2  FT_CHAR 3  FT_BLOCK 4
 *      (FT_DIR     << 12) = 0x2000 = UNFS_IFCHR
 *      (FT_REGULAR << 12) = 0x1000 = UNFS_IFIFO
 *
 *  mkfs wrote FT_DIR — 0x2000 in the mode word — and inode_type() read
 *  the nibble back as FT_BLOCK. That is why a directory reported as a
 *  block device.
 *
 *  unfs_format.h says of its type block "These are the ON-DISK values
 *  and must not change", so it is the authority.  The in-core side now
 *  uses inode_is_dir() / inode_is_reg() / inode_is_lnk() / inode_is_chr()
 *  / inode_is_blk() and inode_make_mode(), all in inode.h.
 *
 *  @version 3.1.0  @date 2026-09-26
 */
#ifndef UIOX_FS_TYPES_H
#define UIOX_FS_TYPES_H

#include "uiox_klibc.h"     /* uint*_t, bool, size_t, NULL */
#include "unfs_format.h"    /* the on-disk format — UNFS owns it */

/* ═════════════════════════════════════════════════════════════════════
 * The inline extent count — and WHY IT IS SPELLED HERE
 *
 * unfs_format.h declares the field as a LITERAL — the four inline
 * extents appear as a bare 4 in the array — and no macro names it.
 * inode.h mirrors the field with
 * UNFS_INLINE_EXTENTS, and cannot see a macro that does not exist:
 *
 *     inode.h:77:  error: 'UNFS_INLINE_EXTENTS' undeclared here
 *     inode.h:92:  static assertion failed: "DiskInode must be exactly
 *                  UNFS_INODE_SIZE (256) bytes"
 *     unfs_format.h:265: error: size of array 'unfs_inode_size_assert'
 *                  is negative
 *
 * — all three of those are this one missing name, because a struct with
 * an undeclared array size is not a struct with a size.
 *
 * It is DEFINED HERE rather than in namei.h, where an earlier revision
 * put it: namei.h is not on inode.h's include path, so its guarded
 * definition never ran for the files that needed it.  fs_types.h sits
 * between inode.h and unfs_format.h, so it is the one place both can see.
 * ═════════════════════════════════════════════════════════════════════ */
#ifndef UNFS_INLINE_EXTENTS
#define UNFS_INLINE_EXTENTS 4u
#endif

/* ═════════════════════════════════════════════════════════════════════
 * Alignment of a variable-length dirent record
 *
 * unfs_format.h does not name this either, and namei.h's helpers need
 * it.  Same reasoning as above: one definition, reachable from both the
 * header that uses the struct and the code that walks it.
 * ═════════════════════════════════════════════════════════════════════ */
#ifndef UNFS_DIRENT_ALIGN
#define UNFS_DIRENT_ALIGN 4u
#endif

/* ═════════════════════════════════════════════════════════════════════
 * In-core inode cache
 *
 * 01_fsa's only allocation.  A bootloader has no cache, so this is not
 * format and does not belong in unfs_format.h.
 * ═════════════════════════════════════════════════════════════════════ */
#define MAX_INCACHE         32u           /* inode.h: icache[] entries    */

/* ═════════════════════════════════════════════════════════════════════
 * Inode dirty flags — Bach Ch.4 §1.2
 *
 *   ACCESSED  a read happened
 *   CHANGED   the inode's METADATA moved
 *   MODIFIED  the file's DATA moved
 *
 * fdatasync() tests MODIFIED to decide whether data must reach disk; a
 * pure timestamp update sets only CHANGED and is skipped.  These are
 * in-core only — nothing writes them to disk.
 * ═════════════════════════════════════════════════════════════════════ */
#define IFLAG_ACCESSED  0x01
#define IFLAG_CHANGED   0x02
#define IFLAG_MODIFIED  0x04

/* ═════════════════════════════════════════════════════════════════════
 * Permission bits
 *
 * Bach's octal spellings, used by fs_mkfs() and by the permission tests.
 * The same values exist in unfs_format.h as UNFS_I*; this file keeps
 * BMI-style names because 01_fsa's sources already use them.
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
 * Super block free lists — Bach Ch.4 §5..§8
 *
 * Sizes of the IN-CORE cached windows, not disk layout.  unfs_sb_t
 * carries the free-block chain and free-inode list on disk; these bound
 * what superblock.c keeps in memory.
 * ═════════════════════════════════════════════════════════════════════ */
#define SB_FREE_INODE_MAX   32
#define SB_FREE_BLOCK_MAX   50

/* ═════════════════════════════════════════════════════════════════════
 * Path limits
 *
 * MAX_PATH_LEN bounds a user-supplied path string, not a disk field.
 * UNFS_NAME_MAX (255) bounds ONE COMPONENT on disk; a full path is a
 * kernel-side limit and stays here.
 * ═════════════════════════════════════════════════════════════════════ */
#define MAX_PATH_LEN        256u

/* ═════════════════════════════════════════════════════════════════════
 * Deliberately NOT defined here, because unfs_format.h owns them
 *
 *   BLOCK_SIZE / UNFS_BLOCK_SIZE      4096 — the disk's unit, not ours
 *   INODE_START_BLOCK                  UNFS_GROUP0_ITABLE
 *   DATA_START_BLOCK                   UNFS_GROUP0_DATA
 *   NDIRECT / NINDIRECT / …            extents replaced them
 *   PTRS_PER_BLOCK                     extents replaced it
 *   MAX_NAME_LEN                       UNFS_NAME_MAX
 *   MAX_BLOCKS / MAX_INODES            read from unfs_sb_t
 *   FileType                           REMOVED — it COLLIDED with UNFS_IF*
 *
 * If a source still references one of those names, it is a file that has
 * not been converted to the UNFS format yet — and a compile error naming
 * it is the correct outcome, not something to paper over with an alias.
 * ═════════════════════════════════════════════════════════════════════ */

#endif /* UIOX_FS_TYPES_H */
