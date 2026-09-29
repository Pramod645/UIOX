/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_clock.c
 *
 * Bach's Algorithm 2 — clock_tick.
 *
 * Ported to the 40_psa process type.  Two prefixes, one per owner:
 *   p->p   the embedded 40_psa entry (p_state, p_sched, p_timers)
 *   p->s   scheduler-private accounting (s_total_ticks, s_cpu_usage)
 *
 * ── the seven steps of Algorithm 2, and where each one is ───────────
 *   1. restart clock                  the arch timer re-arms; here the
 *                                     tick counters advance
 *   2. process callout table          process_callouts()
 *   3. kernel profiling sample        profiler_tick(0, 0)
 *   4. system statistics              update_load_avg(), once per second
 *   5. per-process statistics         the tick loop below
 *   6. CPU utilisation measure        the tick loop below
 *   7. per-second block               update_load_avg() + readjust
 *
 * ── why the callout table lives here and not in timer.c ─────────────
 * timer.c has the general dynamic-timer wheel.  The callout table is
 * different: it is the mechanism Bach's clock algorithm names, it uses
 * TICK DELTAS rather than absolute deadlines, and the waker that finally
 * ends a proc_sleep is one of its entries.  Keeping it in uiox_kix_clock.c puts
 * the waker next to the interrupt that fires it.
 *
 * ── the one step that is only simulated ─────────────────────────────
 * "wakeup swapper process if necessary" prints in the original and does
 * nothing.  That is honest here: the swapper is process 0, and whether
 * it needs waking depends on whether any READY_SWAPPED process exists —
 * a test that can be written, but only once 02_MemMngnt owns swap-in.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_clock.h"
#include "../include/uiox_kix_scheduler.h"
#include "../include/uiox_kix_profiler.h"

/* ── Module-private state ──────────────────────────────────────────── */
static CalloutTable  callout_table;
static LoadAvg       load_avg;
static uint64_t      last_second_tick = 0;
static uint64_t      total_ticks      = 0;

/* ── Integer EWMA decay factors (x1000, replaces double LOAD_ALPHA_*) ──
 *   alpha_1  = 920  ~ exp(-1/60)  x 1000
 *   alpha_5  = 983  ~ exp(-1/300) x 1000
 *   alpha_15 = 994  ~ exp(-1/900) x 1000
 *
 *   new_load = (alpha * old + (1000 - alpha) * sample) / 1000
 *
 * Integer because the x86_64 build carries -mno-sse and a double would
 * use registers that are not available there. */
#define LOAD_ALPHA_1   920u
#define LOAD_ALPHA_5   983u
#define LOAD_ALPHA_15  994u

static uint32_t ewma_update(uint32_t alpha, uint32_t old_val, uint32_t sample)
{
    return (uint32_t)((alpha * (uint64_t)old_val +
                       (1000u - alpha) * (uint64_t)sample) / 1000u);
}

/* ── clock_init ────────────────────────────────────────────────────── */
void clock_init(void)
{
    int i;
    for (i = 0; i < MAX_CALLOUTS; i++) {
        callout_table.entries[i].active      = false;
        callout_table.entries[i].delta_ticks = 0;
        callout_table.entries[i].fn          = (void (*)(void *))0;
        callout_table.entries[i].arg         = (void *)0;
    }
    callout_table.count = 0;

    load_avg.load_1  = 0u;
    load_avg.load_5  = 0u;
    load_avg.load_15 = 0u;

    last_second_tick = 0;
    total_ticks      = 0;
}

/* ── process_callouts — Algorithm 2 step 2 ───────────────────────────
 * "adjust callout times; schedule callout function if time elapsed".
 *
 * Delta ticks rather than absolute deadlines, so the adjust is a
 * decrement.  A callout fires when its delta reaches zero, and is
 * deactivated BEFORE the function runs so a callout that re-registers
 * itself cannot find its own slot still marked active.
 *
 * The fn is captured before the deactivation for the same reason: if
 * the function reuses this slot, reading c->fn after the call would
 * read the new one. */
static void process_callouts(void)
{
    int i;
    for (i = 0; i < MAX_CALLOUTS; i++) {
        Callout *c = &callout_table.entries[i];
        void   (*fn)(void *);

        if (!c->active) continue;

        c->delta_ticks--;
        if (c->delta_ticks > 0) continue;

        fn       = c->fn;
        c->active = false;
        callout_table.count--;

        if (fn) fn(c->arg);
    }
}

/* ── update_load_avg — Algorithm 2 steps 4 and 7 ─────────────────────
 * Counts runnable processes per second and feeds three EWMAs.  The
 * eligibility test is scheduler.h's, so the number matches what
 * schedule_process would actually pick from. */
static void update_load_avg(void)
{
    int      q;
    Process *p;
    int      q2;
    Process *p2;
    uint32_t active = 0;
    RunQueue *rq;

    rq = get_run_queue();

    for (q = 0; q < MAX_PRIORITY_QUEUES; q++)
        for (p = rq->heads[q]; p; p = (Process *)(void *)p->p.p_next)
            if (p->p.p_state == UIOX_KIX_PSA_PROC_READY && p->s_in_memory)
                active++;

    load_avg.load_1  = ewma_update(LOAD_ALPHA_1,  load_avg.load_1,  active);
    load_avg.load_5  = ewma_update(LOAD_ALPHA_5,  load_avg.load_5,  active);
    load_avg.load_15 = ewma_update(LOAD_ALPHA_15, load_avg.load_15, active);

    /* Step 7: "if (process to execute in user mode) adjust process
     * priority".  Only READY processes, because a sleeping one has no
     * slice to have used — the test lives in readjust_all_priorities. */
    for (q2 = 0; q2 < MAX_PRIORITY_QUEUES; q2++)
        for (p2 = rq->heads[q2]; p2; p2 = (Process *)(void *)p2->p.p_next)
            if (p2->p.p_state == UIOX_KIX_PSA_PROC_READY)
                recalculate_priority(p2);

    readjust_all_priorities();

    /* "wakeup swapper process if necessary" — see the banner.  The test
     * to write is: does any process sit in READY_SWAPPED? */
}

/* ── clock_tick — Algorithm 2, called on EVERY timer interrupt ─────── */
void clock_tick(void)
{
    int      q;
    Process *p;
    RunQueue *rq;

    /* ── Step 1: restart clock ───────────────────────────────────
     * The arch layer re-arms the timer.  What this layer advances is
     * the tick counters and the wall clock. */
    total_ticks++;
    jiffies++;
    jiffies_64++;

    xtime.tv_nsec += (uint32_t)TICK_NSEC;
    if (xtime.tv_nsec >= 1000000000UL) {
        xtime.tv_nsec -= 1000000000UL;
        xtime.tv_sec++;
    }

    /* ── Step 2: callout table ─────────────────────────────────── */
    process_callouts();

    /* ── Step 3: profiling sample ──────────────────────────────────
     * The PC values are the arch layer's to supply; this build passes
     * zero, which the profiler counts as bucket 0.  Wiring the real
     * values is the arch trap handler's job. */
    profiler_tick(0, 0);

    /* ── Steps 5 and 6: per-process statistics and CPU utilisation ──
     * Bach gathers these for every process in the system.  This only
     * reaches the QUEUED ones — a running process is not on a queue —
     * so the running one is charged in tick_process instead.  The
     * division is deliberate: uiox_kix_clock.c counts who is waiting, and
     * scheduler.c counts who is running. */
    rq = get_run_queue();
    for (q = 0; q < MAX_PRIORITY_QUEUES; q++)
        for (p = rq->heads[q]; p; p = (Process *)(void *)p->p.p_next) {
            if (p->s_cpu_usage < 255u) p->s_cpu_usage++;
        }

    /* ── Step 7: the once-per-second block ──────────────────────────
     * Bach: "if (1 second or more since last here and interrupt not in
     * critical region of code)".  The critical-region test has no
     * counterpart here — there is no splhi in this layer — so the
     * condition is the elapsed second alone. */
    if (jiffies - last_second_tick >= (uint64_t)HZ) {
        last_second_tick = jiffies;
        update_load_avg();
    }
}

/* ── callout_add ─────────────────────────────────────────────────────
 * Register a deferred call to fn(arg) after `ticks` ticks.
 *
 * Returns the slot index so the caller can cancel it, or -1 when the
 * table is full.  A full table is reported rather than silently
 * overrunning: an alarm that never fires is worse than one that
 * refuses to be set.
 *
 * This is the mechanism the missing WAKER is built from — a timed
 * sleep registers a callout that calls uiox_kix_psa_proc_wakeup on its
 * channel.  That connection is the highest-value integration left in
 * the process layer. */
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
            return i;
        }
    }
    return -1;
}

/* ── callout_cancel ──────────────────────────────────────────────────
 * Deactivate a callout by the index callout_add returned.  An index
 * that was never issued, or one already fired, is a no-op — cancelling
 * a callout that has run is not an error, it is a race the caller
 * cannot avoid. */
void callout_cancel(int idx)
{
    if (idx < 0 || idx >= MAX_CALLOUTS) return;
    if (callout_table.entries[idx].active) {
        callout_table.entries[idx].active = false;
        callout_table.count--;
    }
}

/* ── Readers ─────────────────────────────────────────────────────────── */
LoadAvg      *get_load_avg(void)      { return &load_avg; }
CalloutTable *get_callout_table(void) { return &callout_table; }
