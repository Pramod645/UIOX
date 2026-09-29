/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_profiler.c
 *
 * Profiling — the sample the clock takes on every tick.
 *
 * ── Bach's two lines, and the extension around them ─────────────────
 * Algorithm 2 says only this much about profiling:
 *
 *     if (kernel profiling on)
 *         note program counter at time of interrupt;
 *     if (user profiling on)
 *         note program counter at time of interrupt;
 *
 * profiler_tick() is those two lines.  Everything else in this file —
 * the 256-bucket histogram, the hot-spot report, the NMI watchdog — is
 * Linux's readprofile, NOT Bach.  It is kept because a working profiler
 * is useful during bring-up, and labelled because a reader who sees
 * "Algorithm 3" here would go looking for it in the text, where it is a
 * heading with nothing under it.
 *
 * ── binning ─────────────────────────────────────────────────────────
 * A PC is hashed to a bucket by shifting right 8 and taking the modulo
 * PROF_BUCKETS.  The shift drops the low bits, which on a typical
 * instruction stream are nearly constant and would collapse every
 * sample into a handful of adjacent bins.
 *
 * ── what this file does NOT name ────────────────────────────────────
 * No process type.  That is why the port of 01_schedular to the merged
 * 40_psa type left this file alone — verified, not assumed.
 *
 * ── the NMI watchdog, which is not profiling ────────────────────────
 * It detects a FROZEN KERNEL by noticing that jiffies has stopped
 * moving.  It lives here because it rides the same tick, not because it
 * is related — and naming that is better than letting a reader assume
 * the two are the same mechanism.
 *
 * @version 2.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_profiler.h"
#include "../include/uiox_kix_scheduler.h"   /* jiffies */

static Profiler    prof;
static NmiWatchdog nmi_wd;

/* ── profiler_init ───────────────────────────────────────────────────
 * A mode left OFF is still counted by total_samples but not binned, so
 * a caller can tell "no samples taken" from "samples taken with the
 * histogram disabled" — two different facts a single zero would blur. */
void profiler_init(bool kernel_on, bool user_on)
{
    uiox_uint32_t i;

    prof.kernel_profiling = kernel_on;
    prof.user_profiling   = user_on;
    prof.total_samples    = 0u;

    for (i = 0; i < PROF_BUCKETS; i++) {
        prof.kernel_hits[i] = 0u;
        prof.user_hits[i]   = 0u;
    }

    nmi_wd.enabled      = false;
    nmi_wd.last_jiffies = 0u;
    nmi_wd.threshold    = 0u;
    nmi_wd.nmi_count    = 0u;
}

/* ── profiler_tick ───────────────────────────────────────────────────
 * Bach's two lines.  Called from clock_tick on every interrupt.
 *
 * The PC values come from the arch trap handler.  This build passes
 * zero to both, which bins into bucket 0 — a visible "not wired yet"
 * signal rather than a silent one that would look like a cold spot. */
void profiler_tick(uiox_uint64_t kernel_pc, uiox_uint64_t user_pc)
{
    prof.total_samples++;

    if (prof.kernel_profiling) {
        unsigned int bucket =
            (unsigned int)((kernel_pc >> 8) % (uiox_uint64_t)PROF_BUCKETS);
        prof.kernel_hits[bucket]++;
    }

    if (prof.user_profiling) {
        unsigned int bucket =
            (unsigned int)((user_pc >> 8) % (uiox_uint64_t)PROF_BUCKETS);
        prof.user_hits[bucket]++;
    }

    /* The watchdog rides the same tick: a sample arriving proves the
     * kernel is still taking interrupts, so the last-seen time moves. */
    if (nmi_wd.enabled) {
        nmi_wd.last_jiffies = jiffies;
        nmi_wd.nmi_count++;
    }
}

/* ── pick_top ────────────────────────────────────────────────────────
 * Finds the fullest bucket and empties it, so the next call reports the
 * runner-up rather than the same bucket again.
 *
 * Returns -1 when every bucket is empty, which is how profiler_report
 * knows to stop rather than print a run of zeros. */
static int pick_top(uiox_uint64_t *hits)
{
    uiox_uint64_t max_hits = 0u;
    int           max_idx  = -1;
    int           b;

    for (b = 0; b < PROF_BUCKETS; b++) {
        if (hits[b] > max_hits) { max_hits = hits[b]; max_idx = b; }
    }
    if (max_idx >= 0) hits[max_idx] = 0u;   /* consume it */

    return max_idx;
}

/* ── profiler_report ─────────────────────────────────────────────────
 * The top-N hottest buckets, kernel first then user.
 *
 * Printing through uiox_printf, the freestanding primitive — this layer
 * has no console of its own.
 *
 * Destructive by design: each reported bucket is zeroed, so a second
 * call reports the NEXT set rather than the same one.  A caller wanting
 * a running total should not call this. */
void profiler_report(int top_n)
{
    int t;
    int idx;

    uiox_printf("[profil] samples=%lu  kernel=%s user=%s\n",
                (unsigned long)prof.total_samples,
                prof.kernel_profiling ? "on" : "off",
                prof.user_profiling   ? "on" : "off");

    uiox_printf("  kernel hot spots:\n");
    for (t = 0; t < top_n; t++) {
        idx = pick_top(prof.kernel_hits);
        if (idx < 0) break;
        uiox_printf("    bucket %d\n", idx);
    }

    uiox_printf("  user hot spots:\n");
    for (t = 0; t < top_n; t++) {
        idx = pick_top(prof.user_hits);
        if (idx < 0) break;
        uiox_printf("    bucket %d\n", idx);
    }
}

/* ── NMI watchdog ──────────────────────────────────────────────────── */

void nmi_watchdog_enable(uiox_uint64_t freeze_threshold_ticks)
{
    nmi_wd.enabled      = true;
    nmi_wd.threshold    = freeze_threshold_ticks;
    nmi_wd.last_jiffies = jiffies;
    nmi_wd.nmi_count    = 0u;
}

/* Reports when the gap since the last sample reaches the threshold.
 *
 * Note it does NOT reset last_jiffies — so a frozen kernel keeps
 * reporting on every check rather than firing once and going quiet.
 * That is the useful behaviour for a fault that does not recover. */
void nmi_watchdog_check(void)
{
    uiox_uint64_t elapsed;

    if (!nmi_wd.enabled) return;

    elapsed = jiffies - nmi_wd.last_jiffies;
    if (elapsed >= nmi_wd.threshold) {
        uiox_printf("[nmi] KERNEL FREEZE: %lu ticks since last sample\n",
                    (unsigned long)elapsed);
    }
}
