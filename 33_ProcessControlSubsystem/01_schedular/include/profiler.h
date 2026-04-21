#ifndef UIOX_PROFILER_H
#define UIOX_PROFILER_H

#include "sched_types.h"

/* ─────────────────────────────────────────────────────────────
 * Kernel profiler (Algorithm 3 / readprofile equivalent)
 *
 * At each clock tick the profiler samples the program counter
 * (both kernel-mode PC and user-mode PC) and increments a
 * histogram bucket.  Over time the histogram reveals "hot spots."
 * ───────────────────────────────────────────────────────────── */

#define PROF_BUCKETS 256   /* address space divided into 256 bins */

typedef struct {
    uint64_t kernel_hits[PROF_BUCKETS];
    uint64_t user_hits[PROF_BUCKETS];
    bool     kernel_profiling;
    bool     user_profiling;
    uint64_t total_samples;
} Profiler;

/* ─────────────────────────────────────────────────────────────
 * NMI watchdog
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    bool     enabled;
    uint64_t last_jiffies;   /* jiffies at last NMI */
    uint64_t threshold;      /* ticks before declaring freeze */
    uint64_t nmi_count;
} NmiWatchdog;

/* ─────────────────────────────────────────────────────────────
 * Profiler API
 * ───────────────────────────────────────────────────────────── */
void profiler_init(bool kernel_on, bool user_on);

/*
 * profile_tick — called by clock_tick every interrupt.
 * Samples kernel_pc and user_pc into histogram buckets.
 */
void profiler_tick(uint64_t kernel_pc, uint64_t user_pc);

/* Print top-N hot buckets */
void profiler_report(int top_n);

/* NMI watchdog: check and reset */
void nmi_watchdog_check(void);
void nmi_watchdog_enable(uint64_t freeze_threshold_ticks);

#endif /* UIOX_PROFILER_H */
