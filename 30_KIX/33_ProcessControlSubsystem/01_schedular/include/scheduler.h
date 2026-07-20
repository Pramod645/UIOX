#ifndef UIOX_SCHEDULER_H
#define UIOX_SCHEDULER_H

#include "sched_types.h"

/* ─────────────────────────────────────────────────────────────
 * Multilevel feedback run queue
 * MAX_PRIORITY_QUEUES queues; queue 0 = highest priority.
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    Process *heads[MAX_PRIORITY_QUEUES];
    int      count;                  /* total processes in queue   */
} RunQueue;

/* ─────────────────────────────────────────────────────────────
 * Scheduler API
 * ───────────────────────────────────────────────────────────── */

/* Initialise the run queue and process table */
void  scheduler_init(void);

/*
 * Algorithm schedule_process (§1 in source)
 *
 * While no process is picked:
 *   scan all queues, highest priority first;
 *   idle if nothing eligible;
 * Remove chosen process, context-switch to it.
 */
Process *schedule_process(void);

/* Add a process to the appropriate priority queue */
void  enqueue_process(Process *p);

/* Remove a specific process from the run queue */
void  dequeue_process(Process *p);

/*
 * Recalculate dynamic priority after return from kernel mode.
 * priority = max(0, static_priority - cpu_bonus + nice_penalty)
 */
void  recalculate_priority(Process *p);

/*
 * Periodically readjust all TASK_RUNNING / ready processes.
 * Called once per second by the clock interrupt handler.
 */
void  readjust_all_priorities(void);

/*
 * Called at every tick: decrement the running process's time slice.
 * Returns true if the slice is exhausted (preemption needed).
 */
bool  tick_process(Process *running);

/* Fair-share: distribute CPU fairly within a group */
void  fair_share_adjust(int group_id);

/* Dump queue state (debug) */
void  scheduler_print(void);

/* Access the global run queue */
RunQueue *get_run_queue(void);

#endif /* UIOX_SCHEDULER_H */
