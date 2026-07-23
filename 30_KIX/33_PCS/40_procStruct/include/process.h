/*
 * 30_KIX/33_PCS/40_procStruct/include/process.h
 * REMOVED: #include <stdint.h> / uix_types.h
 * REPLACED WITH: #include "uiox_klibc.h"
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_PROCESS_H
#define UIOX_PROCESS_H

#include "uiox_klibc.h"

#define NPROC           64
#define NOFILE          20
#define NSIG            32
#define KERNEL_STACK_SZ 4096

typedef enum { P_FREE=0, P_SLEEP, P_WAIT, P_RUN, P_IDLE, P_ZOMBIE } proc_state_t;

typedef struct { int sp_policy; int sp_priority; int sp_nice; int sp_ticks_left; } sched_param_t;

typedef struct proc_timer {
    clock_t p_utime; clock_t p_stime; clock_t p_cutime; clock_t p_cstime;
} proc_timer_t;

typedef struct proc {
    proc_state_t   p_state;
    uint32_t       p_pid;
    uint32_t       p_ppid;
    uint16_t       p_uid;   uint16_t p_euid;
    uint16_t       p_gid;   uint16_t p_egid;
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

#define P_LOADED   0x0001
#define P_STICKY   0x0002
#define P_INTERR   0x0004
#define P_SIGCATCH 0x0008
#define PSWP  10
#define PINOD 20
#define PRIBIO 28
#define PWAIT 30
#define PSLEP 40
#define PUSER 50

#endif /* UIOX_PROCESS_H */
