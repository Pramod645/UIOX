#include "../include/proc_algo.h"
#include "../include/process.h"
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

/* ── Globals ────────────────────────────────────────────────── */
sleep_queue_t sleep_hash[SLEEP_HASH_SZ];
int           scheduler_flag = 0;
int           proc_level     = 0;   /* processor interrupt level */

/* ── Hash function for sleep address ───────────────────────── */
static int sleep_hash_fn(uintptr_t wchan)
{
    return (int)((wchan >> 2) % SLEEP_HASH_SZ);
}

/* ── Interrupt level helpers (simulated) ────────────────────── */
static int splhigh(void)
{
    int old   = proc_level;
    proc_level = 15;    /* block all interrupts */
    return old;
}

static void splx(int old_level)
{
    proc_level = old_level;
}

/* ── Sleep queue operations ─────────────────────────────────── */
static void sleep_enqueue(proc_t *p, int bucket)
{
    sleep_queue_t *sq = &sleep_hash[bucket];
    p->p_next = NULL;
    p->p_prev = sq->sq_tail;
    if (sq->sq_tail) sq->sq_tail->p_next = p;
    else             sq->sq_head = p;
    sq->sq_tail = p;
}

static void sleep_dequeue(proc_t *p, int bucket)
{
    sleep_queue_t *sq = &sleep_hash[bucket];
    if (p->p_prev) p->p_prev->p_next = p->p_next;
    else           sq->sq_head = p->p_next;
    if (p->p_next) p->p_next->p_prev = p->p_prev;
    else           sq->sq_tail = p->p_prev;
    p->p_next = p->p_prev = NULL;
}

/* ─────────────────────────────────────────────────────────────
 * 10. Algorithm sleep
 *     input : sleep address (wchan), priority, interruptible flag
 *     output:
 *       0  — woken normally
 *       1  — woken by a caught signal
 *       longjmp if woken by uncaught signal
 */
int proc_sleep(uintptr_t wchan, int priority, int interruptible)
{
    if (!current_proc) return 0;

    proc_t *p = current_proc;

    /* Raise processor execution level to block all interrupts */
    int old_level = splhigh();

    /* Set process state to sleep */
    proc_set_state(p, PROC_SLEEP_MEM);

    /* Put process on sleep hash queue based on sleep address */
    int bucket = sleep_hash_fn(wchan);
    sleep_enqueue(p, bucket);

    /* Save sleep address in process table slot */
    p->p_wchan = wchan;

    /* Set process priority level to input priority */
    p->p_sched.p_pri = priority;

    printf("[sleep] pid=%u sleeping on wchan=0x%lx "
           "priority=%d interruptible=%d\n",
           p->p_pid, (unsigned long)wchan, priority, interruptible);

    if (!interruptible) {
        /* ── Uninterruptible sleep ───────────────────────────── */

        /* Do context switch — process resumes here on wakeup */
        proc_t *next = sched_pick();
        if (next) {
            proc_set_state(next, PROC_KERNEL_RUNNING);
            /* context_switch_proc(p, next); */
        }

        /* Reset processor priority level */
        splx(old_level);
        return 0;
    }

    /* ── Interruptible sleep ─────────────────────────────────── */

    if (!proc_signal_pending(p)) {
        /* No signal pending — do context switch */
        proc_t *next = sched_pick();
        if (next) {
            proc_set_state(next, PROC_KERNEL_RUNNING);
            /* context_switch_proc(p, next); */
        }

        /* Process resumes here when it wakes up */
        if (!proc_signal_pending(p)) {
            splx(old_level);
            printf("[sleep] pid=%u woke normally\n", p->p_pid);
            return 0;
        }
    }

    /* Signal is pending — remove from sleep hash queue if still there */
    if (p->p_wchan == wchan) {
        sleep_dequeue(p, bucket);
        p->p_wchan = 0;
    }

    /* Reset processor priority level */
    splx(old_level);

    /* If priority set to catch signals, return 1 */
    if (priority > PZERO) {
        printf("[sleep] pid=%u woke due to caught signal\n",
               p->p_pid);
        return 1;
    }

    /* Uncaught signal — do longjmp to abort system call */
    printf("[sleep] pid=%u woke due to uncaught signal, longjmp\n",
           p->p_pid);
    longjmp(u.u_qsave, 1);

    /* Never reached */
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * 11. Algorithm wakeup
 *     input : sleep address (wchan)
 *     output: none
 *
 *     Wake up all processes sleeping on the given address.
 */
void proc_wakeup(uintptr_t wchan)
{
    /* Raise processor execution level to block all interrupts */
    int old_level = splhigh();

    /* Find sleep hash queue for sleep address */
    int bucket = sleep_hash_fn(wchan);
    sleep_queue_t *sq = &sleep_hash[bucket];

    proc_t *p = sq->sq_head;

    /* For every process asleep on sleep address */
    while (p) {
        proc_t *next_p = p->p_next;

        if (p->p_wchan == wchan) {
            /* Remove process from hash queue */
            sleep_dequeue(p, bucket);

            /* Clear sleep address field in process table */
            p->p_wchan = 0;

            /* Mark process state: ready to run */
            if (p->p_flag & P_SWAPPED)
                proc_set_state(p, PROC_READY_SWAPPED);
            else
                proc_set_state(p, PROC_READY);

            printf("[wakeup] waking pid=%u from wchan=0x%lx\n",
                   p->p_pid, (unsigned long)wchan);

            if (p->p_flag & P_SWAPPED) {
                /* Process not in memory — wake up swapper (pid 0) */
                proc_t *swapper = proc_find(0);
                if (swapper &&
                    swapper->p_state != PROC_KERNEL_RUNNING) {
                    proc_set_state(swapper, PROC_READY);
                    sched_enqueue(swapper);
                    printf("[wakeup] woke swapper for pid=%u\n",
                           p->p_pid);
                }
            } else {
                /* Put process on scheduler run list */
                sched_enqueue(p);

                /* If awakened process more eligible than current */
                if (current_proc &&
                    p->p_sched.p_pri <
                    current_proc->p_sched.p_pri) {
                    scheduler_flag = 1;
                    printf("[wakeup] scheduler flag set, "
                           "pid=%u preempts pid=%u\n",
                           p->p_pid, current_proc->p_pid);
                }
            }
        }
        p = next_p;
    }

    /* Restore processor execution level */
    splx(old_level);
}
