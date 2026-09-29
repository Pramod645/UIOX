/*
 * 30_KIX/33_PCS/40psa/include/uiox_kix_psa_process.h
 *
 * PSA — Process system Architecture.  The PROCESS TABLE half.
 *
 * PSA splits a process into two halves, and this header is the first:
 *
 *   1. Process table — "contains fields that must always be accessible
 *      to the kernel".  Defined here.  Space is allocated for EVERY
 *      table slot, occupied or not.
 *
 *   2. U area — "contains fields that need to be accessible only to the
 *      running process".  In uiox_kix_psa_proc_algo.h, because PSA adds:
 *      "the kernel allocates space for the u area only when creating a
 *      process, it does not need u area for process table entries that
 *      do not have processes."
 *
 * ── the state model ─────────────────────────────────────────────────
 * Ten enumerators: PSA's nine states, plus UIOX_KIX_PSA_PROC_UNUSED as
 * the allocator's "this slot is free" marker.  PSA numbers the states
 * 1..9 itself and this header keeps that numbering exactly, so a state
 * diagram arrow can be read straight off the value.  State 9 Zombie is,
 * in PSA's words, "the final state of a process".
 *
 * ── signal numbers ──────────────────────────────────────────────────
 * These are the values this layer's algorithms use.  They deliberately
 * do NOT match 50_scps's positional descriptor table, which places
 * SIGCHLD on 18.  It is 17 here, and 17 is what PSA's "death of child"
 * transition assumes.  Any code that mixes the two tables reads the
 * wrong signal, so the disagreement is recorded here rather than
 * silently resolved.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_PSA_PROCESS_H
#define UIOX_KIX_PSA_PROCESS_H

#include "uiox_klibc.h"

/* ── Compile-time limits ───────────────────────────────────────────
 * NPROC sizes the process table.  NOFILE bounds the per-process open
 * file table in the u area.  NSIG bounds the signal arrays. */
#define UIOX_KIX_PSA_NPROC           64
#define UIOX_KIX_PSA_NOFILE          20
#define UIOX_KIX_PSA_NSIG            32
#define UIOX_KIX_PSA_KERNEL_STACK_SZ 4096

/* ── Process states — PSA states 1..9, plus an allocator marker ────
 * The enum is CLOSED.  uiox_kix_psa_proc_set_state() refuses any value
 * outside this range and any transition the PSA diagram does not draw. */
typedef enum uiox_kix_psa_proc_state {
    UIOX_KIX_PSA_PROC_UNUSED         = 0,  /* free table slot — not a PSA state */
    UIOX_KIX_PSA_PROC_USER_RUNNING   = 1,  /* PSA 1: executing in user mode    */
    UIOX_KIX_PSA_PROC_KERNEL_RUNNING = 2,  /* PSA 2: executing in kernel mode  */
    UIOX_KIX_PSA_PROC_READY          = 3,  /* PSA 3: ready, resident in memory */
    UIOX_KIX_PSA_PROC_SLEEP_MEM      = 4,  /* PSA 4: sleeping, resident        */
    UIOX_KIX_PSA_PROC_READY_SWAPPED  = 5,  /* PSA 5: ready, swapped out        */
    UIOX_KIX_PSA_PROC_SLEEP_SWAPPED  = 6,  /* PSA 6: sleeping, swapped out     */
    UIOX_KIX_PSA_PROC_PREEMPTED      = 7,  /* PSA 7: returning to user, preempted */
    UIOX_KIX_PSA_PROC_CREATED        = 8,  /* PSA 8: newly created, in transition */
    UIOX_KIX_PSA_PROC_ZOMBIE         = 9   /* PSA 9: exited; final state       */
} uiox_kix_psa_proc_state_t;

/* ── Signal numbers ──────────────────────────────────────────────────
 * The three-argument signal routines index u_signal[] with these, so a
 * mismatch with 50_scps lands the handler in the wrong slot. */
typedef enum uiox_kix_psa_signal {
    UIOX_KIX_PSA_SIG_NONE   =  0,
    UIOX_KIX_PSA_SIGHUP     =  1,   /* terminal hangup                  */
    UIOX_KIX_PSA_SIGINT     =  2,   /* interrupt from keyboard          */
    UIOX_KIX_PSA_SIGQUIT    =  3,   /* quit from keyboard               */
    UIOX_KIX_PSA_SIGILL     =  4,   /* illegal instruction              */
    UIOX_KIX_PSA_SIGTRAP    =  5,   /* trace trap                       */
    UIOX_KIX_PSA_SIGABRT    =  6,   /* abort                            */
    UIOX_KIX_PSA_SIGKILL    =  9,   /* kill — uncatchable               */
    UIOX_KIX_PSA_SIGPIPE    = 13,   /* write to a pipe with no reader   */
    UIOX_KIX_PSA_SIGALRM    = 14,   /* alarm clock                      */
    UIOX_KIX_PSA_SIGTERM    = 15,   /* software termination             */
    UIOX_KIX_PSA_SIGCHLD    = 17,   /* death of a child                 */
    UIOX_KIX_PSA_SIGCONT    = 18,   /* continue after stop              */
    UIOX_KIX_PSA_SIGSTOP    = 19    /* stop — uncatchable               */
} uiox_kix_psa_signal_t;

/* ── Scheduling parameters ───────────────────────────────────────────
 * Four fields, as PSA's process table lists them.
 *   p_pri  — scheduling priority.  LOWER value = HIGHER priority, which
 *            is the convention sched_enqueue's ordering relies on.
 *   p_cpu  — accumulated CPU use, fed to the priority recalculation.
 *   p_nice — user-set offset applied by the nice() adjustment.
 *   p_time — residence time, used by the scheduler's decay. */
typedef struct uiox_kix_psa_sched_param {
    int p_pri;
    int p_cpu;
    int p_nice;
    int p_time;
} uiox_kix_psa_sched_param_t;

/* ── Timer record — exactly struct tms ───────────────────────────────
 * Four fields whose order matches struct tms, so a times() implementation
 * can copy them straight out:
 *   utime  — CPU time charged to this process in user mode
 *   stime  — CPU time charged to this process in kernel mode
 *   cutime — utime of all reaped children, summed
 *   cstime — stime of all reaped children, summed
 * The last two are why wait() adds a child's usage to its parent. */
typedef struct uiox_kix_psa_proc_timer {
    clock_t p_utime;
    clock_t p_stime;
    clock_t p_cutime;
    clock_t p_cstime;
} uiox_kix_psa_proc_timer_t;

/* ── Process table entry ─────────────────────────────────────────────
 * One slot per potential process.  Every field here is reachable by the
 * kernel at any time, which is what separates this struct from the
 * u area.  The two link fields, p_next and p_prev, are reused by BOTH
 * the ready queue and the sleep hash; a process is on at most one of
 * them at a time. */
typedef struct uiox_kix_psa_proc {
    uiox_kix_psa_proc_state_t  p_state;    /* current state; drives everything */
    uint32_t                   p_pid;      /* process ID                       */
    uint32_t                   p_ppid;     /* parent's PID, for wait/reparent  */
    uint16_t                   p_uid;      /* real user ID                     */
    uint16_t                   p_euid;     /* effective user ID — access checks */
    uint16_t                   p_gid;      /* real group ID                    */
    uint16_t                   p_egid;     /* effective group ID               */
    uint32_t                   p_size;     /* process size, for memory accounting */
    void                      *p_addr;     /* locates the process in memory    */
    uintptr_t                  p_wchan;    /* sleep channel; 0 = not sleeping  */
    uiox_kix_psa_sched_param_t p_sched;    /* scheduling parameters            */
    uint32_t                   p_sig;      /* pending-signal bitmask           */
    uint32_t                   p_sigmask;  /* blocked-signal bitmask           */
    uiox_kix_psa_proc_timer_t  p_timers;   /* CPU accounting, struct tms shape */
    int                        p_flag;     /* P_* status bits                  */
    int                        p_exit_code;/* status a parent collects         */

    /* ── alarm state ───────────────────────────────────────────────
     * alarm() is a PROCESS syscall, so the outstanding alarm is
     * per-process state and lives here — not on the scheduler's
     * wrapper, which holds scheduling POLICY rather than anything a
     * syscall reads and writes.
     *
     * The pair are the two halves POSIX alarm needs and neither can
     * be derived from the other: p_alarm_expire is WHEN it fires, and
     * p_alarm_active distinguishes "an alarm is set" from "the
     * deadline has passed but nothing cleared it".  A single deadline
     * field could not tell those apart.
     *
     * A new alarm REPLACES the old, which is why arming writes both. */
    uint64_t                   p_alarm_expire;  /* jiffies it fires at       */
    int                        p_alarm_active;  /* 1 = an alarm is armed     */
    struct uiox_kix_psa_proc  *p_next;     /* ready-queue / sleep-hash forward */
    struct uiox_kix_psa_proc  *p_prev;     /* ... and backward                 */
} uiox_kix_psa_proc_t;

/* ── Process status flags ────────────────────────────────────────────
 * P_SWAPPED is maintained automatically by proc_set_state whenever the
 * new state is a swapped one.  P_ZOMBIE is an alias for P_WAITED rather
 * than a separate bit, because the two are set at the same moment. */
#define UIOX_KIX_PSA_P_LOADED   0x0001   /* process is resident           */
#define UIOX_KIX_PSA_P_STICKY   0x0002   /* setuid image: keep in memory  */
#define UIOX_KIX_PSA_P_INTERR   0x0004   /* terminated by a signal        */
#define UIOX_KIX_PSA_P_SIGCATCH 0x0008   /* catching signals              */
#define UIOX_KIX_PSA_P_SWAPPED  0x0010   /* swapped out                   */
#define UIOX_KIX_PSA_P_TRACED   0x0020   /* traced by ptrace              */
#define UIOX_KIX_PSA_P_WAITED   0x0040   /* exit status reaped by parent  */
#define UIOX_KIX_PSA_P_ZOMBIE   UIOX_KIX_PSA_P_WAITED

/* ── Sleep priority levels ───────────────────────────────────────────
 * proc_sleep compares the priority it is given against PZERO to decide
 * whether the sleep is interruptible:
 *   <= PZERO  the sleep is UNINTERRUPTIBLE — only an explicit wakeup
 *             returns it, never a signal
 *   >  PZERO  the sleep is INTERRUPTIBLE — a signal wakes it
 * The named levels are the conventional sleep reasons, ordered from the
 * highest-priority sleeper (PSWP, the swapper) to the lowest
 * (PUSER, an ordinary user wait). */
#define UIOX_KIX_PSA_PSWP    0    /* swapper                              */
#define UIOX_KIX_PSA_PINOD   10   /* inode wait                           */
#define UIOX_KIX_PSA_PRIBIO  20   /* buffer I/O wait                      */
#define UIOX_KIX_PSA_PZERO   25   /* the interruptibility threshold       */
#define UIOX_KIX_PSA_PWAIT   30   /* wait() for a child                   */
#define UIOX_KIX_PSA_PSLEP   40   /* low-priority sleep                   */
#define UIOX_KIX_PSA_PUSER   50   /* user-mode sleep, lowest priority     */

/* ── Globals ─────────────────────────────────────────────────────────
 * uiox_kix_psa_proc_table is the whole table; the accessor functions
 * below are the only sanctioned way to reach a slot. */
extern uiox_kix_psa_proc_t  uiox_kix_psa_proc_table[UIOX_KIX_PSA_NPROC];
extern uiox_kix_psa_proc_t *uiox_kix_psa_current_proc;

/* ── Process table API ─────────────────────────────────────────────── */

/* Allocate a free table slot and initialise it to state CREATED.
 * pid  — explicit PID, or 0 to auto-assign the next free one
 * ppid — the allocating process's PID, recorded for wait/reparent
 * Returns the slot, or NULL when the table is full.  On success the
 * caller must move the process out of CREATED with proc_set_state. */
uiox_kix_psa_proc_t *uiox_kix_psa_proc_alloc(uint32_t pid, uint32_t ppid);

/* Return a slot to the free pool.  Zeroes the whole entry and sets
 * UNUSED, so a stale p_next cannot keep a freed process on a queue.
 * Does NOT reap children or close files — the exit algorithm's job. */
void uiox_kix_psa_proc_free(uiox_kix_psa_proc_t *p);

/* Look up a process by PID.  Skips UNUSED slots, so a freed slot is
 * never found.  Returns NULL if no live process has that PID. */
uiox_kix_psa_proc_t *uiox_kix_psa_proc_find(uint32_t pid);

/* Validate and apply a state transition against the PSA state diagram.
 * Returns 0 on success; -1 if the pointer is NULL, the target state is
 * out of range, or the diagram does not draw an arrow from the current
 * state to the requested one.  Maintains P_SWAPPED as a side effect. */
int uiox_kix_psa_proc_set_state(uiox_kix_psa_proc_t *p,
                                uiox_kix_psa_proc_state_t new_state);

/* Count signals that are BOTH pending and unblocked — the test the
 * interruptible branch of proc_sleep uses to decide whether it may
 * sleep at all.  Returns a bitmask; 0 means nothing is deliverable. */
int uiox_kix_psa_proc_sig_pending(uiox_kix_psa_proc_t *p);

/* Insert a process into the READY queue in priority order.
 * Uses a queue head SEPARATE from current_proc, because the running
 * process is by definition not on the ready list. */
void uiox_kix_psa_sched_enqueue(uiox_kix_psa_proc_t *p);

/* Unlink a process from the ready queue, fixing up both neighbours and
 * the queue head if the process was at the front. */
void uiox_kix_psa_sched_dequeue(uiox_kix_psa_proc_t *p);

/* PSA's "reschedule process": return the highest-priority READY process,
 * or NULL when nothing is runnable.  Reads only; the caller performs
 * the state change and the register switch. */
uiox_kix_psa_proc_t *uiox_kix_psa_sched_pick(void);

#endif /* UIOX_KIX_PSA_PROCESS_H */
