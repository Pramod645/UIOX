/*
 * 30KIX/33PCS/00IPC/include/uiox_kix_sem.h
 *
 * Semaphore sets — Bach's Algorithms 9-11.
 *
 * ── version 3.0.0: the sleep/wakeup binding ─────────────────────────
 * uiox_kix_sem_op took SimProcess * — the fourth process model, deleted.  It takes
 * uiox_kix_psa_proc_t * — the ONE process type.
 *
 * That change is what turned uiox_kix_sem_op from a busy loop into a real wait.
 * Its old body called sim_sleep and then jumped back to restart the
 * operation array; sim_sleep only set a flag on a private struct, so the
 * restart waited for nothing and re-ran immediately.  With 40_psa's
 * sleep the restart yields the CPU.
 *
 * ── wait_incr and wait_zero ─────────────────────────────────────────
 * Counters of how many processes are waiting on each condition, per
 * semaphore.  They are NOT the sleep mechanism — Bach uses them to
 * decide who to wake: a V that takes the value to zero wakes the
 * zero-waiters, any V wakes the increment-waiters.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#ifndef UIOX_SEM_H
#define UIOX_SEM_H

#include "uiox_kix_ipc_types.h"
#include "uiox_kix_psa_process.h"

/* ── One semaphore ───────────────────────────────────────────────────
 * last_pid is a copy of a pid, not a reference — a set outlives the
 * processes that operate on it. */
typedef struct {
    int  val;         /* current value                          */
    int  last_pid;    /* pid of the last process to operate     */
    int  wait_incr;   /* waiters for the value to rise          */
    int  wait_zero;   /* waiters for the value to reach zero    */
} Semaphore;

/* ── A set ───────────────────────────────────────────────────────────
 * nsems says how many of sems[] are in use; the array is sized to the
 * maximum so the set needs no allocation. */
typedef struct {
    IpcPerm    perm;
    Semaphore  sems[IPC_MAX_SEMS];
    int        nsems;
    time_t     otime;   /* last uiox_kix_sem_op  */
    time_t     ctime;   /* last uiox_kix_sem_ctl */
    bool       active;
} SemSet;

/* ── One operation in a uiox_kix_sem_op array ──────────────────────────────────
 * sem_op's SIGN selects the operation:
 *   > 0  V, release          < 0  P, acquire         == 0  wait for zero */
typedef struct {
    int  sem_num;   /* which semaphore in the set        */
    int  sem_op;    /* the operation and its magnitude   */
    int  sem_flg;   /* IPC_NOWAIT | IPC_UNDO             */
} SemBuf;

/* uiox_kix_sem_ctl command parameters */
#define GETVAL   4
#define SETVAL   5
#define GETALL   6
#define SETALL   7

/* ── API ───────────────────────────────────────────────────────────── */

void uiox_kix_sem_init(void);

/* Algorithm 9 — find by key, or create a set of nsems. */
int uiox_kix_sem_get(int key, int nsems, int flag);

/* Algorithm 10 — GETVAL/SETVAL on one, GETALL/SETALL on the set,
 * IPC_RMID to remove it and wake its waiters. */
int uiox_kix_sem_ctl(int semid, int semnum, int cmd, int val);

/* Algorithm 11 — apply the array ATOMICALLY.
 *
 * If an operation cannot proceed, those already applied are reversed and
 * the whole array RESTARTS after a real sleep — Bach restarts rather
 * than resuming, because another process may have changed any operation
 * while this one waited.
 *
 * Returns 0 on success, or -1 for an invalid descriptor, IPC_NOWAIT on a
 * blocked operation, or an uncaught signal. */
int uiox_kix_sem_op(int semid, SemBuf *ops, int nops, uiox_kix_psa_proc_t *caller);

#endif /* UIOX_SEM_H */
