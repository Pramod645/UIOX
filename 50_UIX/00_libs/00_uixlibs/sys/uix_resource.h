
#ifndef __SYS_UIX_RESOURCE__H
#define __SYS_UIX_RESOURCE__H
/*
sys/resource.h 
*/
/* This is for only POXIS */

#include "uix_features.h" //??

#include "uix_types.h"
#include "uix_time.h"

#define UIX_RLIMIT_CPU     0   //Max CPU time in seconds
#define UIX_RLIMIT_FSIZE   1   // Max file size in bytes
#define UIX_RLIMIT_DATA    2
#define UIX_RLIMIT_STACK   3     // Max stack size (default 8MB)
#define UIX_RLIMIT_CORE    4
#define UIX_RLIMIT_RSS     5
#define UIX_RLIMIT_NPROC   6
#define UIX_RLIMIT_NOFILE  7        // Max number of open file descriptors
#define UIX_RLIMIT_MEMLOCK 8
#define UIX_RLIMIT_AS      9
#define UIX_RLIM_NLIMITS   10
#define UIX_RLIM_INFINITY  (~0UL)      // No limit

#define UIX_RUSAGE_SELF     0
#define UIX_RUSAGE_CHILDREN (-1)

typedef struct uix_rlimit {
    uix_uint64_t rlim_cur;    // Soft limit — current enforced limit
    uix_uint64_t rlim_max;     // Hard limit — maximum soft limit can be raised to
} uix_rlimit_t;

typedef struct uix_rusage {
    uix_timeval_t ru_utime;   // User CPU time used
    uix_timeval_t ru_stime;
    long ru_maxrss;
    long ru_minflt;          // Minor page faults (no I/O required)
    long ru_majflt;         // Major page faults (I/O required)
    long ru_inblock;
    long ru_oublock;
    long ru_nvcsw;
    long ru_nivcsw;
} uix_rusage_t;

int uix_getrlimit  (int resource, uix_rlimit_t *rlim);     // Gets resource limit
int uix_setrlimit  (int resource, const uix_rlimit_t *rlim);    // Sets resource limit — soft must not exceed hard
int uix_getrusage  (int who, uix_rusage_t *usage);         // Gets resource usage for self or children
int uix_getpriority(int which, int who);            // Gets scheduling priority (nice value)
int uix_setpriority(int which, int who, int prio);          // Sets scheduling priority



#endif /* End of __SYS_UIX_RESOURCE__H */
/* ***This is End of file, there is no more line should be added after this line*** */
