/*
 * 30_KIX/33_PCS/40_procStruct/include/process.h
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: #include <stdint.h>   → uiox_klibc.h provides uint*_t
 *   REMOVED: #include <setjmp.h>   → no setjmp in freestanding;
 *                                     jmp_buf replaced by uiox_jmp_buf_t
 *                                     (defined in proc_algo.h)
 *   REMOVED: #include <time.h>     → uiox_klibc.h provides clock_t / time_t
 *
 * All struct fields, enum values, and constants are identical to the
 * original — zero algorithm changes.
 *
 * @version 2.0.0  @date 2026-07-24
 */
#ifndef PROCESS_H
#define PROCESS_H

#include "uiox_klibc.h"   /* replaces <stdint.h>  <time.h>  */
/* Note: <setjmp.h> removed — jmp_buf replaced by uiox_jmp_buf_t
   which is defined in proc_algo.h (included after this header). */

/* ── Constants ───────────────────────────────────────────────── */
#define NPROC           64
#define NOFILE          20
#define NSIG            32
#define KERNEL_STACK_SZ 4096

/* ── Process States ──────────────────────────────────────────── */
typedef enum proc_state {
    PROC_UNUSED         = 0,
    PROC_USER_RUNNING   = 1,
    PROC_KERNEL_RUNNING = 2,
    PROC_READY          = 3,
    PROC_SLEEP_MEM      = 4,
    PROC_READY_SWAPPED  = 5,
    PROC_SLEEP_SWAPPED  = 6,
    PROC_PREEMPTED      = 7,
    PROC_CREATED        = 8,
    PROC_ZOMBIE         = 9
} proc_state_t;

/* ── Signal Numbers ──────────────────────────────────────────── */
typedef enum signal_num {
    SIG_NONE  =  0,
    SIGHUP    =  1,
    SIGINT    =  2,
    SIGQUIT   =  3,
    SIGILL    =  4,
    SIGTRAP   =  5,
    SIGABRT   =  6,
    SIGKILL   =  9,
    SIGPIPE   = 13,
    SIGALRM   = 14,
    SIGTERM   = 15,
    SIGCHLD   = 17,
    SIGCONT   = 18,
    SIGSTOP   = 19
} signal_num_t;

/* ── Scheduling Parameters ───────────────────────────────────── */
typedef struct sched_param {
    int  p_pri;    /* scheduling priority (lower = higher) */
    int  p_cpu;    /* CPU usage for priority calculation   */
    int  p_nice;   /* user-set priority offset             */
    int  p_time;   /* residence time for scheduling        */
} sched_param_t;

/* ── Timer Record ────────────────────────────────────────────── */
typedef struct proc_timer {
    clock_t  p_utime;    /* user mode CPU time               */
    clock_t  p_stime;    /* kernel CPU time                  */
    clock_t  p_cutime;   /* children's user time             */
    clock_t  p_cstime;   /* children's kernel time           */
} proc_timer_t;

/* ── Process Table Entry ─────────────────────────────────────── */
typedef struct proc {
    proc_state_t   p_state;
    uint32_t       p_pid;
    uint32_t       p_ppid;
    uint16_t       p_uid;    uint16_t p_euid;
    uint16_t       p_gid;    uint16_t p_egid;
    uint32_t       p_size;
    void          *p_addr;
    uintptr_t      p_wchan;
    sched_param_t  p_sched;
    uint32_t       p_sig;
    uint32_t       p_sigmask;
    proc_timer_t   p_timers;
    int            p_flag;
    int            p_exit_code;
    struct proc   *p_next;
    struct proc   *p_prev;
} proc_t;

/* ── Process Flags ───────────────────────────────────────────── */
#define P_LOADED    0x0001
#define P_STICKY    0x0002
#define P_INTERR    0x0004
#define P_SIGCATCH  0x0008
#define P_SWAPPED   0x0010
#define P_TRACED    0x0020
#define P_WAITED    0x0040
/* Aliases used by process.h consumers */
#define P_ZOMBIE    P_WAITED   /* process awaiting reap        */

/* ── Sleep Priority Levels ───────────────────────────────────── */
#define PSWP    0
#define PINOD   10
#define PRIBIO  20
#define PZERO   25
#define PWAIT   30
#define PSLEP   40
#define PUSER   50

/* ── Globals ─────────────────────────────────────────────────── */
extern proc_t  proc_table[NPROC];
extern proc_t *current_proc;

/* ── Prototypes ──────────────────────────────────────────────── */
proc_t *proc_alloc  (uint32_t pid, uint32_t ppid);
void    proc_free   (proc_t *p);
proc_t *proc_find   (uint32_t pid);
int     proc_set_state(proc_t *p, proc_state_t new_state);
int     proc_signal_pending(proc_t *p);
void    sched_enqueue(proc_t *p);
void    sched_dequeue(proc_t *p);

#endif /* PROCESS_H */
