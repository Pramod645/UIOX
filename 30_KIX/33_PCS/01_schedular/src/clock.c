/*
 * 30_KIX/33_PCS/01_schedular/src/clock.c
 *
 * Fixes applied (v2.0)
 * ────────────────────
 * 1. Removed system headers:
 *      #include <stdio.h>    → uiox_printf via uiox_klibc.h (flows through clock.h)
 *      #include <string.h>   → uiox_memset via uiox_klibc.h
 *      #include <math.h>     → removed; EWMA now integer fixed-point (no FPU)
 *
 * 2. LOAD_ALPHA_* double constants removed.
 *    LoadAvg fields are uint32_t scaled ×1000 (matching updated sched_types.h).
 *    EWMA formula: new = (alpha_num * old + (1000 - alpha_num) * sample) / 1000
 *      alpha_1  = 920  (≈ exp(-1/60)  × 1000)
 *      alpha_5  = 983  (≈ exp(-1/300) × 1000)
 *      alpha_15 = 994  (≈ exp(-1/900) × 1000)
 *
 * 3. fflags(stderr, ...) replaced with uiox_printf (no stderr in freestanding).
 *
 * 4. No other logic changed — all algorithms identical to original.
 */

 #include "../include/clock.h"
 #include "../include/scheduler.h"
 #include "../include/profiler.h"
 /* stdio.h / string.h / math.h removed — provided by uiox_klibc.h
    which flows in through clock.h → sched_types.h → uiox_klibc.h    */
 
 /* ── Module-private state ───────────────────────────────────────────── */
 static CalloutTable  callout_table;
 static LoadAvg       load_avg;
 static uint64_t      last_second_tick = 0;
 static uint64_t      total_ticks      = 0;
 
 /* ── Integer EWMA decay factors (×1000, replaces double LOAD_ALPHA_*) ─
  *   alpha_1  = 920  ≈ exp(-1/60)  × 1000
  *   alpha_5  = 983  ≈ exp(-1/300) × 1000
  *   alpha_15 = 994  ≈ exp(-1/900) × 1000
  *
  *   Update formula (no FPU):
  *     new_load = (alpha * old_load + (1000 - alpha) * active_count) / 1000
  * ────────────────────────────────────────────────────────────────────── */
 #define LOAD_ALPHA_1   920u
 #define LOAD_ALPHA_5   983u
 #define LOAD_ALPHA_15  994u
 
 static uint32_t ewma_update(uint32_t alpha, uint32_t old_val, uint32_t sample)
 {
     return (uint32_t)((alpha * (uint64_t)old_val +
                        (1000u - alpha) * (uint64_t)sample) / 1000u);
 }
 
 /* ── clock_init ─────────────────────────────────────────────────────── */
 void clock_init(void)
 {
     memset(&callout_table, 0, sizeof callout_table);
     memset(&load_avg,      0, sizeof load_avg);
     last_second_tick = 0;
     total_ticks      = 0;
     printf("[clock] init: HZ=%d  TICK_NSEC=%lu  LATCH=%d\n",
            HZ, (unsigned long)TICK_NSEC, LATCH);
 }
 
 /* ── process_callouts — fire any entries whose delta_ticks reached 0 ── */
 static void process_callouts(void)
 {
     int i;
     for (i = 0; i < MAX_CALLOUTS; i++) {
         Callout *c = &callout_table.entries[i];
         if (!c->active) continue;
         c->delta_ticks--;
         if (c->delta_ticks <= 0) {
             c->active = false;
             callout_table.count--;
             if (c->fn) c->fn(c->arg);
         }
     }
 }
 
 /* ── update_load_avg — called once per second ───────────────────────── */
 static void update_load_avg(void)
 {
     uint32_t active = 0;
     int q;
     RunQueue *rq = get_run_queue();
 
     /* Count runnable + uninterruptible processes */
     for (q = 0; q < MAX_PRIORITY_QUEUES; q++) {
         for (Process *p = rq->heads[q]; p; p = p->next) {
             if (p->state == TASK_RUNNING ||
                 p->state == TASK_UNINTERRUPTIBLE) active++;
         }
     }
 
     /* Integer EWMA update — no FPU */
     load_avg.load_1  = ewma_update(LOAD_ALPHA_1,  load_avg.load_1,  active);
     load_avg.load_5  = ewma_update(LOAD_ALPHA_5,  load_avg.load_5,  active);
     load_avg.load_15 = ewma_update(LOAD_ALPHA_15, load_avg.load_15, active);
 
     printf("[clock/1s] load: %u.%03u  %u.%03u  %u.%03u  active=%u\n",
            load_avg.load_1  / 1000u, load_avg.load_1  % 1000u,
            load_avg.load_5  / 1000u, load_avg.load_5  % 1000u,
            load_avg.load_15 / 1000u, load_avg.load_15 % 1000u,
            active);
 
     /* Adjust priorities for user-mode processes */
     {
         RunQueue *rq2 = get_run_queue();
         for (q = 0; q < MAX_PRIORITY_QUEUES; q++) {
             for (Process *p = rq2->heads[q]; p; p = p->next) {
                 if (p->state == TASK_RUNNING)
                     recalculate_priority(p);
             }
         }
     }
     readjust_all_priorities();
     printf("  [clock/1s] swapper wakeup check complete\n");
 }
 
 /* ── clock_tick — Algorithm clock (§2), called on every timer IRQ ───── */
 void clock_tick(void)
 {
     /* Step 1: restart clock (re-arm hardware; simulated here) */
     total_ticks++;
     jiffies++;
     jiffies_64++;
 
     /* Update xtime by one tick */
     xtime.tv_nsec += (uint32_t)TICK_NSEC;
     if (xtime.tv_nsec >= 1000000000UL) {
         xtime.tv_nsec -= 1000000000UL;
         xtime.tv_sec++;
     }
 
     /* Step 2: process callout table */
     process_callouts();
 
     /* Step 3: profiler tick */
     profiler_tick(0, 0);
 
     /* Step 4 & 5: per-process accounting + CPU utilisation */
     {
         RunQueue *rq = get_run_queue();
         int q;
         for (q = 0; q < MAX_PRIORITY_QUEUES; q++) {
             for (Process *p = rq->heads[q]; p; p = p->next) {
                 p->times.tms_stime++;
                 if (p->cpu_usage < 255u) p->cpu_usage++;
             }
         }
     }
 
     /* Step 6: every ~1 second update load averages and alarms */
     if (jiffies - last_second_tick >= (uint64_t)HZ) {
         last_second_tick = jiffies;
         update_load_avg();
     }
 }
 
 /* ── callout_add — register a deferred function call ───────────────── */
 int callout_add(int64_t ticks, void (*fn)(void *), void *arg)
 {
     int i;
     for (i = 0; i < MAX_CALLOUTS; i++) {
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
     /* No stderr in freestanding — use uiox_printf */
     printf("[clock] ERROR: callout table full\n");
     return -1;
 }
 
 /* ── callout_cancel — deactivate a registered callout ──────────────── */
 void callout_cancel(int idx)
 {
     if (idx < 0 || idx >= MAX_CALLOUTS) return;
     if (callout_table.entries[idx].active) {
         callout_table.entries[idx].active = false;
         callout_table.count--;
         printf("[clock] callout %d cancelled\n", idx);
     }
 }
 