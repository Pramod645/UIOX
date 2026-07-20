#include "clock.h"
#include "scheduler.h"
#include "profiler.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ─────────────────────────────────────────────────────────────
 * Module-private state
 * ───────────────────────────────────────────────────────────── */
static CalloutTable  callout_table;
static LoadAvg       load_avg;
static uint64_t      last_second_tick = 0;  /* jiffies at last 1-s update */
static uint64_t      total_ticks      = 0;  /* total clock interrupts seen */

/* EWMA decay constants (approximating 1-min, 5-min, 15-min) */
#define LOAD_ALPHA_1   0.9200   /* exp(-1/60)  */
#define LOAD_ALPHA_5   0.9835   /* exp(-1/300) */
#define LOAD_ALPHA_15  0.9945   /* exp(-1/900) */

/* ─────────────────────────────────────────────────────────────
 * clock_init
 * ───────────────────────────────────────────────────────────── */
void clock_init(void)
{
    memset(&callout_table, 0, sizeof callout_table);
    memset(&load_avg,      0, sizeof load_avg);
    last_second_tick = 0;
    total_ticks      = 0;
    printf("[clock] init: HZ=%d  TICK_NSEC=%lu  LATCH=%d\n",
           HZ, (unsigned long)TICK_NSEC, LATCH);
}

/* ─────────────────────────────────────────────────────────────
 * Internal: step 2 — process callout table
 * Each entry holds a delta (ticks remaining until it fires).
 * On each tick we decrement the first active entry; when it
 * reaches zero we invoke its function and scan forward.
 * ───────────────────────────────────────────────────────────── */
static void process_callouts(void)
{
    if (callout_table.count == 0) return;

    /* Decrement the head entry */
    for (int i = 0; i < MAX_CALLOUTS; i++) {
        Callout *c = &callout_table.entries[i];
        if (!c->active) continue;
        c->delta_ticks--;
        if (c->delta_ticks <= 0) {
            printf("  [callout] firing entry %d\n", i);
            c->fn(c->arg);
            c->active = false;
            callout_table.count--;
        }
        break; /* only one head entry decremented per tick */
    }
}

/* ─────────────────────────────────────────────────────────────
 * Internal: step 6 — per-second work
 * ───────────────────────────────────────────────────────────── */
static void per_second_work(void)
{
    RunQueue *rq = get_run_queue();

    /* Count active (running + uninterruptible) processes */
    int active = 0;
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++)
        for (Process *p = rq->heads[q]; p; p = p->next)
            if (p->state == TASK_RUNNING ||
                p->state == TASK_UNINTERRUPTIBLE) active++;

    /* Update load averages (EWMA) */
    load_avg.load_1  = LOAD_ALPHA_1  * load_avg.load_1  + (1.0 - LOAD_ALPHA_1)  * active;
    load_avg.load_5  = LOAD_ALPHA_5  * load_avg.load_5  + (1.0 - LOAD_ALPHA_5)  * active;
    load_avg.load_15 = LOAD_ALPHA_15 * load_avg.load_15 + (1.0 - LOAD_ALPHA_15) * active;

    printf("  [clock/1s] load: %.2f %.2f %.2f  active=%d\n",
           load_avg.load_1, load_avg.load_5, load_avg.load_15, active);

    /* Walk all processes: adjust alarms, CPU utilisation, priority */
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (Process *p = rq->heads[q]; p; p = p->next) {

            /* Adjust alarm */
            if (p->alarm_active && jiffies >= p->alarm_expire) {
                printf("  [clock/1s] ALARM fired for pid=%d\n", p->pid);
                p->alarm_active = false;
            }

            /* Decay CPU utilisation: shift right by 1 each second */
            p->cpu_usage = (uint8_t)(p->cpu_usage >> 1);

            /* Recalculate priority for user-mode processes */
            if (p->state == TASK_RUNNING)
                recalculate_priority(p);
        }
    }

    /* Readjust all run-queue priorities */
    readjust_all_priorities();

    /* In a real kernel: wake the swapper if memory is low */
    printf("  [clock/1s] swapper wakeup check complete\n");
}

/* ─────────────────────────────────────────────────────────────
 * Algorithm clock  (§2 in source)
 * ───────────────────────────────────────────────────────────── */
void clock_tick(void)
{
    /* Step 1: restart clock (re-arm hardware; simulated here) */
    total_ticks++;
    jiffies++;
    jiffies_64++;

    /* Update xtime by one tick */
    xtime.tv_nsec += TICK_NSEC;
    if (xtime.tv_nsec >= 1000000000UL) {
        xtime.tv_nsec -= 1000000000UL;
        xtime.tv_sec++;
    }

    /* Step 2: callout table */
    process_callouts();

    /* Step 3 & 4: profiling and statistics (delegated) */
    profiler_tick(/*kernel_pc=*/0xC000CAFE, /*user_pc=*/0x00401234);

    /* Step 5: per-process CPU tick */
    /* (In simulation we tick a representative "current" process) */

    /* Step 6: once per second */
    if (jiffies - last_second_tick >= (uint64_t)HZ) {
        last_second_tick = jiffies;
        per_second_work();
    }
}

/* ─────────────────────────────────────────────────────────────
 * callout_add
 * ───────────────────────────────────────────────────────────── */
int callout_add(int64_t ticks, void (*fn)(void *), void *arg)
{
    for (int i = 0; i < MAX_CALLOUTS; i++) {
        if (!callout_table.entries[i].active) {
            callout_table.entries[i].delta_ticks = ticks;
            callout_table.entries[i].fn          = fn;
            callout_table.entries[i].arg         = arg;
            callout_table.entries[i].active      = true;
            callout_table.count++;
            printf("[clock] callout registered: idx=%d  ticks=%ld\n",
                   i, (long)ticks);
            return i;
        }
    }
    fprintf(stderr, "[clock] callout table full\n");
    return -1;
}

/* ─────────────────────────────────────────────────────────────
 * callout_cancel
 * ───────────────────────────────────────────────────────────── */
void callout_cancel(int idx)
{
    if (idx < 0 || idx >= MAX_CALLOUTS) return;
    if (callout_table.entries[idx].active) {
        callout_table.entries[idx].active = false;
        callout_table.count--;
        printf("[clock] callout %d cancelled\n", idx);
    }
}

LoadAvg      *get_load_avg(void)       { return &load_avg;      }
CalloutTable *get_callout_table(void)  { return &callout_table; }
