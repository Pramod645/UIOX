#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 * Global state
 * ───────────────────────────────────────────────────────────── */
static RunQueue   rq;
static Process    proc_table[MAX_PROCESSES];
static int        proc_count = 0;
static Process   *current   = NULL;  /* running process          */

volatile uint64_t jiffies    = 0;
volatile uint64_t jiffies_64 = 0;
XTime             xtime      = {0, 0};

/* ─────────────────────────────────────────────────────────────
 * Priority → queue mapping
 * dynamic_priority 0..27  → queue 0 (highest)
 * dynamic_priority 28..55 → queue 1
 * …
 * ───────────────────────────────────────────────────────────── */
static int priority_to_queue(int prio)
{
    int band = MAX_PRIORITY / MAX_PRIORITY_QUEUES;
    int q    = prio / band;
    if (q >= MAX_PRIORITY_QUEUES) q = MAX_PRIORITY_QUEUES - 1;
    return q;
}

/* ─────────────────────────────────────────────────────────────
 * scheduler_init
 * ───────────────────────────────────────────────────────────── */
void scheduler_init(void)
{
    memset(&rq, 0, sizeof rq);
    proc_count = 0;
    current    = NULL;
    printf("[scheduler] initialised  queues=%d  HZ=%d\n",
           MAX_PRIORITY_QUEUES, HZ);
}

/* ─────────────────────────────────────────────────────────────
 * enqueue_process
 * ───────────────────────────────────────────────────────────── */
void enqueue_process(Process *p)
{
    if (!p || p->state != TASK_RUNNING || !p->in_memory) return;

    int q      = priority_to_queue(p->dynamic_priority);
    p->next    = rq.heads[q];
    rq.heads[q] = p;
    rq.count++;
    printf("[scheduler] enqueue pid=%d  prio=%d  queue=%d\n",
           p->pid, p->dynamic_priority, q);
}

/* ─────────────────────────────────────────────────────────────
 * dequeue_process
 * ───────────────────────────────────────────────────────────── */
void dequeue_process(Process *p)
{
    if (!p) return;
    int q    = priority_to_queue(p->dynamic_priority);
    Process *prev = NULL, *cur = rq.heads[q];

    while (cur) {
        if (cur == p) {
            if (prev) prev->next = cur->next;
            else      rq.heads[q] = cur->next;
            cur->next = NULL;
            rq.count--;
            return;
        }
        prev = cur; cur = cur->next;
    }
}

/* ─────────────────────────────────────────────────────────────
 * recalculate_priority
 *
 * Simple model:
 *   dynamic_priority = clamp(static_priority
 *                            - cpu_bonus
 *                            + nice_adjustment, 0, MAX_PRIORITY-1)
 *
 * Processes that use more CPU get a higher (worse) dynamic
 * priority number, making them less likely to be chosen next.
 * ───────────────────────────────────────────────────────────── */
void recalculate_priority(Process *p)
{
    if (!p) return;

    /* cpu_bonus: the more CPU used, the less bonus (0..20) */
    int cpu_bonus = (int)(20 - (p->cpu_usage * 20) / 255);

    /* nice maps -20..+19 → penalty of -20..+19 */
    int new_prio  = p->static_priority - cpu_bonus + p->nice;

    if (new_prio < 0)               new_prio = 0;
    if (new_prio >= MAX_PRIORITY)   new_prio = MAX_PRIORITY - 1;

    p->dynamic_priority = new_prio;
    printf("[scheduler] pid=%d  priority recalc: static=%d cpu_bonus=%d "
           "nice=%d → dynamic=%d\n",
           p->pid, p->static_priority, cpu_bonus, p->nice, new_prio);
}

/* ─────────────────────────────────────────────────────────────
 * readjust_all_priorities  (called once per second)
 * ───────────────────────────────────────────────────────────── */
void readjust_all_priorities(void)
{
    printf("[scheduler] readjusting all priorities\n");
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (Process *p = rq.heads[q]; p; p = p->next) {
            if (p->state == TASK_RUNNING)
                recalculate_priority(p);
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm schedule_process  (§1 in source)
 * ───────────────────────────────────────────────────────────── */
Process *schedule_process(void)
{
    Process *chosen = NULL;

    /*
     * Outer loop: keep trying until a process is picked.
     * In a real kernel this is driven by interrupt wakeups;
     * here we break after one idle pass for simulation.
     */
    while (!chosen) {
        /* Scan queues from highest priority (0) downward */
        for (int q = 0; q < MAX_PRIORITY_QUEUES && !chosen; q++) {
            for (Process *p = rq.heads[q]; p; p = p->next) {
                if (p->state == TASK_RUNNING && p->in_memory) {
                    chosen = p;
                    break;
                }
            }
        }

        if (!chosen) {
            printf("[scheduler] no eligible process — machine idle\n");
            /* In simulation, break; a real kernel halts the CPU here */
            break;
        }
    }

    if (chosen) {
        dequeue_process(chosen);
        chosen->time_slice = TIME_QUANTUM;
        current = chosen;
        printf("[scheduler] context switch → pid=%d  prio=%d  slice=%lu\n",
               chosen->pid, chosen->dynamic_priority,
               (unsigned long)chosen->time_slice);
    }

    return chosen;
}

/* ─────────────────────────────────────────────────────────────
 * tick_process
 * Decrement time slice of running process.
 * Returns true when slice is exhausted → preempt.
 * ───────────────────────────────────────────────────────────── */
bool tick_process(Process *running)
{
    if (!running) return false;

    running->total_ticks++;
    running->times.tms_utime++;

    /* Decay CPU usage: exponential moving average */
    running->cpu_usage = (uint8_t)((running->cpu_usage * 7 + 255) / 8);

    if (running->time_slice > 0) running->time_slice--;

    if (running->time_slice == 0) {
        printf("[scheduler] pid=%d time slice expired → preempting\n",
               running->pid);
        /* Re-enqueue at lower priority (feedback) */
        if (running->dynamic_priority < MAX_PRIORITY - 1)
            running->dynamic_priority++;
        enqueue_process(running);
        return true; /* caller should call schedule_process() */
    }
    return false;
}

/* ─────────────────────────────────────────────────────────────
 * fair_share_adjust
 * Penalise processes in a group that over-consumed CPU.
 * ───────────────────────────────────────────────────────────── */
void fair_share_adjust(int group_id)
{
    uint64_t group_total = 0;
    int      group_size  = 0;

    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (Process *p = rq.heads[q]; p; p = p->next) {
            if (p->group_id == group_id) {
                group_total += p->total_ticks;
                group_size++;
            }
        }
    }
    if (!group_size) return;

    uint64_t avg = group_total / (uint64_t)group_size;
    printf("[scheduler] fair-share group %d: avg_ticks=%lu\n",
           group_id, (unsigned long)avg);

    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (Process *p = rq.heads[q]; p; p = p->next) {
            if (p->group_id == group_id) {
                if (p->total_ticks > avg &&
                    p->dynamic_priority < MAX_PRIORITY - 1)
                    p->dynamic_priority++;  /* penalise over-users */
                else if (p->total_ticks < avg && p->dynamic_priority > 0)
                    p->dynamic_priority--;  /* reward under-users  */
            }
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 * scheduler_print
 * ───────────────────────────────────────────────────────────── */
void scheduler_print(void)
{
    printf("[scheduler] run queue (total=%d):\n", rq.count);
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        if (!rq.heads[q]) continue;
        printf("  queue %d: ", q);
        for (Process *p = rq.heads[q]; p; p = p->next)
            printf("pid=%d(prio=%d) ", p->pid, p->dynamic_priority);
        printf("\n");
    }
}

RunQueue *get_run_queue(void) { return &rq; }
