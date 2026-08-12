/*
 * 30_KIX/33_PCS/include/uiox_syscall.h
 *
 * UIOX Syscall dispatch table and handler declarations.
 * Called from arch_syscall_entry() in 10_Arch/<arch>/src/arch_init.c
 *
 * Syscall numbers match 40_SystemCallInterface definitions.
 *
 * @version 1.0.0
 * @date    2026-07-28
 */

 #ifndef UIOX_SYSCALL_H
 #define UIOX_SYSCALL_H
 
 #include "uiox_soc.h"
 
 /* ── Syscall numbers ───────────────────────────────────────────────── */
 #define SYS_READ            0
 #define SYS_WRITE           1
 #define SYS_OPEN            2
 #define SYS_CLOSE           3
 #define SYS_MMAP            9
 #define SYS_MUNMAP         11
 #define SYS_IOCTL          16
 #define SYS_GETPID         39
 #define SYS_EXIT           60
 #define SYS_SYNC          162
 #define SYS_FSYNC          74
 #define SYS_FDATASYNC      75
 #define SYS_SYNCFS        306
 /* Security (33_PCS/05_sec) */
 #define SYS_GETLABEL      250
 #define SYS_SETLABEL      251
 #define SYS_GETPOLICY     252
 #define SYS_SETPOLICY     253
 #define SYS_ASLR_STATUS   254
 /* Live patch (33_PCS/06_kpatch) */
 #define SYS_KPATCH_APPLY  260
 #define SYS_KPATCH_REVERT 261
 #define SYS_KPATCH_STATUS 262
 
 #define UIOX_NR_SYSCALLS  320
 
 /* ── Syscall return type ───────────────────────────────────────────── */
 typedef long uiox_syscall_ret_t;
 #define UIOX_SYSCALL_ERR(e)  ((uiox_syscall_ret_t)(-(e)))
 
 /* ── Syscall argument frame ────────────────────────────────────────── */
 /* Arch-independent view — arch entry fills this before dispatching */
 typedef struct {
     unsigned long nr;    /* syscall number                        */
     unsigned long a0;    /* arg 0  (x0/r0/a0/rdi)                */
     unsigned long a1;    /* arg 1  (x1/r1/a1/rsi)                */
     unsigned long a2;    /* arg 2  (x2/r2/a2/rdx)                */
     unsigned long a3;    /* arg 3  (x3/r3/a3/r10)                */
     unsigned long a4;    /* arg 4  (x4/r4/a4/r8)                 */
     unsigned long a5;    /* arg 5  (x5/r5/a5/r9)                 */
 } uiox_syscall_frame_t;
 
 /*
  * uiox_syscall_dispatch — main entry point called by arch layer.
  * Returns value placed in user return register (x0/r0/a0/rax).
  */
 uiox_syscall_ret_t uiox_syscall_dispatch(const uiox_syscall_frame_t *f);
 
 /* ── Individual handler declarations ──────────────────────────────── */
 uiox_syscall_ret_t sys_read    (int fd, void *ubuf, size_t count);
 uiox_syscall_ret_t sys_write   (int fd, const void *ubuf, size_t count);
 uiox_syscall_ret_t sys_open    (const char *upath, int flags, int mode);
 uiox_syscall_ret_t sys_close   (int fd);
 uiox_syscall_ret_t sys_ioctl   (int fd, unsigned long cmd, unsigned long arg);
 uiox_syscall_ret_t sys_mmap    (void *addr, size_t len, int prot,
                                  int flags, int fd, long off);
 uiox_syscall_ret_t sys_munmap  (void *addr, size_t len);
 uiox_syscall_ret_t sys_getpid  (void);
 uiox_syscall_ret_t sys_exit    (int code);
 uiox_syscall_ret_t sys_sync    (void);
 uiox_syscall_ret_t sys_fsync   (int fd);
 
 #endif /* UIOX_SYSCALL_H */
 