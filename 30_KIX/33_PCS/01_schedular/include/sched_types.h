/*
 * 30_KIX/33_PCS/01_schedular/include/sched_types.h
 *
 * REMOVED system headers:
 *   #include <stdint.h>
 *   #include <stdbool.h>
 *   #include <stddef.h>
 *   #include <time.h>
 * REPLACED WITH: #include "uiox_klibc.h"
 *
 * NOTE: LoadAvg.load_* changed from double → uint32_t x1000
 *       to eliminate the FPU dependency in freestanding builds.
 *       Example: load average 1.234 is stored as 1234.
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_SCHED_TYPES_H
#define UIOX_SCHED_TYPES_H

/*
 * uiox_klibc.h must be reachable via -I33_PCS/include.
 * It replaces <stdint.h>, <stdbool.h>, <stddef.h>, and <time.h>.
 */
#include "uiox_klibc.h"

/* ── Timing constants ───────────────────────────────────────────────── */
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

/* ── Process states ─────────────────────────────────────────────────── */
typedef enum {
    TASK_RUNNING         = 0,
    TASK_SLEEPING        = 1,
    TASK_INTERRUPTIBLE   = 2,
    TASK_UNINTERRUPTIBLE = 3,
    TASK_ZOMBIE          = 4,
    TASK_STOPPED         = 5
} TaskState;

/* ── Scheduling policy ──────────────────────────────────────────────── */
typedef enum {
    SCHED_NORMAL = 0,
    SCHED_FIFO   = 1,
    SCHED_RR     = 2,
    SCHED_FAIR   = 3
} SchedPolicy;

/* ── Process time accounting ────────────────────────────────────────── */
typedef struct {
    uint64_t tms_utime;
    uint64_t tms_stime;
    uint64_t tms_cutime;
    uint64_t tms_cstime;
} ProcTimes;

/* ── Process Control Block (scheduling view) ────────────────────────── */
typedef struct Process {
    int              pid;
    TaskState        state;
    SchedPolicy      policy;
    int              static_priority;
    int              dynamic_priority;
    int              nice;             /* nice value -20..+19, default 0  */
    int              time_slice;
    uint64_t         total_ticks;
    ProcTimes        times;
    uint32_t         cpu_usage;
    uint64_t         alarm_expire;
    bool             alarm_active;
    int              group_id;
    bool             in_memory;
    struct Process  *next;
} Process;

/* ── Callout entry ──────────────────────────────────────────────────── */
typedef struct Callout {
    int64_t   delta_ticks;
    void    (*fn)(void *arg);
    void     *arg;
    bool      active;
} Callout;

/* ── Load average (integer fixed-point × 1000, no FPU) ─────────────── */
typedef struct {
    uint32_t load_1;    /* 1-min  EWMA × 1000  */
    uint32_t load_5;    /* 5-min  EWMA × 1000  */
    uint32_t load_15;   /* 15-min EWMA × 1000  */
} LoadAvg;

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
