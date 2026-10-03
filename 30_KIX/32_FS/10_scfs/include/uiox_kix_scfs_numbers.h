/* ═══════════════════════════════════════════════════════════════════
 *  10_scfs/include/uiox_kix_scfs_numbers.h
 *
 *  The BSD syscall numbers THIS LAYER ROUTES, so the dispatch table in
 *  uiox_kix_scfs_dispatch.c can index by them without including the
 *  syscall layer's header.
 *
 *  ── why they live here and not in 40_SCIX ─────────────────────────
 *  The dispatcher's own header sets out the split: SCiX decides which
 *  SUBSYSTEM owns a number, and this layer decides which FUNCTION
 *  within it.  Copying the numbers down keeps that split while removing
 *  the upward include — 10_scfs is a filesystem layer and 40_SCIX is
 *  the syscall boundary ABOVE it, so including uix_sys.h from here
 *  inverted the direction of the dependency.
 *
 *  ── which values, and why it matters ──────────────────────────────
 *  40_SCIX/uix_sys.h defines many of these names TWICE, with different
 *  values, and in C the later #define wins silently:
 *
 *      SYS_LSTAT     40 (upper)  vs  107 (lower)
 *      SYS_FSTAT     53          vs  108
 *      SYS_READV    120          vs   19
 *      SYS_TRUNCATE 167          vs   92
 *      ...and ~35 more
 *
 *  The lower block also collides with itself — FCHMODAT and FACCESSAT
 *  both land on 307, PIPE2 and SHUTDOWN both on 293 — so at most one of
 *  the two blocks can be right.
 *
 *  The UPPER block is the one copied here.  It is the coherent BSD
 *  table (numeric order, gaps preserved), and the userspace side agrees
 *  with it: 50_UIX/00_libs/00_uixlibs/uix_sysh.beckup has LSTAT 40,
 *  READV 120 and TRUNCATE 167 — the upper values, none of the lower.
 *
 *  The lower block is NOT copied.  If it is ever shown to be the real
 *  ABI, this file is wrong and must be regenerated from it.
 *
 *  ── a wrong number does not fail to compile ───────────────────────
 *  It routes a syscall to another handler, silently.  Every define
 *  below is #ifndef-guarded so that a legitimately-in-scope uix_sys.h
 *  wins the test and these fall away.
 * ═══════════════════════════════════════════════════════════════════ */

 #ifndef UIOX_KIX_SCFS_NUMBERS_H
 #define UIOX_KIX_SCFS_NUMBERS_H
 
 /* ── table (data) ─────────────────────────────────────────────────── */
 #ifndef SYS_READ
 #define SYS_READ              3
 #endif
 #ifndef SYS_WRITE
 #define SYS_WRITE             4
 #endif
 #ifndef SYS_LSEEK
 #define SYS_LSEEK           166
 #endif
 #ifndef SYS_DUP
 #define SYS_DUP              41
 #endif
 #ifndef SYS_DUP2
 #define SYS_DUP2             90
 #endif
 #ifndef SYS_DUP3
 #define SYS_DUP3            102
 #endif
 #ifndef SYS_FCNTL
 #define SYS_FCNTL            92
 #endif
 #ifndef SYS_IOCTL
 #define SYS_IOCTL            54
 #endif
 #ifndef SYS_FSYNC
 #define SYS_FSYNC            95
 #endif
 #ifndef SYS_SYNC
 #define SYS_SYNC             36
 #endif
 
 /* ── table (metadata) ─────────────────────────────────────────────── */
 #ifndef SYS_STAT
 #define SYS_STAT             38
 #endif
 #ifndef SYS_FSTAT
 #define SYS_FSTAT            53
 #endif
 #ifndef SYS_LSTAT
 #define SYS_LSTAT            40
 #endif
 #ifndef SYS_FSTATAT
 #define SYS_FSTATAT          42
 #endif
 #ifndef SYS_CHMOD
 #define SYS_CHMOD            15
 #endif
 #ifndef SYS_FCHMOD
 #define SYS_FCHMOD          124
 #endif
 #ifndef SYS_FCHMODAT
 #define SYS_FCHMODAT        314
 #endif
 #ifndef SYS_CHOWN
 #define SYS_CHOWN            16
 #endif
 #ifndef SYS_FCHOWN
 #define SYS_FCHOWN          123
 #endif
 #ifndef SYS_FCHOWNAT
 #define SYS_FCHOWNAT        315
 #endif
 #ifndef SYS_TRUNCATE
 #define SYS_TRUNCATE        167
 #endif
 #ifndef SYS_FTRUNCATE
 #define SYS_FTRUNCATE       168
 #endif
 #ifndef SYS_UMASK
 #define SYS_UMASK            60
 #endif
 #ifndef SYS_FACCESSAT
 #define SYS_FACCESSAT       313
 #endif
 #ifndef SYS_ACCESS
 #define SYS_ACCESS           33
 #endif
 
 /* ── table (namespace) ────────────────────────────────────────────── */
 #ifndef SYS_OPEN
 #define SYS_OPEN              5
 #endif
 #ifndef SYS_OPENAT
 #define SYS_OPENAT          321
 #endif
 #ifndef SYS_CLOSE
 #define SYS_CLOSE             6
 #endif
 #ifndef SYS_MKDIR
 #define SYS_MKDIR           136
 #endif
 #ifndef SYS_MKDIRAT
 #define SYS_MKDIRAT         318
 #endif
 #ifndef SYS_RMDIR
 #define SYS_RMDIR           137
 #endif
 #ifndef SYS_CHDIR
 #define SYS_CHDIR            12
 #endif
 #ifndef SYS_FCHDIR
 #define SYS_FCHDIR           13
 #endif
 #ifndef SYS_LINK
 #define SYS_LINK              9
 #endif
 #ifndef SYS_LINKAT
 #define SYS_LINKAT          317
 #endif
 #ifndef SYS_UNLINK
 #define SYS_UNLINK           10
 #endif
 #ifndef SYS_UNLINKAT
 #define SYS_UNLINKAT        325
 #endif
 #ifndef SYS_SYMLINK
 #define SYS_SYMLINK          57
 #endif
 #ifndef SYS_SYMLINKAT
 #define SYS_SYMLINKAT       324
 #endif
 #ifndef SYS_READLINK
 #define SYS_READLINK         58
 #endif
 #ifndef SYS_READLINKAT
 #define SYS_READLINKAT      322
 #endif
 #ifndef SYS_RENAME
 #define SYS_RENAME          128
 #endif
 #ifndef SYS_RENAMEAT
 #define SYS_RENAMEAT        323
 #endif
 #ifndef SYS_MKNOD
 #define SYS_MKNOD            14
 #endif
 #ifndef SYS_MKNODAT
 #define SYS_MKNODAT         320
 #endif
 #ifndef SYS_MKFIFO
 #define SYS_MKFIFO          132
 #endif
 #ifndef SYS_CHROOT
 #define SYS_CHROOT           61
 #endif
 #ifndef SYS_GETDENTS
 #define SYS_GETDENTS         99
 #endif
 #ifndef SYS_GETCWD
 #define SYS_GETCWD          304
 #endif
 #ifndef SYS_REALPATH
 #define SYS_REALPATH        115
 #endif
 
 /* ── table (names present, but owned elsewhere or not yet implemented) ─
  * These rows exist in the dispatcher so a caller gets ENOSYS rather
  * than a fall-through.  The numbers are the upper block's, so the
  * table's index arithmetic stays correct. */
 #ifndef SYS_MOUNT
 #define SYS_MOUNT            21
 #endif
 #ifndef SYS_UNMOUNT
 #define SYS_UNMOUNT          22
 #endif
 #ifndef SYS_STATFS
 #define SYS_STATFS           63
 #endif
 #ifndef SYS_FSTATFS
 #define SYS_FSTATFS          64
 #endif
 #ifndef SYS_CHFLAGS
 #define SYS_CHFLAGS          34
 #endif
 #ifndef SYS_FCHFLAGS
 #define SYS_FCHFLAGS         35
 #endif
 #ifndef SYS_PATHCONF
 #define SYS_PATHCONF        191
 #endif
 #ifndef SYS_FLOCK
 #define SYS_FLOCK           131
 #endif
 #ifndef SYS_SELECT
 #define SYS_SELECT           71
 #endif
 #ifndef SYS_POLL
 #define SYS_POLL            252
 #endif
 #ifndef SYS_PIPE
 #define SYS_PIPE            263
 #endif
 #ifndef SYS_PIPE2
 #define SYS_PIPE2           101
 #endif
 #ifndef SYS_KQUEUE
 #define SYS_KQUEUE          269
 #endif
 #ifndef SYS_MLOCK
 #define SYS_MLOCK           203
 #endif
 #ifndef SYS_MUNLOCK
 #define SYS_MUNLOCK         204
 #endif
 #ifndef SYS_MSYNC
 #define SYS_MSYNC           256
 #endif
 #ifndef SYS_MMAP
 #define SYS_MMAP             49
 #endif
 #ifndef SYS_MUNMAP
 #define SYS_MUNMAP           73
 #endif
 #ifndef SYS_MPROTECT
 #define SYS_MPROTECT         74
 #endif
 #ifndef SYS_GETENTROPY
 #define SYS_GETENTROPY        7
 #endif
 #ifndef SYS_PREAD
 #define SYS_PREAD           169
 #endif
 #ifndef SYS_PWRITE
 #define SYS_PWRITE          170
 #endif
 #ifndef SYS_READV
 #define SYS_READV           120
 #endif
 #ifndef SYS_WRITEV
 #define SYS_WRITEV          121
 #endif
 
 #endif /* UIOX_KIX_SCFS_NUMBERS_H */
 