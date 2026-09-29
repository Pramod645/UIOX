/*
 * 30_KIX/33_PCS/02_MemMngnt/include/uiox_kix_accounting.h
 *
 * Per-process and system-wide ACCOUNTING — the three things the memory
 * subsystem measures on every tick.
 *
 * ── this replaces clock.c, and here is why ──────────────────────────
 * 02_MemMngnt/src/clock.c carried NINE functions.  Six of them were not
 * this layer's:
 *
 *   callout_add / callout_del / callout_tick   -> 01_schedular
 *   clock_interrupt                            -> 01_schedular (clock_tick)
 *   profile_kernel_tick / profile_user_tick    -> 01_schedular (profiler.c)
 *
 * callout_add was the acute problem: it was a SECOND definition of a
 * function 01_schedular already provides, so a build linking both layers
 * would either fail to link or silently pick one.  Two implementations of
 * one name is the defect this whole pass exists to remove.
 *
 * What is left — and what this file is — is the three functions that
 * genuinely measure memory behaviour:
 *
 *   gather_system_stats        how many processes are runnable, and the
 *                              load average that follows from it
 *   gather_per_process_stats   charge one process for the tick it used
 *   adjust_cpu_utilization     decay that charge so it does not grow
 *                              without bound
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * Every process argument is uiox_kix_psa_proc_t.  The earlier version
 * took proc_entry_t — a rival entry from a scheduler header that is now
 * deleted — and reached for pe_sched.sp_cpu_usage, pe_tms.tms_utime and
 * pe_sched.sp_residence.
 *
 * The first two map onto real fields.  The third does not, and is the
 * one decision inside this file:
 *
 *   p_sched.p_cpu    CPU ticks recently used — the field the priority
 *                    feedback reads, and the one this file increments
 *   p_timers.p_utime struct tms user time — what times() reports
 *   p_sched.p_time   RESIDENCE.  Bach's swapper scores a swap-out
 *                    candidate by "residence time", and this is the
 *                    field that carries it
 *
 * Whether p_time is the right home for residence is stated here rather
 * than assumed: it is shared with the scheduler's own use of the field,
 * and a scheduler that also advances p_time would double-count.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_ACCOUNTING_H
#define UIOX_KIX_ACCOUNTING_H

#include "uiox_klibc.h"
#include "uiox_kix_psa_process.h"

/* ── Load average, as a fixed-point pair ─────────────────────────────
 * The old version held double[3] and did:
 *
 *     ss_load_avg[0] = ss_load_avg[0] * 0.9 + runnable * 0.1;
 *
 * Three problems with that, and the third is why it is integer here:
 *
 *   1. Only load_avg[0] was ever updated.  The 5- and 15-minute
 *      averages existed in the struct and stayed at zero — a field that
 *      looks like a measurement and never changes.
 *
 *   2. The decay factor 0.9 does not correspond to one minute.  The
 *      real factor for a one-minute half-life at HZ=1000 is
 *      exp(-1/60) ~ 0.9835.  0.9 gives a window nearer 10 ticks.
 *
 *   3. It used double arithmetic, and the x86_64 build carries
 *      -mno-sse -mno-sse2 — which ban the registers a double is passed
 *      in.  The old banner claimed "the FPU is available on arm64" and
 *      left it at that, which is true and beside the point: one arch
 *      failing is enough.
 *
 * So the averages are integers scaled by 1000, the same scheme
 * 01_schedular's LoadAvg uses, and all three are updated.
 *
 * The decay factors, x1000: exp(-1/60), exp(-1/300), exp(-1/900). */
#define UIOX_KIX_LOAD_SCALE     1000u
#define UIOX_KIX_LOAD_ALPHA_1   983u   /* exp(-1/60)  x 1000 */
#define UIOX_KIX_LOAD_ALPHA_5   997u   /* exp(-1/300) x 1000 */
#define UIOX_KIX_LOAD_ALPHA_15  999u   /* exp(-1/900) x 1000 */

typedef struct uiox_kix_load_avg {
    uiox_uint32_t la_1;    /* 1-minute,  scaled x1000 */
    uiox_uint32_t la_5;    /* 5-minute,  scaled x1000 */
    uiox_uint32_t la_15;   /* 15-minute, scaled x1000 */
} uiox_kix_load_avg_t;

/* ── System statistics ───────────────────────────────────────────────
 * What the accounting collects about the machine as a whole.  Tick
 * counters are plain uiox_uint64_t: they only ever increase. */
typedef struct uiox_kix_sys_stats {
    uiox_uint64_t       ss_total_ticks;
    uiox_uint64_t       ss_user_ticks;
    uiox_uint64_t       ss_kernel_ticks;
    uiox_uint64_t       ss_idle_ticks;
    uiox_uint64_t       ss_wait_ticks;
    uiox_uint32_t       ss_runnable;   /* processes ready or running */
    uiox_kix_load_avg_t ss_load_avg;   /* all three, actually updated */
} uiox_kix_sys_stats_t;

extern uiox_kix_sys_stats_t uiox_kix_sys_stats;

/* ── Tick accounting ─────────────────────────────────────────────────
 * Called once per clock interrupt, AFTER the scheduler has decided who
 * is running.  The order matters: the figures describe the tick that
 * just elapsed, so the running process must already be known. */

/* Count runnable processes and fold the count into all three load
 * averages.  Bach's Algorithm 2 gathers system statistics every tick
 * and adjusts the averages once a second — this updates the counters
 * every call and lets the caller decide the cadence. */
void uiox_kix_gather_system_stats(void);

/* Charge one process for the tick it just used.
 *
 * Three fields, and they are deliberately different quantities:
 *   p_sched.p_cpu      priority feedback input
 *   p_timers.p_utime   struct tms, what times() reports
 *   p_sched.p_time     residence — how long it has been in memory
 *
 * A process that is not running is not charged; pass NULL to skip. */
void uiox_kix_gather_per_process_stats(uiox_kix_psa_proc_t *p);

/* Decay a process's CPU charge so it does not grow without bound.
 *
 * The decay is what makes the priority feedback a RECENT-usage measure
 * rather than a lifetime total.  Without it a long-running process
 * climbs to the worst priority and stays there, which is starvation
 * dressed as fairness.
 *
 * x2/3 per call, matching the old version's "UNIX decay" */
void uiox_kix_adjust_cpu_utilization(uiox_kix_psa_proc_t *p);

#endif /* UIOX_KIX_ACCOUNTING_H */
