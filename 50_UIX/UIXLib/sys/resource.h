
#ifndef __SYS_RESOURCE__H
#define __SYS_RESOURCE__H
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

#endif /* End of __SYS_RESOURCE__H */
/* ***This is End of file, there is no more line should be added after this line*** */