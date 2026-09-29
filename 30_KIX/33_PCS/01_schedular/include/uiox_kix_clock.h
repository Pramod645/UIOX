/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_clock.h
 *
 * The clock — Bach's Algorithm 2 (clock_tick).
 *
 *   input: none
 *   output: none
 *   {
 *       restart clock;        //so that it will interrupt again
 *       if (callout table not empty)
 *       {
 *           adjust callout times;
 *           schedule callout function if time elapsed;
 *       }
 *       if (kernel profiling on)
 *           note program counter at time of interrupt;
 *       if (user profiling on)
 *           note program counter at time of interrupt;
 *       gather system statistics;
 *       gather statistics per process;
 *       adjust measure of process CPU utilization;
 *       if (1 second or more since last here and interrupt not in
 *           critical region of code)
 *       {
 *           for (all processes in the system)
 *           {
 *               adjust alarm time if active;
 *               adjust measure of CPU utilization;
 *               if (process to execute in user mode)
 *                   adjust process priority;
 *           }
 *           wakeup swapper process if necessary;
 *       }
 *   }
 *
 * Every step above has a counterpart in clock.c, and the per-second
 * block is why the callout table is here rather than in timer.c: the
 * clock OWNS the callout mechanism, and the waker that finally ends a
 * proc_sleep is one of its callouts.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#ifndef UIOX_CLOCK_H
#define UIOX_CLOCK_H

#include "sched_types.h"

/* ── Callout table — deferred kernel function calls ──────────────────
 * MAX_CALLOUTS entries, each with a tick delta rather than an absolute
 * deadline: "adjust callout times" is a decrement per tick. */
typedef struct {
    Callout entries[MAX_CALLOUTS];
    int     count;
} CalloutTable;

/* ── Clock API ─────────────────────────────────────────────────────── */

/* Zero the callout table and the load averages. */
void clock_init(void);

/* Algorithm clock — called on EVERY hardware timer interrupt.
 * Implements the seven steps in the banner above, in order. */
void clock_tick(void);

/* Register a deferred callout to fire after `ticks` ticks.
 * Returns the slot index, or -1 when the table is full.  This is the
 * mechanism an alarm and a timed sleep both build on. */
int  callout_add(int64_t ticks, void (*fn)(void *), void *arg);

/* Cancel a callout by the index callout_add returned. */
void callout_cancel(int idx);

/* The load averages, for a reader that wants them without calling
 * update_load_avg. */
LoadAvg *get_load_avg(void);

/* The callout table, for a diagnostic. */
CalloutTable *get_callout_table(void);

#endif /* UIOX_CLOCK_H */
