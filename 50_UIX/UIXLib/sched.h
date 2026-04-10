
#ifndef __SCHED__H
#define __SCHED__H
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

#endif /* End of __SCHED__H */
/* ***This is End of file, there is no more line should be added after this line*** */