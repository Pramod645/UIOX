/*
 * 30_KIX/32_FS/include/uiox_scfs.h
 *
 * UIOX — SCFS syscall surface (System Call File System layer).
 *
 * SCFS is the syscall-facing filesystem layer.  It owns NO storage logic:
 * every entry validates a user pointer, resolves an fd or a path, then
 * dispatches into the VFS abstraction (01_fsa) via uiox_file_ops_t /
 * uiox_inode_ops_t.  Backends (UNFS, ramfs, netfs) sit below.
 *
 *   syscall table (this file)
 *        -> SCFS  (validate + marshal)          [10_scfs]
 *        -> FSA/VFS (inode, dentry, namei, bmap) [01_fsa]
 *        -> UNFS (extents, COW, xattr, journal)  [10_unfs]
 *        -> block device                          [drivers]
 *
 * @version 1.0.0  @date 2026-09-20
 */
#ifndef UIOX_SCFS_H
#define UIOX_SCFS_H

#include "uiox_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * Dispatch target — which VFS operation object an entry calls.
 * ===================================================================== */
typedef enum {
    SCFS_DISPATCH_FILE   = 0,  /* uiox_file_ops_t                     */
    SCFS_DISPATCH_INODE  = 1,  /* uiox_inode_ops_t                    */
    SCFS_DISPATCH_VFS    = 2,  /* VFS-global (mount, namei, fd table)  */
    SCFS_DISPATCH_PCS    = 3,  /* process state (cwd) — 33_PCS owns   */
    SCFS_DISPATCH_ENOSYS = 4   /* surface present, backend not ready   */
} scfs_dispatch_t;

/* Availability of each entry against the current build. */
typedef enum {
    SCFS_READY    = 0,  /* wired to a live backend operation         */
    SCFS_PARTIAL  = 1,  /* syscall exists, backend returns ENOSYS    */
    SCFS_PENDING  = 2   /* waits on UNFS registration / page cache   */
} scfs_state_t;

/* =====================================================================
 * One syscall-table entry.
 *
 *   nr        — syscall number (arch syscall ABI; see note below)
 *   name      — POSIX name for diagnostics / the syscall-name table
 *   dispatch  — which VFS object the call resolves to
 *   state     — readiness
 *   handler   — the entry point.  Signature is the generic syscall form:
 *                 long (*)(uiox_reg_t a0, a1, a2, a3, a4, a5)
 *               so the arch syscall stubs (arch_syscall_entry) can jump
 *               through this table without per-arch wrappers.
 * ===================================================================== */
typedef long (*scfs_handler_t)(uiox_reg_t a0, uiox_reg_t a1, uiox_reg_t a2,
                               uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5);

typedef struct {
    uint32_t          nr;
    const char       *name;
    scfs_dispatch_t   dispatch;
    scfs_state_t      state;
    scfs_handler_t    handler;
} scfs_syscall_t;

/* =====================================================================
 * Syscall numbers.
 *
 * SCFS numbers follow the Linux generic (asm-generic) syscall list so the
 * table matches what userland and any future strace-ish tooling expect.
 * The arch stub adds no offset — x86-64 numbers already match this list.
 * Rebase here only if a target diverges.
 * these are already available from book analysis and for created definations as for below:
 * chdir
 * close
 * creat
 * file
 * inode
 * link
 * mkdir
 * mknod
 * mount
 * open
 * pipe
 * read
 * write
 * ===================================================================== */
//POSIX are these 
#define SCFS_NR_open      2 //uiox_kix_scfs_open.c
#define SCFS_NR_close     3
#define SCFS_NR_read      0 //uiox_kix_scfs_read.c
#define SCFS_NR_write     1 //uiox_kix_scfs_write.c
#define SCFS_NR_lseek     8 //uiox_kix_scfs_read.c
#define SCFS_NR_pread     17
#define SCFS_NR_pwrite    18
#define SCFS_NR_readv     19
#define SCFS_NR_writev    20
#define SCFS_NR_stat      4 //uiox_kix_scfs_chdir.c
#define SCFS_NR_lstat     6
#define SCFS_NR_fstat     5 //uiox_kix_scfs_chdir.c
#define SCFS_NR_newfstatat 262
#define SCFS_NR_chmod     90
#define SCFS_NR_fchmod    91 //uiox_kix_scfs_chdir.c
#define SCFS_NR_chown     92 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fchown    93
#define SCFS_NR_truncate  76
#define SCFS_NR_ftruncate 77
#define SCFS_NR_access    21
#define SCFS_NR_umask     95
#define SCFS_NR_mkdir     83 //uiox_kix_scfs_mkdir.c
#define SCFS_NR_rmdir     84 //uiox_kix_scfs_mkdir.c
#define SCFS_NR_chdir     80 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fchdir    81 
#define SCFS_NR_getcwd    79
#define SCFS_NR_getdents64 217
#define SCFS_NR_link      86  // uiox_kix_scfs_link.c
#define SCFS_NR_unlink    87 // uiox_kix_scfs_link.c
#define SCFS_NR_rename    82
#define SCFS_NR_symlink   88
#define SCFS_NR_readlink  89
#define SCFS_NR_fsync     74
#define SCFS_NR_fdatasync 75
#define SCFS_NR_sync      162
#define SCFS_NR_mount     165 //uiox_kix_scfs_mount.c
#define SCFS_NR_umount2   166
#define SCFS_NR_statfs    137
#define SCFS_NR_fstatfs   138
#define SCFS_NR_dup       32
#define SCFS_NR_dup3      292
#define SCFS_NR_fcntl     72
#define SCFS_NR_ioctl     16
#define SCFS_NR_mmap      9
#define SCFS_NR_munmap    11
#define SCFS_NR_msync     26
#define SCFS_NR_statx     332
//posix end ehere
// below new
#define SCFS_NR_chroot    161 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fclose    57 //uiox_kix_scfs_close.c
#define SCFS_NR_dirname   158 // uiox_kix_scfs_creat.c
#define SCFS_NR_creat     85 // uiox_kix_scfs_creat.c
#define SCFS_NR_falloc    59 // uiox_kix_scfs_file.c
#define SCFS_NR_fclose     60 // uiox_kix_scfs_file.c
#define SCFS_NR_ufalloc    61 // uiox_kix_scfs_file.c
#define SCFS_NR_dirname     62 // uiox_kix_scfs_link.c, uiox_kix_scfs_mkdir.c,uiox_kix_scfs_mknod.c
#define SCFS_NR_mknod     133 // uiox_kix_scfs_mknod.c
#define SCFS_NR_umount     134 // uiox_kix_scfs_mount.c
#define SCFS_NR_pipe      22 // uiox_kix_scfs_pipe.c






/* =====================================================================
 * Public entry points implemented by 10_scfs/scfs.c
 * ===================================================================== */

/* Install the table with the arch syscall dispatcher.  Call once, before
 * the first syscall is dispatched (from uiox_fs_init()). */
void scfs_register(void);

/* Look-up + dispatch.  The arch trap handler calls this with the raw
 * register arguments; returns the value placed in the return register. */
long scfs_dispatch(uint32_t nr,
                   uiox_reg_t a0, uiox_reg_t a1, uiox_reg_t a2,
                   uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5);

/* The table itself — exposed for init and for a future /proc-style dump. */
const scfs_syscall_t *scfs_table(void);
uint32_t              scfs_table_size(void);

/* Filesystem registration (called by uiox_fs_init(), not by SCFS). */
void scfs_init(void);   /* kept: registers SCFS's fs_type with the VFS */
void unfs_register(void); /* UNFS backend — REQUIRED before vfs_mount_root */

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SCFS_H */
