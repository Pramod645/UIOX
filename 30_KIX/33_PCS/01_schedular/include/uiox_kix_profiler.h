/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_profiler.h
 *
 * Profiling — the sample the clock takes on every tick.
 *
 * ── Bach's two lines ────────────────────────────────────────────────
 * Algorithm 2 contains exactly this much about profiling:
 *
 *     if (kernel profiling on)
 *         note program counter at time of interrupt;
 *     if (user profiling on)
 *         note program counter at time of interrupt;
 *
 * profiler_tick(kernel_pc, user_pc) is those two lines.  What surrounds
 * it in this header — the 256-bucket histogram, the hot-spot report, the
 * NMI watchdog — is an EXTENSION, Linux's readprofile, not Bach.
 *
 * It is kept because a working profiler is useful during bring-up, but
 * the distinction is stated so that nobody reads "Algorithm 3" here and
 * goes looking for it in the text.  Bach's Algorithm 3 is in your notes
 * as a heading with nothing under it.
 *
 * ── the NMI watchdog, specifically ──────────────────────────────────
 * It is not profiling at all — it detects a FROZEN KERNEL by noticing
 * that jiffies has not moved.  It is here because it is driven by the
 * same tick, and removing it would leave a gap in the file that a later
 * reader would fill back in.
 *
 * @version 2.0.0  @date 2026-09-29
 */
#ifndef UIOX_PROFILER_H
#define UIOX_PROFILER_H

#include "sched_types.h"

/* ── Kernel profiler ─────────────────────────────────────────────────
 * The address space is divided into PROF_BUCKETS bins and each tick
 * increments the bin the interrupted PC falls in.  Over time the
 * histogram reveals hot spots. */
#define PROF_BUCKETS 256

typedef struct {
    uint64_t kernel_hits[PROF_BUCKETS];
    uint64_t user_hits[PROF_BUCKETS];
    bool     kernel_profiling;   /* Bach's "kernel profiling on"  */
    bool     user_profiling;     /* Bach's "user profiling on"    */
    uint64_t total_samples;
} Profiler;

/* ── NMI watchdog ────────────────────────────────────────────────────
 * last_jiffies is updated on every sample; if the gap since then reaches
 * threshold, the kernel has stopped taking interrupts. */
typedef struct {
    bool     enabled;
    uint64_t last_jiffies;
    uint64_t threshold;    /* ticks before declaring a freeze */
    uint64_t nmi_count;
} NmiWatchdog;

/* ── API ─────────────────────────────────────────────────────────────
 * Enable the histogram for either or both modes.  A mode left off is
 * still counted by total_samples but not binned. */
void profiler_init(bool kernel_on, bool user_on);

/* Bach's two lines.  Called by clock_tick every interrupt.
 * The PC values are the arch trap handler's to supply; this build passes
 * zero, which bins into bucket 0 — a visible "not wired yet" signal
 * rather than a silent one. */
void profiler_tick(uint64_t kernel_pc, uint64_t user_pc);

/* Print the top-N hottest buckets, emptying each as it reports. */
void profiler_report(int top_n);

/* NMI watchdog: arm it, and check whether jiffies has stopped moving. */
void nmi_watchdog_check(void);
void nmi_watchdog_enable(uint64_t freeze_threshold_ticks);

#endif /* UIOX_PROFILER_H */
