
#ifndef __PTHREAD__H
#define __PTHREAD__H
/*
pthread.h is the POSIX Threads header, defining the entire Pthreads API for multithreading in C. 
It provides thread creation, synchronization (mutexes, condition variables), and management functions 
that form the foundation of concurrent programming on Unix-like systems.

*/
/* This is for only POXIS */

#include "features.h"

#include <time.h>
#include <sched.h>
#include <signal.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Opaque types for threads and synchronization /
typedef unsigned long pthreadt;
typedef struct { int _dummy; } pthreadattrt;
typedef struct { int lock; } pthreadmutext;
typedef struct { int dummy; } pthreadmutexattrt;
typedef struct { int dummy; } pthreadcondt;
typedef struct { int dummy; } pthreadcondattrt;

// Thread creation and control /
int pthreadcreate(pthreadt thread,
                   const pthreadattrt attr,
                   void (startroutine)(void ),
                   void arg);
int pthreadjoin(pthreadt thread, void *retval);
void pthreadexit(void retval);
pthreadt pthreadself(void);
int pthreadequal(pthreadt t1, pthreadt t2);

// Mutex operations /
int pthreadmutexinit(pthreadmutext mutex,
                       const pthreadmutexattrt attr);
int pthreadmutexdestroy(pthreadmutext mutex);
int pthreadmutexlock(pthreadmutext mutex);
int pthreadmutextrylock(pthreadmutext mutex);
int pthreadmutexunlock(pthreadmutext mutex);

// Condition variable operations /
int pthreadcondinit(pthreadcondt cond,
                      const pthreadcondattrt attr);
int pthreadconddestroy(pthreadcondt cond);
int pthreadcondwait(pthreadcondt cond,
                      pthreadmutext mutex);
int pthreadcondsignal(pthreadcondt cond);
int pthreadcondbroadcast(pthreadcondt cond);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __PTHREAD__H */
/* ***This is End of file, there is no more line should be added after this line*** */