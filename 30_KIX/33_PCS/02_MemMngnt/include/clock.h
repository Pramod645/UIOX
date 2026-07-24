#ifndef CLOCK_H
#define CLOCK_H

#include "uiox_klibc.h"
#include "scheduler.h"

/* ── Callout table entry ────────────────────────────────────── */
#define MAX_CALLOUT     64

typedef struct callout {
    int      co_active;         /* slot in use                  */
    int      co_delta;          /* ticks until function fires   */
    void   (*co_func)(void *);  /* function to call             */
    void    *co_arg;            /* argument to function         */
} callout_t;

/* ── System statistics gathered each tick ───────────────────── */
typedef struct sys_stats {
    uint64_t ss_total_ticks;    /* total clock ticks            */
    uint64_t ss_idle_ticks;     /* ticks spent idle             */
    uint64_t ss_user_ticks;     /* ticks in user mode           */
    uint64_t ss_kernel_ticks;   /* ticks in kernel mode         */
    uint64_t ss_wait_ticks;     /* ticks waiting for I/O        */
    double   ss_load_avg[3];    /* 1, 5, 15 minute load avg     */
    int      ss_runnable;       /* # runnable processes         */
} sys_stats_t;

/* ── Kernel profiling entry ─────────────────────────────────── */
#define PROFILE_BUCKETS 256

typedef struct kprof {
    uint64_t  kp_counts[PROFILE_BUCKETS]; /* hits per PC bucket */
    uintptr_t kp_base_pc;                 /* lowest PC tracked  */
    uintptr_t kp_pc_step;                 /* bytes per bucket   */
    int       kp_enabled;
} kprof_t;

/* ── Globals ────────────────────────────────────────────────── */
extern callout_t callout_table[MAX_CALLOUT];
extern sys_stats_t sys_stats;
extern kprof_t kernel_prof;
extern kprof_t user_prof;

/* ── Function prototypes ────────────────────────────────────── */
void clock_init(void);
void clock_interrupt(uintptr_t kernel_pc, uintptr_t user_pc);

/* Callout table operations */
int  callout_add(int delta_ticks, void (*fn)(void *), void *arg);
void callout_del(int index);
void callout_tick(void);

/* Statistics */
void gather_system_stats(void);
void gather_per_process_stats(proc_entry_t *p);
void adjust_cpu_utilization(proc_entry_t *p);

/* Profiling */
void profile_kernel_tick(uintptr_t pc);
void profile_user_tick(uintptr_t pc);

/* Swapper wakeup (defined in swapper.c) */
void wakeup_swapper(void);

#endif /* CLOCK_H */
