
#ifndef __UIX_SCHED__H
#define __UIX_SCHED__H
/*
sched.h header defines the POSIX real‑time scheduling API, which lets you control process or thread scheduling 
policies such as SCHEDOTHER, SCHEDFIFO, and SCHEDRR.  
It also provides functions like schedsetscheduler(), schedgetparam(), and schedyield() 
for fine‑grained control of CPU scheduling.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Scheduling policies /
#define SCHEDOTHER 0  // default time-sharing /
#define SCHEDFIFO  1  // first-in, first-out real-time /
#define SCHEDRR    2  // round-robin real-time /
#define SCHEDBATCH 3  // non-interactive, CPU-bound /
#define SCHEDIDLE  5  // very low priority background tasks /

// Scheduling parameter structure /
struct schedparam {
    int schedpriority;  // thread or process priority /
};

// Function prototypes /
int schedsetscheduler(pidt pid, int policy, const struct schedparam param);
int schedgetscheduler(pidt pid);
int schedsetparam(pidt pid, const struct schedparam param);
int schedgetparam(pidt pid, struct schedparam param);
int schedyield(void);
int schedgetprioritymax(int policy);
int schedgetprioritymin(int policy);
int schedrrgetinterval(pidt pid, struct timespec interval);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#ifndef UIX_SCHED_H
#define UIX_SCHED_H

#include "uix_types.h"
#include "uix_time.h"

#define UIX_SCHED_OTHER 0
#define UIX_SCHED_FIFO  1
#define UIX_SCHED_RR    2
#define UIX_SCHED_BATCH 3
#define UIX_SCHED_IDLE  5

typedef struct uix_sched_param {
    int sched_priority;
} uix_sched_param_t;

int uix_sched_setscheduler    (uix_pid_t pid, int policy,
                                const uix_sched_param_t *param);
int uix_sched_getscheduler    (uix_pid_t pid);
int uix_sched_setparam        (uix_pid_t pid,
                                const uix_sched_param_t *param);
int uix_sched_getparam        (uix_pid_t pid, uix_sched_param_t *param);
int uix_sched_get_priority_max(int policy);
int uix_sched_get_priority_min(int policy);
int uix_sched_rr_get_interval (uix_pid_t pid, uix_timespec_t *tp);
int uix_sched_yield           (void);

#endif /* UIX_SCHED_H */



#endif /* End of __UIX_SCHED__H */
/* ***This is End of file, there is no more line should be added after this line*** */