
#ifndef __SYS_UIX_RESOURCE__H
#define __SYS_UIX_RESOURCE__H
/*
sys/stat.h 
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <time.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Resource limit type /
typedef unsigned long rlimt;

// Special limit value: infinite /
#define RLIMINFINITY ((rlimt)~0UL)

// Resource limit structure /
struct rlimit {
    rlimt rlimcur; // soft limit /
    rlimt rlimmax; // hard limit /
};

// Resource limit selectors /
#define RLIMITCPU        0
#define RLIMITFSIZE      1
#define RLIMITDATA       2
#define RLIMITSTACK      3
#define RLIMITCORE       4
#define RLIMITRSS        5
#define RLIMITNPROC      6
#define RLIMITNOFILE     7
#define RLIMITMEMLOCK    8
#define RLIMITAS         9
#define RLIMITLOCKS      10
#define RLIMITSIGPENDING 11
#define RLIMITMSGQUEUE   12
#define RLIMITNICE       13
#define RLIMITRTPRIO     14
#define RLIMITRTTIME     15

// Who selector for getpriority/setpriority /
#define PRIOPROCESS 0
#define PRIOPGRP    1
#define PRIOUSER    2

// Simplified timeval /
#ifndef STRUCTTIMEVAL
#define STRUCTTIMEVAL
struct timeval {
    timet tvsec;
    long   tvusec;
};
#endif

// Resource usage structure /
struct rusage {
    struct timeval ruutime;   // user CPU time used /
    struct timeval rustime;   // system CPU time used /

    long rumaxrss;    // maximum resident set size /
    long ruixrss;     // integral shared memory size /
    long ruidrss;     // integral unshared data size /
    long ruisrss;     // integral unshared stack size /
    long ruminflt;    // page reclaims (soft page faults) /
    long rumajflt;    // page faults (hard page faults) /
    long runswap;     // swaps /
    long ruinblock;   // block input operations /
    long ruoublock;   // block output operations /
    long rumsgsnd;    // IPC messages sent /
    long rumsgrcv;    // IPC messages received /
    long runsignals;  // signals received /
    long runvcsw;     // voluntary context switches /
    long runivcsw;    // involuntary context switches /
};

// who values for getrusage() /
#define RUSAGESELF     0
#define RUSAGECHILDREN (-1)
#define RUSAGETHREAD   1   // Linux-specific /

// Function declarations /
int getrlimit(int resource, struct rlimit rlim);
int setrlimit(int resource, const struct rlimit rlim);

int getrusage(int who, struct rusage usage);

int getpriority(int which, idt who);
int setpriority(int which, idt who, int prio);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#ifndef UIX_RESOURCE_H
#define UIX_RESOURCE_H

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

#endif /* UIX_RESOURCE_H */



#endif /* End of __SYS_UIX_RESOURCE__H */
/* ***This is End of file, there is no more line should be added after this line*** */