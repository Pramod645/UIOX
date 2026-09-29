/*
 * 30_KIX/33_PCS/02_MemMngnt/src/uiox_kix_accounting.c
 *
 * Per-process and system-wide accounting — the three measurements the
 * memory subsystem takes on every tick.
 *
 * ── what this file replaced, and what it deliberately does not do ───
 * 02_MemMngnt/src/clock.c held nine functions.  Six belonged to
 * 01_schedular and are gone from here:
 *
 *   callout_add / callout_del / callout_tick   now uiox_kix_clock.c
 *   clock_interrupt                            now uiox_kix_clock.c
 *   profile_kernel_tick / profile_user_tick    now uiox_kix_profiler.c
 *
 * callout_add was the acute one: a SECOND definition of a name
 * 01_schedular already provides.  A build linking both would either fail
 * at link or silently keep one — and which one is decided by argument
 * order, not by intent.  That is the collision this file removes.
 *
 * There is no clock tick here, no callout table, and no profiler.  A
 * reader looking for those should go to 01_schedular.
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * The three keepers used proc_entry_t, the rival entry from a scheduler
 * header that is now deleted.  They take uiox_kix_psa_proc_t instead, and
 * the three fields they touch are spelled out per function below.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_accounting.h"
#include "../include/uiox_kix_scheduler.h"   /* uiox_printf */

/* ── Globals ────────────────────────────────────────────────────────── */
uiox_kix_sys_stats_t uiox_kix_sys_stats;

/* ── ewma_update ─────────────────────────────────────────────────────
 * One step of an exponential moving average, in integer arithmetic:
 *
 *     new = (alpha * old + (SCALE - alpha) * sample) / SCALE
 *
 * Integer because the x86_64 build carries -mno-sse -mno-sse2, which ban
 * the registers a double is passed in.  The old version used double and
 * its banner noted "the FPU is available on arm64" — true, and beside
 * the point, since one failing arch is enough to break the build.
 *
 * The 64-bit intermediate is required: alpha is up to 999 and old up to
 * a few thousand, so the product overflows 32 bits immediately. */
static uiox_uint32_t uiox_kix_ewma(uiox_uint32_t alpha,
                                   uiox_uint32_t old_val,
                                   uiox_uint32_t sample)
{
    return (uiox_uint32_t)(((uiox_uint64_t)alpha * (uiox_uint64_t)old_val +
                            (uiox_uint64_t)(UIOX_KIX_LOAD_SCALE - alpha) *
                            (uiox_uint64_t)sample) / UIOX_KIX_LOAD_SCALE);
}

/* ── uiox_kix_gather_system_stats ────────────────────────────────────
 * Count runnable processes and fold the count into the load averages.
 *
 * Bach's Algorithm 2 gathers system statistics on every tick.  This
 * updates the counters on every call and lets the CALLER decide how
 * often to feed the averages — because the averages are a per-second
 * quantity and the counters are per-tick.  The old version conflated the
 * two by updating one average per tick.
 *
 * ── three fixes over the version this replaces ──────────────────────
 *   1. ALL THREE averages are updated.  The old code touched only
 *      load_avg[0]; the 5- and 15-minute fields existed in the struct
 *      and stayed at zero, which is a field that looks like a
 *      measurement and never changes.
 *
 *   2. The decay factor is right.  The old 0.9 does not correspond to
 *      one minute — at HZ=1000 the real factor for sixty seconds is
 *      exp(-1/60) ~ 0.983.  0.9 gives a window nearer ten ticks, so the
 *      "1-minute" figure was measuring about a hundredth of a second.
 *
 *   3. It is integer arithmetic, for the -mno-sse reason above.
 *
 * ── what "runnable" means here ──────────────────────────────────────
 * READY plus the two RUNNING states.  Bach's load is the number of
 * processes that WANT the CPU, which excludes sleepers — a process
 * waiting on I/O is not load, however long it waits. */
void uiox_kix_gather_system_stats(void)
{
    int i;
    uiox_uint32_t runnable = 0u;

    uiox_kix_sys_stats.ss_total_ticks++;

    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        uiox_kix_psa_proc_t *p = &uiox_kix_psa_proc_table[i];

        if (p->p_state == UIOX_KIX_PSA_PROC_READY   ||
            p->p_state == UIOX_KIX_PSA_PROC_USER_RUNNING ||
            p->p_state == UIOX_KIX_PSA_PROC_KERNEL_RUNNING)
            runnable++;
    }

    uiox_kix_sys_stats.ss_runnable = runnable;

    uiox_kix_sys_stats.ss_load_avg.la_1 =
        uiox_kix_ewma(UIOX_KIX_LOAD_ALPHA_1,
                      uiox_kix_sys_stats.ss_load_avg.la_1, runnable);
    uiox_kix_sys_stats.ss_load_avg.la_5 =
        uiox_kix_ewma(UIOX_KIX_LOAD_ALPHA_5,
                      uiox_kix_sys_stats.ss_load_avg.la_5, runnable);
    uiox_kix_sys_stats.ss_load_avg.la_15 =
        uiox_kix_ewma(UIOX_KIX_LOAD_ALPHA_15,
                      uiox_kix_sys_stats.ss_load_avg.la_15, runnable);
}

/* ── uiox_kix_gather_per_process_stats ───────────────────────────────
 * Charge one process for the tick it just used.
 *
 * THREE fields, and they are three different quantities — which is why
 * this function does not collapse them into one increment:
 *
 *   p_sched.p_cpu       recent CPU ticks.  The INPUT the priority
 *                       feedback reads: a larger value becomes a
 *                       larger p_pri, which is a lower priority.
 *                       This is the value adjust_cpu_utilization
 *                       decays, because it must measure RECENT use.
 *
 *   p_timers.p_utime    struct tms user time.  A LIFETIME total, never
 *                       decayed — what times() reports and what a
 *                       parent collects from a child.  Monotonic by
 *                       definition; decaying it would make it lie.
 *
 *   p_sched.p_time      residence.  How long this process has been in
 *                       memory, which Bach's swapper scores a
 *                       swap-out candidate by ("residence time +
 *                       nice").  It advances while the process is
 *                       resident, including while it sleeps.
 *
 * The first two map onto the old pe_sched.sp_cpu_usage and
 * pe_tms.tms_utime directly.  The third has no single counterpart — the
 * old code had a separate sp_residence — and p_time is where it lives
 * now.  That is a SHARED field: if the scheduler also advances p_time
 * for its own purposes, this double-counts, and the fix is a field
 * rather than a guard here.
 *
 * A NULL process is skipped rather than faulted: the caller is the tick
 * handler and a machine with nothing running is a legal state. */
void uiox_kix_gather_per_process_stats(uiox_kix_psa_proc_t *p)
{
    if (!p) return;

    p->p_sched.p_cpu++;
    p->p_timers.p_utime++;
    p->p_sched.p_time++;
}

/* ── uiox_kix_adjust_cpu_utilization ─────────────────────────────────
 * Decay a process's CPU charge: usage = (usage * 2) / 3.
 *
 * The decay is what makes the priority feedback a recency measure
 * rather than a lifetime total.  Without it a long-running process
 * climbs to the worst priority and stays there — the priority feedback
 * would be a one-way ratchet, and every process that ran long enough
 * would end up starved.
 *
 * x2/3 is the "UNIX decay" the old version used.  It is a weaker decay
 * than the load average's exp(-1/60) because the two answer different
 * questions: the load average describes the machine over a minute, and
 * this describes one process over the quantum it just ran.
 *
 * Integer division truncates, so a process sitting at p_cpu == 1 decays
 * to 0 and stops — which is correct: 1 * 2 / 3 is 0, and a charge below
 * one tick is not representable. */
void uiox_kix_adjust_cpu_utilization(uiox_kix_psa_proc_t *p)
{
    if (!p) return;

    p->p_sched.p_cpu = (p->p_sched.p_cpu * 2) / 3;
}
