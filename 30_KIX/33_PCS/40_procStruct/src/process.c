#include "../include/process.h"
#include "../include/proc_algo.h"
#include <string.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────────── */
proc_t  proc_table[NPROC];
proc_t *current_proc   = NULL;
proc_t *run_queue_head = NULL;

/* ── proc_alloc ─────────────────────────────────────────────
 * Find a free slot in the process table and initialise it.
 * Called during fork(). New process enters PROC_CREATED state.
 */
proc_t *proc_alloc(void)
{
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].p_state == PROC_UNUSED) {
            memset(&proc_table[i], 0, sizeof(proc_t));
            proc_table[i].p_state = PROC_CREATED;
            proc_table[i].p_pid   = (uint32_t)(i + 1);
            printf("[proc_alloc] allocated pid=%u\n",
                   proc_table[i].p_pid);
            return &proc_table[i];
        }
    }
    fprintf(stderr, "[proc_alloc] process table full\n");
    return NULL;
}

/* ── proc_free ──────────────────────────────────────────────
 * Return a process table slot to free pool.
 */
void proc_free(proc_t *p)
{
    if (!p) return;
    printf("[proc_free] freeing pid=%u\n", p->p_pid);
    memset(p, 0, sizeof(proc_t));
    p->p_state = PROC_UNUSED;
}

/* ── proc_find ──────────────────────────────────────────────
 * Locate a process by PID.
 */
proc_t *proc_find(uint32_t pid)
{
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].p_state != PROC_UNUSED &&
            proc_table[i].p_pid == pid)
            return &proc_table[i];
    }
    return NULL;
}

/* ── proc_set_state ─────────────────────────────────────────
 * Transition a process to a new state.
 * Enforces valid state transitions per the Unix state diagram.
 */
void proc_set_state(proc_t *p, proc_state_t new_state)
{
    if (!p) return;

    proc_state_t old = p->p_state;

    /* Basic transition validity check */
    int valid = 0;
    switch (old) {
    case PROC_USER_RUNNING:
        valid = (new_state == PROC_KERNEL_RUNNING ||
                 new_state == PROC_PREEMPTED);
        break;
    case PROC_KERNEL_RUNNING:
        valid = (new_state == PROC_USER_RUNNING   ||
                 new_state == PROC_PREEMPTED       ||
                 new_state == PROC_SLEEP_MEM       ||
                 new_state == PROC_READY           ||
                 new_state == PROC_ZOMBIE);
        break;
    case PROC_READY:
        valid = (new_state == PROC_KERNEL_RUNNING  ||
                 new_state == PROC_READY_SWAPPED);
        break;
    case PROC_SLEEP_MEM:
        valid = (new_state == PROC_READY           ||
                 new_state == PROC_SLEEP_SWAPPED);
        break;
    case PROC_READY_SWAPPED:
        valid = (new_state == PROC_READY);
        break;
    case PROC_SLEEP_SWAPPED:
        valid = (new_state == PROC_READY_SWAPPED);
        break;
    case PROC_PREEMPTED:
        valid = (new_state == PROC_USER_RUNNING    ||
                 new_state == PROC_READY);
        break;
    case PROC_CREATED:
        valid = (new_state == PROC_READY           ||
                 new_state == PROC_READY_SWAPPED);
        break;
    case PROC_ZOMBIE:
        valid = 0;   /* zombie is terminal */
        break;
    default:
        valid = 1;
        break;
    }

    if (!valid) {
        fprintf(stderr,
                "[proc_set_state] invalid transition %d -> %d "
                "for pid=%u\n", old, new_state, p->p_pid);
        return;
    }

    printf("[proc_set_state] pid=%u  state %d -> %d\n",
           p->p_pid, old, new_state);
    p->p_state = new_state;

    /* Update flags when swapped */
    if (new_state == PROC_READY_SWAPPED ||
        new_state == PROC_SLEEP_SWAPPED)
        p->p_flag |=  P_SWAPPED;
    else
        p->p_flag &= ~P_SWAPPED;
}

/* ── proc_signal_pending ────────────────────────────────────
 * Return non-zero if any unblocked signal is pending.
 */
int proc_signal_pending(proc_t *p)
{
    if (!p) return 0;
    return (int)(p->p_sig & ~p->p_sigmask);
}

/* ── sched_enqueue ──────────────────────────────────────────
 * Add a process to the run queue (sorted by priority).
 */
void sched_enqueue(proc_t *p)
{
    if (!p) return;
    p->p_next = NULL;
    p->p_prev = NULL;

    if (!run_queue_head) {
        run_queue_head = p;
        return;
    }

    /* Insert in priority order (lower p_pri value = higher priority) */
    proc_t *cur  = run_queue_head;
    proc_t *prev = NULL;

    while (cur && cur->p_sched.p_pri <= p->p_sched.p_pri) {
        prev = cur;
        cur  = cur->p_next;
    }
    if (!prev) {
        p->p_next      = run_queue_head;
        run_queue_head->p_prev = p;
        run_queue_head = p;
    } else {
        p->p_next    = cur;
        p->p_prev    = prev;
        prev->p_next = p;
        if (cur) cur->p_prev = p;
    }
}

/* ── sched_pick ─────────────────────────────────────────────
 * Pick the highest-priority runnable process.
 */
proc_t *sched_pick(void)
{
    if (!run_queue_head) return NULL;
    proc_t *p    = run_queue_head;
    run_queue_head = p->p_next;
    if (run_queue_head)
        run_queue_head->p_prev = NULL;
    p->p_next = p->p_prev = NULL;
    return p;
}
