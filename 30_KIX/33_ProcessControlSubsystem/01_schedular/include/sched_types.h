#ifndef UIOX_SCHED_TYPES_H
#define UIOX_SCHED_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ─────────────────────────────────────────────────────────────
 * Constants
 * ───────────────────────────────────────────────────────────── */
#define HZ                  1000        /* clock ticks per second           */
#define CLOCK_TICK_RATE     1193182     /* PIT oscillator frequency (Hz)    */
#define LATCH               (CLOCK_TICK_RATE / HZ) /* PIT divisor          */
#define TICK_NSEC           (1000000000UL / HZ)    /* nanoseconds per tick  */
#define TIME_QUANTUM        10          /* default time slice (ticks)       */
#define MAX_PRIORITY        140         /* priority range 0..139            */
#define MAX_PROCESSES       64
#define MAX_CALLOUTS        32
#define MAX_PRIORITY_QUEUES 5           /* multilevel feedback queues       */
#define TIMER_MAGIC         0xDEADBEEF
#define TVEC_SIZE           256         /* timer wheel bucket count         */

/* ─────────────────────────────────────────────────────────────
 * Process states
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    TASK_RUNNING         = 0,
    TASK_SLEEPING        = 1,
    TASK_INTERRUPTIBLE   = 2,
    TASK_UNINTERRUPTIBLE = 3,
    TASK_ZOMBIE          = 4,
    TASK_STOPPED         = 5
} TaskState;

/* ─────────────────────────────────────────────────────────────
 * Scheduling policy
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    SCHED_NORMAL    = 0,   /* round-robin multilevel feedback    */
    SCHED_FIFO      = 1,   /* real-time FIFO                     */
    SCHED_RR        = 2,   /* real-time round-robin              */
    SCHED_FAIR      = 3    /* fair-share group scheduling        */
} SchedPolicy;

/* ─────────────────────────────────────────────────────────────
 * Process time accounting (struct tms equivalent)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t tms_utime;    /* user time of process               */
    uint64_t tms_stime;    /* kernel (system) time of process    */
    uint64_t tms_cutime;   /* user time of children              */
    uint64_t tms_cstime;   /* kernel time of children            */
} ProcTimes;

/* ─────────────────────────────────────────────────────────────
 * Process Control Block (scheduling view)
 * ───────────────────────────────────────────────────────────── */
typedef struct Process {
    int          pid;
    TaskState    state;
    SchedPolicy  policy;

    /* Priority fields */
    int          static_priority;   /* set by nice/setpriority    */
    int          dynamic_priority;  /* recalculated each quantum  */
    int          nice;              /* -20..+19                   */

    /* Time accounting */
    uint64_t     time_slice;        /* remaining ticks in quantum */
    uint64_t     total_ticks;       /* total CPU ticks consumed   */
    ProcTimes    times;

    /* CPU utilisation decay (exponential moving average) */
    uint32_t     cpu_usage;         /* scaled 0–255               */

    /* Alarm */
    uint64_t     alarm_expire;      /* jiffies value when alarm fires */
    bool         alarm_active;

    /* Fair-share group ID */
    int          group_id;

    /* Loaded in memory? (swapper integration) */
    bool         in_memory;

    /* Linked list for run queue */
    struct Process *next;
} Process;

/* ─────────────────────────────────────────────────────────────
 * Callout table entry (deferred function calls at tick time)
 * ───────────────────────────────────────────────────────────── */
typedef struct Callout {
    int64_t       delta_ticks;      /* ticks until this fires     */
    void        (*fn)(void *arg);
    void         *arg;
    bool          active;
} Callout;

/* ─────────────────────────────────────────────────────────────
 * System load averages (1-, 5-, 15-minute EWMAs)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    double load_1;
    double load_5;
    double load_15;
} LoadAvg;

/* ─────────────────────────────────────────────────────────────
 * Global jiffies counter
 * ───────────────────────────────────────────────────────────── */
extern volatile uint64_t jiffies;
extern volatile uint64_t jiffies_64;

/* ─────────────────────────────────────────────────────────────
 * xtime — wall-clock time (seconds + nanoseconds since epoch)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int64_t  tv_sec;
    uint32_t tv_nsec;
} XTime;

extern XTime xtime;

/* ─────────────────────────────────────────────────────────────
 * Timer source type (hardware abstraction)
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    TIMER_SRC_NONE = 0,
    TIMER_SRC_PIT,
    TIMER_SRC_TSC,
    TIMER_SRC_HPET,
    TIMER_SRC_ACPI_PMT,
    TIMER_SRC_LOCAL_APIC
} TimerSource;

#endif /* UIOX_SCHED_TYPES_H */
