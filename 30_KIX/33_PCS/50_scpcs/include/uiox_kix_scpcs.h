/*
 * 33_PCS/50_scpcs/include/uiox_kix_scpcs.h
 *
 * SCPCS — System Call Process Control System.
 * The process-side syscall BOUNDARY.
 *
 *   above : 10Arch's arch_sys_call_dispatch()
 *   below : 33_PCS/40psa's algorithms — uiox_kix_psa_*
 *
 * ── naming ──────────────────────────────────────────────────────────
 *   layer    SCPCS        (folder 50_scpcs)
 *   functions uiox_kix_scpcs_<verb>()
 *   macros   SCPCS_<NAME>
 *   guard    UIOX_KIX_SCPCS_H
 *
 * The function prefix tracks the FOLDER, the same way 33_PCS/40psa's
 * files are uiox_kix_psa_*.  One rule, two layers.
 *
 * ── scope, and what is deliberately NOT here ─────────────────────────
 * read, write, open, close  -> 32_FS/10_scfs (implemented there)
 * mmap, munmap              -> no algorithm covers them, and mm.h
 *                              declares bodies that do not exist
 *
 * Files, directories, select/poll, dup/fcntl/pipe, chdir/mkdir/unlink,
 * stat/fstat, and credentials beyond the getuid family are not process
 * algorithms either.  They belong to the FS layer.  Absent, not stubbed.
 *
 * ── the three groups ────────────────────────────────────────────────
 * The grouping is the algorithm text's own, not POSIX's ordering:
 *
 *   1. System Calls dealing with memory Management
 *        fork   -> dupreg, attachreg
 *        exec   -> detachreg, allocreg, attachreg, growreg, loadreg, mapreg
 *        brk    -> growreg
 *        exit   -> detachreg
 *
 *   2. System calls Dealing with Synchronization
 *        wait, signal, kill
 *
 *   3. Miscellaneous
 *        setpgrp, setuid
 *
 * ── extension beyond the nine ───────────────────────────────────────
 * Only the calls 40psa's fields can actually support, then bound:
 *   getpid/getppid  getuid/euid  getgid/egid  nice  times
 *   sigprocmask  sigpending  raise  pause  alarm
 *
 * Each reads a field that exists on uiox_kix_psa_proc_t:
 *   p_pid p_ppid p_uid p_euid p_gid p_egid p_sig p_sigmask
 *   p_sched.p_nice p_timers
 * Calls needing a field that does not exist are ABSENT, not stubbed:
 *   setpgrp  — no group field on the process table entry
 *   setsid   — no session model at all
 *
 * ── the type this binds to ──────────────────────────────────────────
 * uiox_kix_psa_proc_t from 40psa's uiox_kix_psa_process.h.  NOT
 * uiox_task_t, NOT the pe_* shape 02_MemMngnt/src/page_fault.c uses,
 * NOT the sim_proc_t that signal.c casts to.  This is the one model the
 * 40psa algorithms are written against.
 *
 * @version 4.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_SCPCS_H
#define UIOX_KIX_SCPCS_H

#include "uiox_klibc.h"
#include "uiox_kix_psa_process.h"   /* uiox_kix_psa_proc_t, states, PZERO */

/* ── Return convention: int64, negative = error ──────────────────────
 * POSIX's own numbers, repeated here because the kernel builds
 * -nostdinc and there is no <errno.h>.  A caller that receives -38 gets
 * the same meaning a libc would give it.
 *
 * The distinction between these matters: ENOSYS reports a missing
 * implementation, EINVAL blames the caller's argument, and ENOMEM says
 * the kernel tried and ran out.  A caller debugging a failure acts
 * differently on each. */
#define SCPCS_EOK       ((int64_t)  0)   /* success                     */
#define SCPCS_EPERM     ((int64_t) -1)   /* operation not permitted     */
#define SCPCS_ENOENT    ((int64_t) -2)   /* no such file or directory   */
#define SCPCS_ESRCH     ((int64_t) -3)   /* no such process             */
#define SCPCS_EINTR     ((int64_t) -4)   /* interrupted by a signal     */
#define SCPCS_EIO       ((int64_t) -5)   /* I/O error                   */
#define SCPCS_ECHILD    ((int64_t)-10)   /* no child processes          */
#define SCPCS_EAGAIN    ((int64_t)-11)   /* try again (would block)     */
#define SCPCS_ENOMEM    ((int64_t)-12)   /* out of memory               */
#define SCPCS_EFAULT    ((int64_t)-14)   /* bad address                 */
#define SCPCS_EBUSY     ((int64_t)-16)   /* device or resource busy     */
#define SCPCS_EINVAL    ((int64_t)-22)   /* invalid argument            */
#define SCPCS_ENOSYS    ((int64_t)-38)   /* not implemented             */

/* ══ Group 1 — memory Management ═══════════════════════════════════ */

/* fork() — PSA Algorithm 1, via dupreg (Algorithm 9) and attachreg
 * (Algorithm 4).
 *
 * Returns the child's PID in the parent and 0 in the child, which is the
 * only way a caller can tell which process it is.
 *
 * ENOSYS today: the callee runs the resource checks and returns 0
 * unconditionally, to parent and child alike.  Returning that 0 would
 * tell every caller it is the child, so the wrapper refuses instead. */
int64_t uiox_kix_scpcs_fork(void);

/* execve() — PSA Algorithm 6, with Algorithm 7 (xalloc) for the text
 * region.  All six region calls it needs are implemented in 40psa.
 *
 * Returns 0 on success — exec never returns to the caller.
 *
 * ENOSYS: the header verification fabricates the magic it then tests,
 * and the inode it receives is NULL, so no file is ever read.
 *
 * path is validated through the user-address gate; argv and envp may be
 * NULL per POSIX and are not dereferenced. */
int64_t uiox_kix_scpcs_execve(uiox_uintptr_t path, uiox_uintptr_t argv,
                              uiox_uintptr_t envp);

/* brk() — PSA Algorithm 9, via growreg (Algorithm 5).
 *
 * Returns the PREVIOUS break on success — not the new one — or ENOMEM
 * when the requested value is rejected.  That old-value convention is
 * the opposite of nice(), and both are deliberate in POSIX.
 *
 * The validation is real; the region resize and the zeroing of the new
 * space are not. */
int64_t uiox_kix_scpcs_brk(uiox_uint64_t new_brk);

/* exit() — PSA Algorithm 4, via detachreg (Algorithm 8).
 *
 * Does not return on success.  Status is 8 bits, the low byte reserved
 * for a terminating signal number, so a wider value is EINVAL rather
 * than a silent truncation.
 *
 * The callee prints the steps and returns; it does not set ZOMBIE and
 * does not send SIGCHLD, which is why no process is ever reapable. */
int64_t uiox_kix_scpcs_exit(uiox_uint64_t exit_code);

/* ══ Group 2 — Synchronization ═════════════════════════════════════ */

/* waitpid() — PSA Algorithm 5.
 *
 * Returns the reaped child's PID, or ECHILD when there is nothing to
 * reap.  A non-NULL status pointer is refused with EFAULT because the
 * kernel has no copy_to_user, and writing through a raw user address
 * from kernel mode is the bug that layer exists to prevent.  NULL is
 * legal per POSIX.
 *
 * ENOSYS while the zombie transition and the child scan are unfinished:
 * the callee's "has_children" flag is never set, so it always reports
 * no children. */
int64_t uiox_kix_scpcs_wait_pid(uiox_uint64_t pid, uiox_uintptr_t status,
                                uiox_uint64_t options);

/* kill() — Group 2.  Sends a signal to a process by PID.
 *
 * Returns 0 on success, ESRCH when the PID names no live process.
 *
 * The PID lookup is real, so ESRCH is a genuine answer.  Delivery is
 * ENOSYS: the send is commented out in the signal implementation.
 *
 * Signal 0 is POSIX's existence check rather than a signal, and the
 * kill(0, ...) and kill(-1, ...) group forms cannot be expressed — the
 * process table entry has no group field. */
int64_t uiox_kix_scpcs_kill(uiox_uint64_t pid, uiox_uint64_t sig);

/* sigaction() — Group 2.  Installs a signal disposition.
 *
 * Returns 0 on success, EINVAL for an invalid signal or an attempt to
 * change SIGKILL/SIGSTOP.
 *
 * ENOSYS because the process table entry carries no handler array: the
 * dispositions live on a private structure inside the signal
 * implementation, reached by a cast.  Returning success would claim a
 * handler was installed when nothing was stored. */
int64_t uiox_kix_scpcs_sig_action(uiox_uint64_t sig, uiox_uintptr_t act,
                                  uiox_uintptr_t oldact);

/* sigprocmask() — Group 2.  Changes the blocked-signal set.
 *
 * Returns 0 on success, EINVAL for an unknown operation code.
 *
 * The storage is real — p_sigmask exists — but the call is ENOSYS for
 * two reasons: set and oldset are user addresses with no copy_to_user,
 * and the signal implementation tests a CAST mask rather than
 * p_sigmask, so writing the real field would have no effect.  A write
 * nothing reads is a silent no-op dressed as success. */
int64_t uiox_kix_scpcs_sig_procmask(uiox_uint64_t how, uiox_uintptr_t set,
                                    uiox_uintptr_t oldset);

/* sigpending() — Group 2.  Returns the signals both pending and blocked.
 *
 * The mask comes back in the return value, because there is no
 * copy_to_user to write it to set.  A non-NULL set is refused with
 * EFAULT rather than ignored: accepting it would tell the caller a write
 * happened.
 *
 * The intersection is the meaning — a pending signal that is NOT blocked
 * is about to be delivered, so it waits for nothing. */
int64_t uiox_kix_scpcs_sig_pending(uiox_uintptr_t set);

/* raise() — Group 2.  Sends a signal to the calling process.
 *
 * Returns 0 on success.  Needs no PID lookup and no PID validation, so
 * it can work before the process table is searchable — which makes it
 * the cheapest first test of the signal delivery path. */
int64_t uiox_kix_scpcs_raise(uiox_uint64_t sig);

/* ══ Group 3 — Miscellaneous and identities ════════════════════════ */

/* getpid() — returns the calling process's PID.  REAL.
 * ESRCH when no process is current.  Zero is a legal PID (the swapper),
 * so it is never used to mean "none". */
int64_t uiox_kix_scpcs_get_pid(void);

/* getppid() — returns the parent's PID.  REAL from the field, but the
 * value is not yet trustworthy: fork returns 0 to the parent as well as
 * the child, so the parent recorded here never learned of its child. */
int64_t uiox_kix_scpcs_get_ppid(void);

/* getuid() — the real user ID.  REAL. */
int64_t uiox_kix_scpcs_get_uid(void);

/* geteuid() — the effective user ID, the one that decides access.
 * REAL, and the exec path already writes it for a setuid image. */
int64_t uiox_kix_scpcs_get_euid(void);

/* getgid() — the real group ID.  REAL. */
int64_t uiox_kix_scpcs_get_gid(void);

/* getegid() — the effective group ID, consulted for group access.
 * REAL. */
int64_t uiox_kix_scpcs_get_egid(void);

/* setuid() — the permission-checked form of what exec already does.
 *
 * Returns 0 on success, EPERM when the caller may not take the identity.
 * The privilege rule is real and evaluated: root sets both IDs, a
 * process may re-assert either of its own, and anything else is EPERM.
 *
 * ENOSYS is NOT returned for a refused identity — that would report a
 * missing implementation where a real decision was made.  The saved-ID
 * half of POSIX setuid is what cannot be expressed, for want of a field. */
int64_t uiox_kix_scpcs_set_uid(uiox_uint64_t uid);

/* getpgrp() — returns the process group ID.
 *
 * ENOSYS: the concept is in the algorithm text's Miscellaneous column,
 * but the process table entry has no group field.  Returning 0 would
 * claim membership of the swapper's group — a wrong answer of a kind the
 * caller cannot detect, since group IDs are opaque. */
int64_t uiox_kix_scpcs_get_pgrp(void);

/* nice() — changes the scheduling niceness, returns the NEW value.
 *
 * POSIX specifies the new value here, the opposite of brk's old-value
 * convention.
 *
 * A positive increment LOWERS priority: the user convention is
 * subtracted because this kernel's field is lower-is-better.  The result
 * is clamped at both ends, since clamping only the high end would let a
 * caller outrank the swapper. */
int64_t uiox_kix_scpcs_nice(uiox_uint64_t inc);

/* times() — returns process CPU accounting, struct tms shape.
 *
 * ENOSYS, and the blocker is not the user write: nothing CHARGES the
 * four timer fields yet, so they hold the allocator's zeroing rather
 * than measurements.  A well-formed packet of zeros would tell a caller
 * it had been idle rather than unbilled.
 *
 * A non-NULL buffer is refused with EFAULT for want of a copy_to_user. */
int64_t uiox_kix_scpcs_times(uiox_uintptr_t buf_out);

/* ══ Time and waiting ══════════════════════════════════════════════ */

/* nanosleep() — sleeps for a requested interval, PSA Algorithm 10.
 *
 * Returns 0 when the full interval elapsed, EINTR if a signal cut it
 * short.  req is mandatory and validated; rem is optional and validated
 * only when non-NULL.
 *
 * ENOSYS because the WAKEUP half is missing: the sleeping half is
 * complete, but nothing calls the wakeup routine when the deadline
 * passes, so a timed sleep never ends.  A busy-wait was rejected as
 * worse than not sleeping — it would starve every other process. */
int64_t uiox_kix_scpcs_nano_sleep(uiox_uintptr_t req, uiox_uintptr_t rem);

/* clock_gettime() — REAL end to end.  The only fully working call in
 * this layer.
 *
 * Returns nanoseconds in bits 63..32 and seconds in bits 31..0, a
 * stopgap forced by the missing copy_to_user and replaced the moment
 * one exists.  Seconds saturate rather than wrap, so an implausible
 * reading is obviously implausible instead of looking like a real time.
 *
 * Only the realtime and monotonic identifiers are accepted; returning
 * wall-clock time for a CPU-time request would be worse than refusing. */
int64_t uiox_kix_scpcs_clock_get_time(uiox_uint64_t clock_id,
                                      uiox_uintptr_t out);

/* pause() — suspends until a signal is delivered.
 *
 * Returns EINTR when a signal arrives, which is the normal outcome
 * rather than an error.
 *
 * ENOSYS today, but this is the cheapest call in the layer to finish:
 * it needs NO timer, only an interruptible sleep above the
 * interruptibility threshold, and the signal check on the return-to-user
 * path already exists.  Its priority MUST be above that threshold, or
 * the sleep becomes uninterruptible and pause becomes unkillable. */
int64_t uiox_kix_scpcs_pause(void);

/* alarm() — schedules a SIGALRM.
 *
 * Returns the seconds REMAINING on a previously set alarm.
 *
 * ENOSYS: the callout table and an alarm entry point both exist, but
 * nothing is bound to the CALLING process, so replacement semantics
 * cannot be expressed and the remaining-seconds return is unanswerable.
 * Reporting 0 would claim no alarm was pending, which the caller's
 * decision turn on. */
int64_t uiox_kix_scpcs_alarm(uiox_uint64_t seconds);

/* ── The ONE place a process pointer is obtained ──────────────────────
 * Several shapes of "current process" exist across the tree and two of
 * them spell the pointer differently.  Routing every entry point through
 * this accessor means that question has one answer to correct, not
 * twenty-five.
 *
 * Returns NULL when no process is current; callers turn that into ESRCH. */
uiox_kix_psa_proc_t *uiox_kix_scpcs_current(void);

/* ── The user-address gate ────────────────────────────────────────────
 * There is no copy_from_user or copy_to_user in the tree, so a wrapper
 * has two honest options for a user pointer: refuse it, or validate it
 * and dereference.  Doing the second without this check is how a syscall
 * becomes an arbitrary read/write primitive in kernel mode.
 *
 * Returns 0 when [p, p+len) lies inside the user window and is aligned;
 * SCPCS_EFAULT otherwise.  A zero length is always valid.  The window
 * bounds restate the 32-bit model and change in one place when 10BSP's
 * SoC memory map and 02_MemMngnt's symbols are reconciled. */
int uiox_kix_scpcs_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                                  uiox_uint64_t align);

#endif /* UIOX_KIX_SCPCS_H */
