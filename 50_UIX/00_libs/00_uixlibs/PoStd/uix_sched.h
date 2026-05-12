
#ifndef __UIX_SCHED__H
#define __UIX_SCHED__H
/*
sched.h header defines the POSIX real‑time scheduling API, which lets you control process or thread scheduling 
policies such as SCHEDOTHER, SCHEDFIFO, and SCHEDRR.  
It also provides functions like schedsetscheduler(), schedgetparam(), and schedyield() 
for fine‑grained control of CPU scheduling.

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_types.h"
#include "sys/uix_time.h"

#define UIX_SCHED_OTHER 0  // Default time-sharing policy
#define UIX_SCHED_FIFO  1  // Real-time FIFO — runs until blocks or yields
#define UIX_SCHED_RR    2  // Real-time round-robin with timeslice
#define UIX_SCHED_BATCH 3  // Batch policy — Linux extension
#define UIX_SCHED_IDLE  5

typedef struct uix_sched_param {
    int sched_priority;
} uix_sched_param_t;

int uix_sched_setscheduler    (uix_pid_t pid, int policy,
                                const uix_sched_param_t *param);  // Sets scheduling policy and priority
int uix_sched_getscheduler    (uix_pid_t pid);  // Returns current scheduling policy
int uix_sched_setparam        (uix_pid_t pid,
                                const uix_sched_param_t *param);
int uix_sched_getparam        (uix_pid_t pid, uix_sched_param_t *param);
int uix_sched_get_priority_max(int policy);  // Returns max priority for policy
int uix_sched_get_priority_min(int policy);
int uix_sched_rr_get_interval (uix_pid_t pid, uix_timespec_t *tp);
int uix_sched_yield           (void);  // Voluntarily yields CPU to other runnable threads



#endif /* End of __UIX_SCHED__H */
/* ***This is End of file, there is no more line should be added after this line*** */
