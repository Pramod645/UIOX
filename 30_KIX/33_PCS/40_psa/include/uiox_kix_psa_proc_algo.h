/*
 * 30_KIX/33_PCS/40psa/include/uiox_kix_psa_proc_algo.h
 *
 * The U AREA, and the two algorithms that operate on it.
 *
 * ── PSA's own justification for the split ───────────────────────────
 *   "1. Process table: contains fields that must always be accessible
 *       to the kernel
 *    2. U area: u area contains fields that need to be accessible only
 *       to the running process
 *
 *    Kernel allocates space for the u area only when creating a
 *    process, it does not need u area for process table entries that do
 *    not have processes."
 *
 * So the u area is allocated per RUNNING process, not per table slot.
 * The global below is the one instance belonging to whoever is running.
 *
 * ── deliberately absent: a syscall table ────────────────────────────
 * The previous version of this header carried syscall_table[256],
 * syscall_register() and a syscall() dispatcher — a third dispatcher
 * alongside 50_scps's own.  System calls are 50_scps's job.  Two tables
 * means two answers to "what does syscall number 9 do", and the loser is
 * whichever one a build happens to link.
 *
 * The u area still holds u_error and u_rval, because those ARE per-process
 * state that a syscall writes into — the DISPATCH simply does not happen
 * here.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PSA_PROC_ALGO_H
#define UIOX_KIX_PSA_PROC_ALGO_H

#include "uiox_kix_psa_process.h"
#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_context.h"
#include "uiox_klibc.h"

/* ── Error codes the algorithms place in u_error ───────────────────── */
#ifndef EINVAL
#define EINVAL  22   /* bad argument                                  */
#endif
#ifndef EINTR
#define EINTR    4   /* system call interrupted by a signal           */
#endif

/* ── Sleep hash ──────────────────────────────────────────────────────
 * Sleeping processes are hashed by wait channel so a wakeup does not
 * have to scan the whole table.  SLEEP_HASH_SZ sets the bucket count. */
#define UIOX_KIX_PSA_SLEEP_HASH_SZ 64

typedef struct uiox_kix_psa_sleep_queue {
    uiox_kix_psa_proc_t *sq_head;   /* first sleeper on this channel   */
    uiox_kix_psa_proc_t *sq_tail;   /* last sleeper — maintained by enqueue */
} uiox_kix_psa_sleep_queue_t;

/* ── Abort flag for an unwinding system call ─────────────────────────
 * Freestanding has no <setjmp.h>, so longjmp() cannot be used to unwind
 * a syscall that a signal interrupted.  regs[0] standing non-zero is the
 * replacement: the syscall entry checks it and abandons the call.  It is
 * a flag, not a jump — the unwinding is cooperative. */
#define UIOX_KIX_PSA_JMP_BUF_SLOTS 8u
typedef struct uiox_kix_psa_jmp_buf {
    uint64_t regs[UIOX_KIX_PSA_JMP_BUF_SLOTS];
} uiox_kix_psa_jmp_buf_t;

struct uiox_kix_psa_u_area;
typedef struct file  uiox_kix_psa_file_t;
typedef struct inode uiox_kix_psa_inode_t;

/* ── I/O parameters for a load or a read ───────────────────────────
 * Describes a transfer in progress: where, how much, at what offset, and
 * which address space the buffer lives in (io_seg). */
typedef struct uiox_kix_psa_io_params {
    char     *io_base;
    uint32_t  io_count;
    uint32_t  io_offset;
    int       io_seg;
} uiox_kix_psa_io_params_t;

/* ── The U AREA ──────────────────────────────────────────────────────
 * Per-running-process state.  Everything reachable only by the process
 * that owns it lives here rather than on the process table entry. */
typedef struct uiox_kix_psa_u_area {
    uiox_kix_psa_proc_t       *u_proc;     /* back-pointer to table slot  */
    uiox_kix_psa_file_t       *u_ofile[UIOX_KIX_PSA_NOFILE];
    int                        u_signal[UIOX_KIX_PSA_NSIG];
    int                        u_error;    /* errno for the current call  */
    int64_t                    u_rval;     /* return value for the caller */
    int                        u_uid;      /* real user ID                */
    int                        u_gid;      /* real group ID               */
    int                        u_euid;     /* effective user ID           */
    int                        u_egid;     /* effective group ID          */
    uiox_kix_psa_io_params_t   u_io;       /* transfer in progress        */
    uiox_kix_psa_pregion_t     u_pregs[UIOX_KIX_PSA_MAX_REG_PER_PROC];
    uiox_kix_psa_jmp_buf_t     u_qsave;    /* abort flag, not setjmp      */
    uiox_kix_psa_reg_context_t u_saved_regs;
    uiox_kix_psa_sys_context_t u_sysctx;   /* context stack for interrupts */
} uiox_kix_psa_u_area_t;

/* ── The running process's u area ──────────────────────────────────── */
extern uiox_kix_psa_u_area_t uiox_kix_psa_u;

/* ── Sleep state, shared by both algorithms ──────────────────────────
 * scheduler_flag is set when a woken process outranks the running one,
 * which tells the kernel to reschedule.  proc_level tracks the raised
 * interrupt level so a nested call can restore it. */
extern uiox_kix_psa_sleep_queue_t
       uiox_kix_psa_sleep_hash[UIOX_KIX_PSA_SLEEP_HASH_SZ];
extern int uiox_kix_psa_scheduler_flag;
extern int uiox_kix_psa_proc_level;

/* ── PSA Algorithm 11 — wake every process sleeping on a channel ───
 * Walks the hash bucket, unlinks each match, moves it from a SLEEP state
 * to the matching READY state, and enqueues it.  Sets scheduler_flag when
 * a woken process has higher priority than the running one.  A process
 * sleeping on a DIFFERENT channel in the same bucket is left alone. */
void uiox_kix_psa_proc_wakeup(uintptr_t wchan);

/* ── PSA Algorithm 10 — sleep on a wait channel ──────────────────────
 * wchan         — event address; wakeup must be called with the same one
 * priority      — compared against PZERO to pick the branch
 * interruptible — 0 forces the uninterruptible branch regardless
 *
 * Returns  0  woken normally
 *          1  woken by a signal the process CATCHES (priority > PZERO)
 *         -1  woken by an UNCAUGHT signal: u_error is set to EINTR and
 *             the abort flag is raised so the syscall entry unwinds.
 *
 * An interruptible sleep still sleeps when nothing is pending; it checks
 * for a pending signal both BEFORE and AFTER, which is why a signal
 * arriving between the two is not lost. */
int uiox_kix_psa_proc_sleep(uintptr_t wchan, int priority, int interruptible);

/* ── Context switch primitive — declared, not defined ────────────────
 * PSA names swtch as the switch itself.  The register transfer is
 * per-architecture, so this layer declares it and the arch layer
 * defines it.  Declaring it here keeps the dependency visible. */
void uiox_kix_psa_swtch(void);

#endif /* UIOX_KIX_PSA_PROC_ALGO_H */
