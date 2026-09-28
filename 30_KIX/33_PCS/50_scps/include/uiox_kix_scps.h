/*
 * 33_PCS/50_scps/include/uiox_kix_scps.h
 *
 * SCPS — System Call Process System.  Process-side syscall BOUNDARY.
 * Above: 10Arch's arch_sys_call_dispatch().  Below: 40_procStruct.
 *
 * ── scope, and what is deliberately NOT here ─────────────────────────
 * read, write, open, close  -> 32_FS/10_scfs (already implemented there)
 * mmap, munmap              -> no SCPS algorithm; mm.h declares bodies
 *                              that do not exist
 * Files, directories, select/poll, dup/fcntl/pipe, chdir/mkdir/unlink,
 * stat/fstat, credentials beyond getuid-family — none of these are
 * process algorithms.  They belong to the FS layer.
 *
 * ── Bach's three groups, from SCPS: System call Process System ───────
 *   1. Memory Management : fork (dupreg, attachreg)
 *                          exec (detach/alloc/attach/grow/load/mapreg)
 *                          brk  (growreg)      exit (detachreg)
 *   2. Synchronization   : wait, signal, kill
 *   3. Miscellaneous     : setpgrp, setuid
 *
 * ── extension beyond Bach's nine ─────────────────────────────────────
 * The POSIX calls the layer can already support, and only those:
 *   getpid/getppid  getuid/euid  getgid/egid  getpgrp  nice
 *   times  sigprocmask  sigpending  raise  pause  alarm
 *
 * Every one is satisfiable from fields that EXIST on proc_t today:
 *   p_pid p_ppid p_uid p_euid p_gid p_egid p_sig p_sigmask
 *   p_sched.p_nice p_timers
 * Calls needing a field that does not exist are absent, not stubbed.
 *
 * ── the type this binds to ───────────────────────────────────────────
 * proc_t (40_procStruct/include/process.h).  NOT uiox_task_t, NOT the
 * pe_* struct that 02_MemMngnt/src/page_fault.c uses, NOT sim_proc_t
 * that 40_procStruct/src/signal.c casts to.  Four process models exist
 * in this tree; this layer binds to proc_t and says so.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#ifndef UIOX_KIX_SCPS_H
#define UIOX_KIX_SCPS_H

#include "uiox_klibc.h"
#include "process.h"       /* proc_t, PROC_*, PZERO, NICE_* */

/* ── return convention: negative = error ──────────────────────────── */
#define SCPS_EOK       ((int64_t)  0)
#define SCPS_EPERM     ((int64_t) -1)
#define SCPS_ENOENT    ((int64_t) -2)
#define SCPS_ESRCH     ((int64_t) -3)
#define SCPS_EINTR     ((int64_t) -4)
#define SCPS_EIO       ((int64_t) -5)
#define SCPS_ECHILD    ((int64_t)-10)
#define SCPS_EAGAIN    ((int64_t)-11)
#define SCPS_ENOMEM    ((int64_t)-12)
#define SCPS_EFAULT    ((int64_t)-14)
#define SCPS_EBUSY     ((int64_t)-16)
#define SCPS_EINVAL    ((int64_t)-22)
#define SCPS_ENOSYS    ((int64_t)-38)

/* ══ Group 1 — Memory Management ══════════════════════════════════ */
int64_t uiox_kix_scps_fork    (void);
int64_t uiox_kix_scps_execve  (uiox_uintptr_t path, uiox_uintptr_t argv,
                               uiox_uintptr_t envp);
int64_t uiox_kix_scps_brk     (uiox_uint64_t new_brk);
int64_t uiox_kix_scps_exit    (uiox_uint64_t exit_code);

/* ══ Group 2 — Synchronization ════════════════════════════════════ */
int64_t uiox_kix_scps_wait_pid     (uiox_uint64_t pid, uiox_uintptr_t status,
                                    uiox_uint64_t options);
int64_t uiox_kix_scps_kill         (uiox_uint64_t pid, uiox_uint64_t sig);
int64_t uiox_kix_scps_sig_action   (uiox_uint64_t sig, uiox_uintptr_t act,
                                    uiox_uintptr_t oldact);
int64_t uiox_kix_scps_sig_procmask (uiox_uint64_t how, uiox_uintptr_t set,
                                    uiox_uintptr_t oldset);
int64_t uiox_kix_scps_sig_pending  (uiox_uintptr_t set);
int64_t uiox_kix_scps_raise        (uiox_uint64_t sig);

/* ══ Group 3 — Miscellaneous + POSIX identities ═══════════════════ */
int64_t uiox_kix_scps_get_pid   (void);
int64_t uiox_kix_scps_get_ppid  (void);
int64_t uiox_kix_scps_get_uid   (void);
int64_t uiox_kix_scps_get_euid  (void);
int64_t uiox_kix_scps_get_gid   (void);
int64_t uiox_kix_scps_get_egid  (void);
int64_t uiox_kix_scps_get_pgrp  (void);
int64_t uiox_kix_scps_set_uid   (uiox_uint64_t uid);
int64_t uiox_kix_scps_nice      (uiox_uint64_t inc);
int64_t uiox_kix_scps_times     (uiox_uintptr_t buf_out);

/* ══ Time and waiting ═════════════════════════════════════════════ */
int64_t uiox_kix_scps_nano_sleep     (uiox_uintptr_t req, uiox_uintptr_t rem);
int64_t uiox_kix_scps_clock_get_time (uiox_uint64_t clock_id,
                                      uiox_uintptr_t out);
int64_t uiox_kix_scps_pause          (void);
int64_t uiox_kix_scps_alarm          (uiox_uint64_t seconds);

/* ── the ONE place a process pointer is obtained ─────────────────────
 * Four process models coexist in this tree and two of them spell the
 * running-process pointer differently (p_* vs pe_*).  Every entry point
 * goes through this accessor so the spelling question has exactly one
 * answer to correct, not twenty-two. */
proc_t *uiox_kix_scps_current(void);

/* ── user-pointer bridge ────────────────────────────────────────────
 * There is no copy_from_user / copy_to_user in the tree.  Until there
 * is, these two are the ONLY sanctioned ways a wrapper touches a user
 * address: check_user_ptr() validates range and alignment and returns
 * 0 on success; user_read_u32()/user_write_u32() would go here when a
 * real user/kernel split lands.  Today check_user_ptr() is the gate
 * that lets a wrapper return EFAULT honestly instead of dereferencing. */
int uiox_kix_scps_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                                 uiox_uint64_t align);

#endif /* UIOX_KIX_SCPS_H */
