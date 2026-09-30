/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_ipc_types.h
 *
 * Shared IPC constants, the permission block, and the sleep/wakeup the
 * three System V mechanisms use.
 *
 * ── SimProcess is gone ──────────────────────────────────────────────
 * This header used to define a FOURTH process model:
 *
 *     typedef struct SimProcess {
 *         int  pid;
 *         int  uid;
 *         int  gid;
 *         bool traced;
 *         bool sleeping;
 *         int  wake_event;
 *     } SimProcess;
 *
 * `pid` was its own spelling — neither 40_psa's p_pid, nor the
 * proc_entry_t this tree has finished deleting, nor uiox_task_t's.  A
 * fourth name for one concept is the defect this pass exists to remove,
 * so every function below that took a SimProcess * now takes a
 * uiox_kix_psa_proc_t * instead.
 *
 * ── sim_sleep / sim_wakeup are gone, and that mattered more ─────────
 * The header carried two inline stubs:
 *
 *     static inline void sim_sleep(SimProcess *p, int event) { ... }
 *     static inline void sim_wakeup(SimProcess *p, int event) { ... }
 *
 * They set and cleared a flag.  Nothing ELSE could observe that flag, so
 * a process that `sim_sleep`ed was never actually woken by anything
 * outside this subsystem — and in two places the result was not a sleep
 * at all:
 *
 *   uiox_kix_sem_op      called sim_sleep, then `goto start` and re-ran the
 *              operation array — a BUSY LOOP, not a wait
 *   uiox_kix_msg_snd     called sim_sleep on a full queue, and uiox_kix_msg_rcv printed
 *              "wakeup: processes waiting to read" without ever calling
 *              sim_wakeup
 *
 * Both now use 40_psa's real pair — uiox_kix_psa_proc_sleep and
 * uiox_kix_psa_proc_wakeup — which have a wait-channel hash, a genuine
 * context switch, and an interruptible path.  The busy loop becomes a
 * sleep, and the printed-but-never-delivered wakeup becomes real.
 *
 * ── what the sleep takes now ────────────────────────────────────────
 * 40_psa's sleep signature is:
 *
 *     int uiox_kix_psa_proc_sleep(uintptr_t wchan, int priority,
 *                                 int interruptible)
 *
 * so a wait is identified by a CHANNEL ADDRESS, not by an event number.
 * The EVENT_* constants that were integers are now pointers used as
 * channels — each queue, semaphore set and socket gets its own address,
 * which is finer-grained than the old five shared numbers.  Two message
 * queues no longer share "EVENT_MSG_ARRIVE".
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_IPC_TYPES_H
#define UIOX_IPC_TYPES_H

#include "../include/uiox_klibc.h"
#include "uiox_kix_psa_process.h"   /* the ONE process type */

/* ── General IPC limits ─────────────────────────────────────────────
 * Unchanged.  Every one of these bounds a static table or pool, so a
 * change here changes memory use at compile time, not at run time. */
#define IPC_MAX_QUEUES      16
#define IPC_MAX_MSGS        32
#define IPC_MAX_MSG_BYTES   512
#define IPC_MAX_QUEUE_BYTES (IPC_MAX_MSGS * IPC_MAX_MSG_BYTES)
#define IPC_MAX_SHM         16
#define IPC_MAX_SHM_SIZE    (64 * 1024)   /* 64 KB per region      */
#define IPC_MAX_SEM_SETS    16
#define IPC_MAX_SEMS        16            /* semaphores per set     */
#define IPC_MAX_SOCKETS     32
#define IPC_MAX_PENDING     8             /* listen backlog         */
#define IPC_MAX_PROCESSES   32

/* ── Creation and control flags ────────────────────────────────────── */
#define IPC_CREAT   0x0200
#define IPC_EXCL    0x0400
#define IPC_NOWAIT  0x0800
#define IPC_RMID    1
#define IPC_SET     2
#define IPC_STAT    3
#define IPC_UNDO    0x1000

/* ── Special keys ──────────────────────────────────────────────────── */
#define IPC_PRIVATE 0

/* ── Permission block ────────────────────────────────────────────────
 * Common to all three System V mechanisms.  uid and gid are plain ints
 * rather than uiox_kix_psa_proc_t's uint16_t, because this is a
 * user-visible STRUCTURE copied out through IPC_STAT — its layout is
 * part of the interface, not an internal record. */
typedef struct {
    int      key;
    uiox_uint16_t mode;    /* permission bits                     */
    int      uid;
    int      gid;
} IpcPerm;

/* ── Wait channels ───────────────────────────────────────────────────
 * 40_psa's sleep takes an ADDRESS, so a wait is identified by where a
 * caller sleeps rather than by a small integer.  These macros name the
 * channels the IPC mechanisms use.
 *
 * The distinction from the old EVENT_* numbers: a channel is derived
 * from the THING waited on, so queue 3's "space available" is a
 * different address from queue 7's.  The old scheme had one number for
 * both, and a wakeup on either woke both.
 *
 * A macro rather than a variable so two translation units naming the
 * same queue compute the same address. */
#define IPC_WCHAN_MSG_SPACE(q)      ((uiox_uintptr_t)(q))
#define IPC_WCHAN_MSG_ARRIVE(q)     ((uiox_uintptr_t)(q))
#define IPC_WCHAN_SEM_INCR(s)       ((uiox_uintptr_t)(s))
#define IPC_WCHAN_SEM_ZERO(s)       ((uiox_uintptr_t)(s))
#define IPC_WCHAN_SOCK_CONN(s)      ((uiox_uintptr_t)(s))

/* ── The sleep priority ──────────────────────────────────────────────
 * ABOVE 40_psa's PZERO threshold, which is what makes the sleep
 * INTERRUPTIBLE.  Bach and POSIX both require that: a process waiting on
 * a message queue or a semaphore must be killable, and a sleep at or
 * below PZERO cannot be ended by a signal.
 *
 * PWAIT is the conventional level for a wait on another process — the
 * same tier as a parent waiting for a child. */
#define IPC_WAIT_PRIORITY   (UIOX_KIX_PSA_PWAIT)

/* ── Sleep and wakeup, delegating to 40_psa ──────────────────────────
 * Thin wrappers rather than macros, so the return code 40_psa's sleep
 * produces is carried through and a caller can act on it:
 *
 *    0   woken normally — the wait is over, retry
 *    1   woken by a CAUGHT signal — retry, but a signal is pending
 *   -1   woken by an UNCAUGHT signal — the syscall must unwind
 *
 * The old sim_sleep had no return value, so a caller could not tell a
 * normal wakeup from one caused by a signal.  uiox_kix_sem_op's retry loop needs
 * that distinction: a signal should end the operation array, not
 * re-run it. */
static inline int ipc_sleep(uiox_kix_psa_proc_t *p, uiox_uintptr_t wchan)
{
    (void)p;   /* the channel identifies the wait; 40_psa reads current */
    return uiox_kix_psa_proc_sleep(wchan, IPC_WAIT_PRIORITY, 1);
}

/* Wake every process waiting on a channel.  Bach's wakeup wakes all
 * waiters, not one — each re-checks its condition and the first that
 * finds it satisfied proceeds. */
static inline void ipc_wakeup(uiox_uintptr_t wchan)
{
    uiox_kix_psa_proc_wakeup(wchan);
}

#endif /* UIOX_IPC_TYPES_H */
