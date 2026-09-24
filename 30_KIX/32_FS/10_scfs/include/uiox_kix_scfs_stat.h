/*
 *  30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_stat.h
 *
 *  SCFS — the status-structure layout and its size.
 *
 *  ── SYSTEM CALL CONTENT REMOVED ───────────────────────────────────────
 *  This header once declared five sys_*()-shaped entry points —
 *
 *      sys_stat / sys_fstat / sys_lstat / sys_statfs / sys_fstatfs
 *
 *  Those are REMOVED.  A sys_*() name belongs to the layer that presents
 *  the syscall ABI, which is SCIX:
 *
 *      40_SCIX/uix_sys.h            the SYS_* numbers
 *      50_UIX/00_libs/00_uixlibs*.c the sys_*() entry points
 *
 *  What remains here is what is NOT syscall ABI: the byte layout of a
 *  status structure and its size.  That is a kernel-internal contract
 *  between the writer and the caller, and it belongs to whoever writes
 *  the fields — this layer.
 *
 *  ── why the size is declared at all ───────────────────────────────────
 *  stat.c and statfs.c fill a caller-supplied buffer at fixed byte
 *  offsets.  Neither originally knew how large that buffer was, so a
 *  caller with a smaller structure had bytes written past the end of its
 *  object.  The offsets were already fixed and correct; what was missing
 *  is the SIZE, so the writer can refuse a buffer it cannot fill safely.
 *
 *  A caller declaring its own structure must make it at least this large:
 *
 *      #include "uiox_kix_scfs_stat.h"
 *      struct my_stat st;
 *      _Static_assert(sizeof st >= SCFS_STAT_SZ, "status struct too small");
 *
 *  ── the call shape ────────────────────────────────────────────────────
 *  The algorithm bodies take the buffer's size as well as its address:
 *
 *      int uiox_kix_scfs_stat(const char *path, void *buf, uint32_t bufsz);
 *
 *  A buffer smaller than the constant below is refused with SCFS_EINVAL
 *  before a single byte is written.
 *
 *  @version 3.0.0  @date 2026-09-24
 */
#ifndef UIOX_KIX_SCFS_STAT_H
#define UIOX_KIX_SCFS_STAT_H

#include <stdint.h>

#include "fs_types.h"     /* MAX_NAME_LEN */
#include "namei.h"        /* DirEntry     */

/* ═════════════════════════════════════════════════════════════════════
 * The status structure — 42 bytes
 *
 *   off  size  field
 *   ───  ────  ───────────────────────────────────────────────────────
 *     0     2  mode          FileType nibble | permission bits
 *     2     2  nlink
 *     4     2  uid
 *     6     2  gid
 *     8     4  ino
 *    12     4  size
 *    16     8  atime
 *    24     8  mtime
 *    32     8  ctime
 *    40     1  dev_major     always 0 — no device field on the inode
 *    41     1  dev_minor     always 0 — no device field on the inode
 *   ═════════════════════════════════════════════════════════════════ */
#define SCFS_STAT_SZ  42u

#define SCFS_STAT_OFF_MODE    0u
#define SCFS_STAT_OFF_NLINK   2u
#define SCFS_STAT_OFF_UID     4u
#define SCFS_STAT_OFF_GID     6u
#define SCFS_STAT_OFF_INO     8u
#define SCFS_STAT_OFF_SIZE   12u
#define SCFS_STAT_OFF_ATIME  16u
#define SCFS_STAT_OFF_MTIME  24u
#define SCFS_STAT_OFF_CTIME  32u
#define SCFS_STAT_OFF_DEVMAJ 40u
#define SCFS_STAT_OFF_DEVMIN 41u

/* ═════════════════════════════════════════════════════════════════════
 * The filesystem status structure — 64 bytes
 *
 *   off  size  field
 *   ───  ────  ───────────────────────────────────────────────────────
 *     0     8  f_bsize      fundamental block size
 *     8     8  f_blocks     total blocks
 *    16     8  f_bfree      free blocks
 *    24     8  f_bavail     free blocks for a non-super-user
 *    32     8  f_files      total inodes
 *    40     8  f_ffree      free inodes
 *    48     8  f_fsid       always 0 — no device field anywhere
 *    56     8  f_namemax    longest name, MAX_NAME_LEN - 1
 *   ═════════════════════════════════════════════════════════════════ */
#define SCFS_STATFS_SZ  64u

#define SCFS_STATFS_OFF_BSIZE    0u
#define SCFS_STATFS_OFF_BLOCKS   8u
#define SCFS_STATFS_OFF_BFREE   16u
#define SCFS_STATFS_OFF_BAVAIL  24u
#define SCFS_STATFS_OFF_FILES   32u
#define SCFS_STATFS_OFF_FFREE   40u
#define SCFS_STATFS_OFF_FSID    48u
#define SCFS_STATFS_OFF_NAMEMAX 56u

/* ═════════════════════════════════════════════════════════════════════
 * The algorithm bodies that use these layouts
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_kix_scfs_stat  (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstat (int fd, void *buf, uint32_t bufsz);
int uiox_kix_scfs_lstat (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_statfs (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstatfs(int fd, void *buf, uint32_t bufsz);

#endif /* UIOX_KIX_SCFS_STAT_H */
