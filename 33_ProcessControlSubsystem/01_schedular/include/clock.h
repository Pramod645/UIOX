#ifndef UIOX_CLOCK_H
#define UIOX_CLOCK_H

#include "sched_types.h"

/* ─────────────────────────────────────────────────────────────
 * Callout table — deferred kernel function calls
 * Entries are sorted by delta_ticks (time until fire).
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    Callout entries[MAX_CALLOUTS];
    int     count;
} CalloutTable;

/* ─────────────────────────────────────────────────────────────
 * Clock subsystem API
 * ───────────────────────────────────────────────────────────── */
void clock_init(void);

/*
 * Algorithm clock  (§2 in source)
 *
 * Invoked on every hardware timer interrupt (each tick):
 *  1. Restart clock (re-arm hardware timer)
 *  2. Process callout table
 *  3. Collect kernel/user profiling samples
 *  4. Gather system and per-process statistics
 *  5. Adjust CPU utilisation measure
 *  6. Every ~1 second: adjust alarms, priorities, wake swapper
 */
void clock_tick(void);

/* Register a deferred callout to fire after 'ticks' ticks */
int  callout_add(int64_t ticks, void (*fn)(void *), void *arg);

/* Cancel a callout by index */
void callout_cancel(int idx);

/* Expose global load average */
LoadAvg *get_load_avg(void);

/* Expose callout table (debug) */
CalloutTable *get_callout_table(void);

#endif /* UIOX_CLOCK_H */
