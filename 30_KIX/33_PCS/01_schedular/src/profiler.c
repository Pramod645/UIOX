 #include "../include/profiler.h"

static Profiler    prof;
static NmiWatchdog nmi_wd;

/* ─────────────────────────────────────────────────────────────
 * profiler_init
 * ───────────────────────────────────────────────────────────── */
void profiler_init(bool kernel_on, bool user_on)
{
    memset(&prof, 0, sizeof prof);
    prof.kernel_profiling = kernel_on;
    prof.user_profiling   = user_on;
    printf("[profiler] init: kernel=%s  user=%s\n",
           kernel_on ? "on" : "off",
           user_on   ? "on" : "off");
}

/* ─────────────────────────────────────────────────────────────
 * profiler_tick
 * Hash PC into a bucket (simple: top 8 bits of address).
 * ───────────────────────────────────────────────────────────── */
void profiler_tick(uint64_t kernel_pc, uint64_t user_pc)
{
    prof.total_samples++;

    if (prof.kernel_profiling) {
        unsigned int bucket = (unsigned int)((kernel_pc >> 8) % PROF_BUCKETS);
        prof.kernel_hits[bucket]++;
    }

    if (prof.user_profiling) {
        unsigned int bucket = (unsigned int)((user_pc >> 8) % PROF_BUCKETS);
        prof.user_hits[bucket]++;
    }

    /* NMI watchdog: reset last-seen tick */
    if (nmi_wd.enabled) {
        nmi_wd.last_jiffies = jiffies;
        nmi_wd.nmi_count++;
    }
}

/* ─────────────────────────────────────────────────────────────
 * profiler_report
 * ───────────────────────────────────────────────────────────── */
void profiler_report(int top_n)
{
    printf("[profiler] total_samples=%llu\n",
           (unsigned long long)prof.total_samples);

    printf("  Top %d kernel hot spots:\n", top_n);
    for (int t = 0; t < top_n; t++) {
        uint64_t max_hits = 0;
        int      max_idx  = 0;
        for (int b = 0; b < PROF_BUCKETS; b++) {
            if (prof.kernel_hits[b] > max_hits) {
                max_hits = prof.kernel_hits[b];
                max_idx  = b;
            }
        }
        if (max_hits == 0) break;
        printf("    bucket %3d  hits=%llu\n",
               max_idx, (unsigned long long)max_hits);
        prof.kernel_hits[max_idx] = 0; /* remove for next iteration */
    }

    printf("  Top %d user hot spots:\n", top_n);
    for (int t = 0; t < top_n; t++) {
        uint64_t max_hits = 0;
        int      max_idx  = 0;
        for (int b = 0; b < PROF_BUCKETS; b++) {
            if (prof.user_hits[b] > max_hits) {
                max_hits = prof.user_hits[b];
                max_idx  = b;
            }
        }
        if (max_hits == 0) break;
        printf("    bucket %3d  hits=%llu\n",
               max_idx, (unsigned long long)max_hits);
        prof.user_hits[max_idx] = 0;
    }
}

/* ─────────────────────────────────────────────────────────────
 * NMI watchdog
 * ───────────────────────────────────────────────────────────── */
void nmi_watchdog_enable(uint64_t freeze_threshold_ticks)
{
    nmi_wd.enabled   = true;
    nmi_wd.threshold = freeze_threshold_ticks;
    nmi_wd.last_jiffies = jiffies;
    printf("[nmi_watchdog] enabled  threshold=%llu ticks\n",
           (unsigned long long)freeze_threshold_ticks);
}

void nmi_watchdog_check(void)
{
    if (!nmi_wd.enabled) return;
    uint64_t elapsed = jiffies - nmi_wd.last_jiffies;
    if (elapsed >= nmi_wd.threshold) {
        printf("[nmi_watchdog] KERNEL FREEZE DETECTED! elapsed=%llu ticks\n",
               (unsigned long long)elapsed);
    }
}
