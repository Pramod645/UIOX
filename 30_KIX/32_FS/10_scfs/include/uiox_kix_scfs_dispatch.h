/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_dispatch.h
 *
 * SCFS — the dispatch header.
 * Bach, The Design of the UNIX Operating System, Ch.5.
 *
 * ── the contract with the arch code ────────────────────────────────────
 * The trap handler in arch_init.c knows a system call's NUMBER and nothing
 * else.  It fills a carrier with the argument registers, calls
 * scfs_dispatch(), and returns what it gets.  Adding a syscall is one row
 * in the table and one number here — never a change to four arch files.
 *
 * ── the numbers are the ABI ────────────────────────────────────────────
 * A user-space stub and this table must agree on every number, so they
 * live here once and both sides include this file.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#ifndef UIOX_KIX_SCFS_DISPATCH_H
#define UIOX_KIX_SCFS_DISPATCH_H

#include "uiox_kix_scfs.h"

/* ═════════════════════════════════════════════════════════════════════
 * The syscall numbers
 *
 * A private fixed range from 1000, so the table is unambiguous on every
 * target and no architecture's native numbering leaks in.  Gaps are left
 * deliberately — 1100 for the *at() family, 1200 for the metadata calls —
 * so a new call joins its group rather than the end of a list nobody can
 * read.
 * ═════════════════════════════════════════════════════════════════════ */
/* ── descriptors ───────────────────────────────────────────────────── */
#define SCFS_SYS_open      1000u
#define SCFS_SYS_close     1001u
#define SCFS_SYS_read      1002u
#define SCFS_SYS_write     1003u
#define SCFS_SYS_lseek     1004u
#define SCFS_SYS_creat     1005u
#define SCFS_SYS_dup       1006u
#define SCFS_SYS_dup2      1007u
#define SCFS_SYS_fcntl     1008u
#define SCFS_SYS_ioctl     1009u
#define SCFS_SYS_pipe      1010u
#define SCFS_SYS_mmap      1011u

/* ── names and directories ─────────────────────────────────────────── */
#define SCFS_SYS_link      1050u
#define SCFS_SYS_unlink    1051u
#define SCFS_SYS_rename    1052u
#define SCFS_SYS_mkdir     1053u
#define SCFS_SYS_rmdir     1054u
#define SCFS_SYS_mknod     1055u
#define SCFS_SYS_chdir     1056u
#define SCFS_SYS_fchdir    1057u
#define SCFS_SYS_chroot    1058u
#define SCFS_SYS_getcwd    1059u
#define SCFS_SYS_getdents64 1060u

/* ── the *at() family ──────────────────────────────────────────────── */
#define SCFS_SYS_openat    1100u
#define SCFS_SYS_unlinkat  1101u
#define SCFS_SYS_mkdirat   1102u
#define SCFS_SYS_faccessat 1103u
#define SCFS_SYS_renameat  1104u
#define SCFS_SYS_linkat    1105u

/* ── status, metadata, durability ──────────────────────────────────── */
#define SCFS_SYS_stat      1200u
#define SCFS_SYS_fstat     1201u
#define SCFS_SYS_lstat     1202u
#define SCFS_SYS_chmod     1203u
#define SCFS_SYS_chown     1204u
#define SCFS_SYS_access    1205u
#define SCFS_SYS_umask     1206u
#define SCFS_SYS_truncate  1207u
#define SCFS_SYS_ftruncate 1208u
#define SCFS_SYS_utime     1209u
#define SCFS_SYS_sync      1210u
#define SCFS_SYS_fsync     1211u
#define SCFS_SYS_fdatasync 1212u
#define SCFS_SYS_statfs    1213u
#define SCFS_SYS_mount     1214u
#define SCFS_SYS_umount    1215u
#define SCFS_SYS_setxattr  1216u
#define SCFS_SYS_getxattr  1217u
#define SCFS_SYS_listxattr 1218u
#define SCFS_SYS_removexattr 1219u

/* ═════════════════════════════════════════════════════════════════════
 * The argument carrier
 *
 * Six uintptr_t, one per argument register.  The arch stub fills them in
 * register order; a table row reads the ones its call takes.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_args {
    uintptr_t a0, a1, a2, a3, a4, a5;
} scfs_args_t;

/* ═════════════════════════════════════════════════════════════════════
 * A row in Bach's sysent[]
 * ═════════════════════════════════════════════════════════════════════ */
typedef intptr_t (*scfs_fn_t)(const scfs_args_t *);

typedef struct scfs_sysent {
    uint32_t        number;   /* the syscall number                        */
    scfs_fn_t       fn;       /* the implementation                        */
    uint32_t        nargs;    /* arguments it reads, for the count check   */
    const char     *name;     /* Bach: "used for maintenance"              */
} scfs_sysent_t;

/* A declared gap — a number that exists but has no working implementation
 * on this architecture. */
typedef struct scfs_gap {
    const char *name;
    const char *reason;
} scfs_gap_t;

/* ═════════════════════════════════════════════════════════════════════
 * The surface
 * ═════════════════════════════════════════════════════════════════════ */
extern const scfs_sysent_t  scfs_sysent_table[];
extern const uint32_t       scfs_sysent_count;
extern const scfs_gap_t     scfs_gap_table[];

/* The one entry point the arch trap handler calls. */
intptr_t scfs_dispatch(uint32_t num, const scfs_args_t *args);

/* Number to row, for a debug command or a trap that wants the name. */
const scfs_sysent_t *scfs_sysent_lookup(uint32_t num);

#endif /* UIOX_KIX_SCFS_DISPATCH_H */
