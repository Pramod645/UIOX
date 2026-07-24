#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "uiox_klibc.h"
//#include <time.h>

/* ── Constants ──────────────────────────────────────────────── */
#define NPROC           64          /* max processes             */
#define TIME_QUANTUM    100         /* ticks per time slice      */
#define NICE_DEFAULT    0
#define NICE_MIN       -20
#define NICE_MAX        19

/* ── Priority ranges ────────────────────────────────────────── */
#define PRIO_KERNEL     0           /* highest (kernel sleeping) */
#define PRIO_USER_BASE  50          /* base user priority        */
#define PRIO_IDLE       127         /* lowest (idle)             */

/* ── Process states (scheduler view) ───────────────────────── */
typedef enum sched_state {
    SCHED_UNUSED      = 0,
    SCHED_RUNNING     = 1,   /* on CPU                          */
    SCHED_READY       = 2,   /* in run queue, in memory         */
    SCHED_SLEEP       = 3,   /* sleeping, in memory             */
    SCHED_READY_SWAP  = 4,   /* ready but swapped out           */
    SCHED_SLEEP_SWAP  = 5,   /* sleeping and swapped out        */
    SCHED_ZOMBIE      = 6,   /* exited, awaiting parent wait()  */
    SCHED_CREATED     = 7    /* newly forked                    */
} sched_state_t;

/* ── Scheduling parameters ──────────────────────────────────── */
typedef struct sched_param {
    int      sp_priority;    /* effective priority (0=highest)  */
    int      sp_nice;        /* user-settable bias              */
    int      sp_cpu_usage;   /* recent CPU ticks used           */
    uint32_t sp_time_slice;  /* ticks remaining in quantum      */
    uint32_t sp_residence;   /* ticks resident in memory        */
} sched_param_t;

/* ── Process timing totals (struct tms analogue) ────────────── */
typedef struct proc_tms {
    clock_t tms_utime;       /* user CPU time of this process   */
    clock_t tms_stime;       /* kernel CPU time of this process */
    clock_t tms_cutime;      /* user time of waited children    */
    clock_t tms_cstime;      /* kernel time of waited children  */
} proc_tms_t;

/* ── Lightweight process descriptor for the scheduler ───────── */
typedef struct proc_entry {
    uint32_t       pe_pid;
    uint16_t       pe_uid;
    sched_state_t  pe_state;
    sched_param_t  pe_sched;
    proc_tms_t     pe_tms;
    int            pe_locked;       /* 1 = locked in memory      */
    int            pe_in_memory;    /* 1 = loaded in main memory */
    uint32_t       pe_size;         /* process size in pages     */
    uint32_t       pe_swap_time;    /* time when swapped out     */
    int            pe_alarm;        /* alarm countdown (ticks)   */
    int            pe_group_id;     /* fair-share group          */
} proc_entry_t;

/* ── Run queue ──────────────────────────────────────────────── */
typedef struct run_queue {
    proc_entry_t  *rq_procs[NPROC];
    int            rq_count;
} run_queue_t;

/* ── Globals ────────────────────────────────────────────────── */
extern proc_entry_t  proc_table[NPROC];
extern proc_entry_t *current_proc;
extern run_queue_t   run_queue;
extern uint64_t      clock_ticks;      /* total ticks since boot */
extern int           need_resched;     /* reschedule flag         */

/* ── Function prototypes ────────────────────────────────────── */
void schedule_process(void);
void run_queue_add(proc_entry_t *p);
void run_queue_remove(proc_entry_t *p);
proc_entry_t *pick_highest_priority(void);
void recalc_priority(proc_entry_t *p);
void recalc_all_priorities(void);
void context_switch_to(proc_entry_t *next);
void sched_init(void);

/* Fair-share helpers */
void fairshare_update(void);

#endif /* SCHEDULER_H */
