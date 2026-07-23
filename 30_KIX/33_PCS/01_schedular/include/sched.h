#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <stdint.h>
#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
#include "../../../../50_UIX/00_libs/00_uixlibs/PoStd/uix_sched.h"

#define MAX_PROCS  64
#define TIME_SLICE 10

typedef struct {
    int       sp_in_use;
    uix_pid_t sp_pid;
    int       sp_policy;
    int       sp_priority;
    int       sp_ticks_left;
    int       sp_state;
} sched_proc_t;

#define PROC_READY   0
#define PROC_RUNNING 1
#define PROC_BLOCKED 2
#define PROC_ZOMBIE  3

int       kernel_sched_setscheduler(uix_pid_t pid, int policy,
                                     const uix_sched_param_t *p);
int       kernel_sched_getscheduler(uix_pid_t pid);
int       kernel_sched_yield       (void);
int       sched_add_proc           (uix_pid_t pid, int policy, int priority);
int       sched_remove_proc        (uix_pid_t pid);
int       sched_tick               (void);
uix_pid_t sched_next               (void);

#endif /* KERNEL_SCHED_H */
