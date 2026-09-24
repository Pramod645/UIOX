/*
 *  30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_stat.h
 *
 *  SCFS — the status-structure layout and its size.
 *
 *  ── why this file exists ─────────────────────────────────────────────
 *  Both stat.c and statfs.c fill a caller-supplied buffer at fixed byte
 *  offsets.  Neither had a declared size, and neither checked the buffer
 *  it was handed — so a caller with a structure smaller than the one the
 *  writer assumed would have had 42 (stat) or 64 (statfs) bytes written
 *  past the end of its object.
 *
 *  The offsets were already fixed and correct; what was missing is the
 *  SIZE, so the writer can refuse a buffer it cannot fill safely.  It
 *  lives here, in one place, so the two writers and every caller read the
 *  same number.
 *
 *  ── how to use it ───────────────────────────────────────────────────
 *  A caller declaring its own structure must make it at least as large as
 *  SCFS_STAT_SZ / SCFS_STATFS_SZ:
 *
 *      #include "uiox_kix_scfs_stat.h"
 *      struct my_stat  st;                       // 42 bytes minimum
 *      _Static_assert(sizeof st >= SCFS_STAT_SZ, "status struct too small");
 *
 *      struct my_statfs sf;                      // 64 bytes minimum
 *      _Static_assert(sizeof sf >= SCFS_STATFS_SZ, "statfs struct too small");
 *
 *  And the syscall takes a `uint32_t bufsz` alongside the pointer, so a
 *  short buffer is EINVAL rather than a silent overwrite.  See the note
 *  on the call signatures at the bottom.
 *
 *  ── Bach's fields, and what each offset holds ────────────────────────
 *  Bach lists what a status call returns: file type, owner, access
 *  permissions, file size, number of links, inode number, access times.
 *  The layout below covers all of them plus the device number, which
 *  this filesystem stores nowhere and reports as zero.
 *
 *  v1.0: first cut — the sizes that were implicit are now declared.
 */
#ifndef UIOX_KIX_SCFS_STAT_H
#define UIOX_KIX_SCFS_STAT_H

#include <stdint.h>

#include "fs_types.h"     /* MAX_NAME_LEN */
#include "namei.h"        /* DirEntry     */

/* ═════════════════════════════════════════════════════════════════════
 * The status structure — 42 bytes
 *
 * Laid out so every field lands on its natural alignment without
 * padding: the two int64_t pairs after the 16-byte header are already
 * 8-aligned, and nothing needs a gap.
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
 * Eight int64_t, the conventional statfs shape:
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
 * The signatures that carry the size
 *
 * The syscall bodies take the buffer's size as well as its address, so a
 * short buffer is refused instead of overwritten.  These are the forms
 * stat.c and statfs.c implement; the older two-argument prototypes are
 * dropped so nothing can call the unchecked version by accident.
 *
 * A caller that genuinely has a fixed-size local structure passes
 * sizeof of it — the constants above are what it must be at least.
 * ═════════════════════════════════════════════════════════════════════ */
int uiox_kix_scfs_stat  (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstat (int fd, void *buf, uint32_t bufsz);
int uiox_kix_scfs_lstat (const char *path, void *buf, uint32_t bufsz);

int uiox_kix_scfs_statfs (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstatfs(int fd, void *buf, uint32_t bufsz);

/* ── the sys_* forms, so the arch stub matches ──────────────────────── */
int sys_stat  (const char *path, void *buf, uint32_t bufsz);
int sys_fstat (int fd, void *buf, uint32_t bufsz);
int sys_lstat (const char *path, void *buf, uint32_t bufsz);
int sys_statfs(const char *path, void *buf, uint32_t bufsz);
int sys_fstatfs(int fd, void *buf, uint32_t bufsz);

#endif /* UIOX_KIX_SCFS_STAT_H */
