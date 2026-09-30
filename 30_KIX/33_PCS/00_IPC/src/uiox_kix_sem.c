/*
 * 30KIX/33PCS/00IPC/src/sem.c
 *
 * Bach's Algorithms 9-11 — uiox_kix_sem_get, uiox_kix_sem_ctl, uiox_kix_sem_op (atomic P/V with
 * reverse-on-block, UNDO, wait_incr and wait_zero).
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * The caller argument was SimProcess * — the fourth process model, now
 * deleted.  It is uiox_kix_psa_proc_t * — the ONE process type.
 *
 * ── the busy loop this replaces ─────────────────────────────────────
 * The version before this called sim_sleep and then jumped back to
 * re-run the whole operation array:
 *
 *     sim_sleep(caller, EVENT_SEM_INCR);
 *     s->wait_incr--;
 *     goto start;          <-- re-runs the array immediately
 *
 * sim_sleep only set a flag on a private struct, so the goto did not wait
 * for anything — it re-ran the array, found the same semaphore still
 * short, and went round again.  On a uniprocessor that is a hang, and on
 * a multiprocessor it is a spin that starves everyone else.
 *
 * It is now a real sleep on 40_psa's wait channel, taken inside a retry
 * loop.  The loop is still there — Bach restarts the array, and a wakeup
 * does not guarantee the condition now holds — but each pass now yields
 * the CPU.
 *
 * ── the return code, which now matters ──────────────────────────────
 * 40_psa's sleep returns -1 when an UNCAUGHT signal ended the wait.
 * That must unwind the syscall rather than retry: a process blocked on a
 * semaphore has to remain killable, and re-running the array on a signal
 * would make it unkillable.
 *
 * ── wait_incr / wait_zero ───────────────────────────────────────────
 * Kept.  They are not the sleep mechanism — Bach uses them to decide WHO
 * to wake when a value changes, and that logic is sound.  A V operation
 * that takes the value to 0 wakes the zero-waiters; any V wakes the
 * increment-waiters.
 *
 * @version 3.0.0  @date 2026-09-30
 */
#include "../include/uiox_kix_sem.h"
#include "../include/uiox_kix_ipc_types.h"
#include "uiox_klibc.h"

/* Wall clock, maintained by 01_schedular's timekeeping. */
extern volatile uiox_uint64_t jiffies;

/* ── Semaphore set table ────────────────────────────────────────────── */
static SemSet sem_sets[IPC_MAX_SEM_SETS];

/* ── uiox_kix_sem_init ──────────────────────────────────────────────── */
void uiox_kix_sem_init(void)
{
    memset(sem_sets, 0, sizeof sem_sets);
}

/* ── Algorithm 9 — uiox_kix_sem_get ────────────────────────────────────────────
 * Find by key, or create a set of nsems semaphores.
 *
 * The key search runs first, so a second uiox_kix_sem_get on the same key returns
 * the existing set — which is how two unrelated processes reach it. */
int uiox_kix_sem_get(int key, int nsems, int flag)
{
    int i;

    for (i = 0; i < IPC_MAX_SEM_SETS; i++) {
        if (sem_sets[i].active && sem_sets[i].perm.key == key) {
            if (flag & IPC_EXCL) return -1;
            return i;
        }
    }

    if (!(flag & IPC_CREAT)) return -1;
    if (nsems <= 0 || nsems > IPC_MAX_SEMS) return -1;

    for (i = 0; i < IPC_MAX_SEM_SETS; i++) {
        if (!sem_sets[i].active) {
            sem_sets[i].active    = true;
            sem_sets[i].perm.key  = key;
            sem_sets[i].perm.mode = (uiox_uint16_t)(flag & 0x1FF);
            sem_sets[i].nsems     = nsems;
            sem_sets[i].ctime     = (time_t)jiffies;
            memset(sem_sets[i].sems, 0, sizeof sem_sets[i].sems);
            return i;
        }
    }
    return -1;   /* no free slots */
}

/* ── Algorithm 10 — uiox_kix_sem_ctl ───────────────────────────────────────────
 * GETVAL/SETVAL act on one semaphore; GETALL/SETALL on the whole set.
 *
 * IPC_RMID removes the set AND WAKES ITS WAITERS.  Without the wakeup a
 * process blocked on a semaphore that no longer exists would wait
 * forever — the same defect uiox_kix_msg_ctl's IPC_RMID had. */
int uiox_kix_sem_ctl(int semid, int semnum, int cmd, int val)
{
    int i;
    SemSet *ss;

    if (semid < 0 || semid >= IPC_MAX_SEM_SETS) return -1;
    if (!sem_sets[semid].active) return -1;

    ss = &sem_sets[semid];

    switch (cmd) {
    case GETVAL:
        if (semnum < 0 || semnum >= ss->nsems) return -1;
        return ss->sems[semnum].val;

    case SETVAL:
        if (semnum < 0 || semnum >= ss->nsems) return -1;
        ss->sems[semnum].val = val;
        ss->ctime = (time_t)jiffies;
        /* A raised value may satisfy an increment-waiter. */
        ipc_wakeup(IPC_WCHAN_SEM_INCR(&ss->sems[semnum]));
        return 0;

    case GETALL:
        for (i = 0; i < ss->nsems; i++)
            ;   /* a real build reports each value to the caller */
        return 0;

    case SETALL:
        for (i = 0; i < ss->nsems; i++) {
            ss->sems[i].val = val;
            ipc_wakeup(IPC_WCHAN_SEM_INCR(&ss->sems[i]));
        }
        ss->ctime = (time_t)jiffies;
        return 0;

    case IPC_RMID:
        /* Release every waiter before the set disappears, so none is
         * left sleeping on a channel that will never be signalled. */
        for (i = 0; i < ss->nsems; i++) {
            ipc_wakeup(IPC_WCHAN_SEM_INCR(&ss->sems[i]));
            ipc_wakeup(IPC_WCHAN_SEM_ZERO(&ss->sems[i]));
        }
        memset(ss, 0, sizeof *ss);
        return 0;

    default:
        return -1;
    }
}

/* ── reverse_ops ─────────────────────────────────────────────────────
 * Undo the operations already applied, in REVERSE order.
 *
 * Bach: uiox_kix_sem_op is all-or-nothing.  If the third operation in an array
 * cannot proceed, the first two must be undone so the set looks as it
 * did before the call.  Reverse order matters when one semaphore appears
 * twice in the array — undoing forward would apply the two in the wrong
 * sequence and land on a different value. */
static void reverse_ops(SemSet *ss, SemBuf *ops, int done_count)
{
    int i;
    for (i = done_count - 1; i >= 0; i--) {
        Semaphore *s = &ss->sems[ops[i].sem_num];
        s->val -= ops[i].sem_op;
        ipc_wakeup(IPC_WCHAN_SEM_INCR(s));
    }
}

/* ── Algorithm 11 — uiox_kix_sem_op ────────────────────────────────────────────
 * Apply an array of operations atomically.
 *
 * Three operation kinds, from the sign of sem_op:
 *   > 0   V — release.  Always succeeds; may wake waiters.
 *   < 0   P — acquire.  Blocks while the value is short.
 *   == 0  Z — wait for the value to reach zero.
 *
 * ── the restart ─────────────────────────────────────────────────────
 * Bach restarts the WHOLE array after a block, not just the operation
 * that blocked.  That is what makes the array atomic: a process that
 * slept must re-validate every operation, because another process may
 * have changed any of them while it waited.
 *
 * The restart is a real loop now, with a real sleep inside it — see the
 * banner. */
int uiox_kix_sem_op(int semid, SemBuf *ops, int nops, uiox_kix_psa_proc_t *caller)
{
    SemSet *ss;
    int     i;

    if (semid < 0 || semid >= IPC_MAX_SEM_SETS) return -1;
    if (!sem_sets[semid].active) return -1;
    if (!ops || nops <= 0) return -1;

    ss = &sem_sets[semid];

restart:
    /* Validate every index before touching anything.  Doing it inside
     * the apply loop would leave the earlier operations applied when a
     * later one is rejected. */
    for (i = 0; i < nops; i++) {
        if (ops[i].sem_num < 0 || ops[i].sem_num >= ss->nsems) return -1;
    }

    {
        int done = 0;

        for (i = 0; i < nops; i++) {
            Semaphore *s  = &ss->sems[ops[i].sem_num];
            int        op = ops[i].sem_op;

            if (op > 0) {
                /* V — release. */
                s->val += op;
                ipc_wakeup(IPC_WCHAN_SEM_INCR(s));
                if (s->val == 0) ipc_wakeup(IPC_WCHAN_SEM_ZERO(s));
                done++;

            } else if (op < 0) {
                /* P — acquire. */
                if (s->val >= -op) {
                    s->val += op;    /* op is negative */
                    done++;
                } else {
                    int rc;

                    reverse_ops(ss, ops, done);

                    if (ops[i].sem_flg & IPC_NOWAIT) return -1;

                    s->wait_incr++;
                    rc = ipc_sleep(caller, IPC_WCHAN_SEM_INCR(s));
                    s->wait_incr--;

                    /* -1 means an uncaught signal ended the wait.  The
                     * syscall must unwind — retrying would make a
                     * process blocked here unkillable. */
                    if (rc < 0) return -1;

                    goto restart;
                }

            } else {
                /* Z — wait for zero. */
                if (s->val != 0) {
                    int rc;

                    reverse_ops(ss, ops, done);

                    if (ops[i].sem_flg & IPC_NOWAIT) return -1;

                    s->wait_zero++;
                    rc = ipc_sleep(caller, IPC_WCHAN_SEM_ZERO(s));
                    s->wait_zero--;

                    if (rc < 0) return -1;

                    goto restart;
                }
                done++;
            }
        }
    }

    ss->otime = (time_t)jiffies;
    if (caller) {
        for (i = 0; i < nops; i++)
            ss->sems[ops[i].sem_num].last_pid = (int)caller->p_pid;
    }
    return 0;
}
