/*
 * 30_KIX/33_PCS/01_schedular/include/sched_types.h
 *
 * Scheduling types — now built ON 40_psa rather than beside it.
 *
 * ── what changed, and why ───────────────────────────────────────────
 * This header used to define its own `Process` struct and its own
 * `TaskState` enum.  40_psa defines `uiox_kix_psa_proc_t` and the ten
 * PSA states.  Two structs for one concept is the defect class this tree
 * keeps producing — two models that never get compared, and the winner
 * decided by whichever a build happens to link.
 *
 * The decision was made: there is ONE process type, and it is 40_psa's.
 * Everything below either comes from that header or is scheduler-private
 * state that 40_psa has no reason to carry.
 *
 * ── the direction of the merge ──────────────────────────────────────
 * Where the two disagreed, 40_psa won, because its choices are the ones
 * Bach's model requires:
 *
 *   STATE      PSA's ten states, matching the state diagram in
 *              uiox_kix_psa_process.h.  The old six TASK_* values are
 *              gone — they were Linux-shaped and had no diagram behind
 *              them.
 *
 *   PRIORITY   PSA's p_sched, where a LOWER p_pri is a HIGHER
 *              priority — the convention the ready-queue ordering
 *              depends on.  The old separate static_priority /
 *              dynamic_priority / nice ints are gone, and with them the
 *              polarity inversion they carried.
 *
 *   TIMERS     PSA's p_timers, four clock_t matching struct tms exactly.
 *
 *   LINKS      PSA's p_next / p_prev pair, because the sleep hash needs
 *              backward traversal.  The old single `next` could not
 *              support it.
 *
 * ── what the scheduler ADDS ─────────────────────────────────────────
 * Five fields 40_psa has no reason to know about, because they are
 * scheduling policy rather than process identity.  They are declared in
 * a wrapper struct below so the boundary stays visible: everything under
 * `p` is 40_psa's, everything alongside it is the scheduler's.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#ifndef UIOX_SCHED_TYPES_H
#define UIOX_SCHED_TYPES_H

#include "uiox_klibc.h"
#include "uiox_kix_psa_process.h"   /* the ONE process type             */

/* ── Timing constants ─────────────────────────────────────────────────
 * HZ is the tick rate.  TIME_QUANTUM is how many ticks a process gets
 * before the scheduler preempts it — Bach's "time slice". */
#define HZ                  1000
#define CLOCK_TICK_RATE     1193182
#define LATCH               (CLOCK_TICK_RATE / HZ)
#define TICK_NSEC           (1000000000UL / HZ)
#define TIME_QUANTUM        10
#define MAX_PRIORITY        140
#define MAX_PROCESSES       64
#define MAX_CALLOUTS        32
#define MAX_PRIORITY_QUEUES 5
#define TIMER_MAGIC         0xDEADBEEFU
#define TVEC_SIZE           256

/* ── The scheduling policy a process runs under ──────────────────────
 * Kept from the old model because nothing in 40_psa expresses it: a
 * process's POLICY is not part of what it IS, only of how it is run. */
typedef enum {
    UIOX_SCHED_NORMAL = 0,
    UIOX_SCHED_FIFO   = 1,
    UIOX_SCHED_RR     = 2,
    UIOX_SCHED_FAIR   = 3
} uiox_sched_policy_t;

/* ── The process, as the scheduler sees it ───────────────────────────
 * `p`  — 40_psa's process table entry, the ONE definition.  Its state,
 *        priority, timers, links and signals all live there.
 *
 * `...` — scheduler-private state.  These five are here rather than on
 *        the process type because they are accounting the SCHEDULER
 *        keeps, not facts about the process.  40_psa has no opinion
 *        about how much CPU a process has used in this quantum.
 *
 * There is deliberately no `policy` on the process itself: policy is
 * per-process scheduling intent and belongs to the scheduler, so it
 * sits here alongside the accounting. */
typedef struct uiox_sched_proc {
    uiox_kix_psa_proc_t  p;              /* 40_psa — the real entry  */

    /* ── scheduler-private ─────────────────────────────────────── */
    int                  s_time_slice;   /* ticks left this quantum  */
    uint64_t             s_total_ticks;  /* lifetime CPU ticks       */
    uint32_t             s_cpu_usage;    /* 0..255, for the feedback */
    uiox_sched_policy_t  s_policy;       /* how this process is run  */
    int                  s_group_id;     /* fair-share grouping      */
    int                  s_in_memory;    /* resident; not swapped    */
} uiox_sched_proc_t;

/* ── Backwards-compatible alias ──────────────────────────────────────
 * A great deal of existing code says `Process *`.  Rather than rewrite
 * every signature, `Process` now MEANS the scheduler's view of a 40_psa
 * process — one type, two names, and the name that was already there
 * points at the definition that survived.
 *
 * New code should write uiox_sched_proc_t. */
typedef uiox_sched_proc_t  Process;

/* ── Load average (integer fixed-point x1000, no FPU) ────────────── */
typedef struct {
    uint32_t load_1;    /* 1-min  EWMA x1000  */
    uint32_t load_5;    /* 5-min  EWMA x1000  */
    uint32_t load_15;   /* 15-min EWMA x1000  */
} LoadAvg;

/* ── Callout entry ──────────────────────────────────────────────────── */
typedef struct Callout {
    int64_t   delta_ticks;
    void    (*fn)(void *arg);
    void     *arg;
    bool      active;
} Callout;

/* ── Global tick counters ───────────────────────────────────────────── */
extern volatile uint64_t jiffies;
extern volatile uint64_t jiffies_64;

/* ── Wall-clock time ────────────────────────────────────────────────── */
typedef struct {
    int64_t  tv_sec;
    uint32_t tv_nsec;
} XTime;
extern XTime xtime;

/* ── Timer source ───────────────────────────────────────────────────── */
typedef enum {
    TIMER_SRC_NONE = 0,
    TIMER_SRC_PIT,
    TIMER_SRC_TSC,
    TIMER_SRC_HPET,
    TIMER_SRC_ACPI_PMT,
    TIMER_SRC_LOCAL_APIC
} TimerSource;

#endif /* UIOX_SCHED_TYPES_H */
