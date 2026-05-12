
#ifndef __UIX_PTHREAD__H
#define __UIX_PTHREAD__H
/*
pthread.h is the POSIX Threads header, defining the entire Pthreads API for multithreading in C. 
It provides thread creation, synchronization (mutexes, condition variables), and management functions 
that form the foundation of concurrent programming on Unix-like systems.

*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_types.h"
#include "sys/uix_time.h"

typedef uix_uint64_t uix_pthread_t;   // Thread identifier
typedef uix_uint64_t uix_pthread_mutex_t;  // Mutex lock
typedef uix_uint64_t uix_pthread_cond_t;  // Condition variable
typedef uix_uint64_t uix_pthread_rwlock_t;
typedef uix_uint64_t uix_pthread_spinlock_t;
typedef uix_uint32_t uix_pthread_key_t;
typedef uix_uint32_t uix_pthread_once_t;

typedef struct uix_pthread_attr {
    uix_size_t stacksize;
    int        detachstate;
    int        schedpolicy;
    int        inheritsched;
    int        scope;
} uix_pthread_attr_t;

typedef struct uix_pthread_mutexattr {
    int type;
    int robust;
    int pshared;
} uix_pthread_mutexattr_t;

typedef struct uix_pthread_condattr {
    int pshared;
    int clock;
} uix_pthread_condattr_t;

#define UIX_PTHREAD_MUTEX_NORMAL      0
#define UIX_PTHREAD_MUTEX_RECURSIVE   1
#define UIX_PTHREAD_MUTEX_ERRORCHECK  2
#define UIX_PTHREAD_ONCE_INIT         0
#define UIX_PTHREAD_MUTEX_INITIALIZER 0  // Static mutex initializer
#define UIX_PTHREAD_COND_INITIALIZER  0
#define UIX_PTHREAD_CREATE_JOINABLE   0   // Thread can be joined
#define UIX_PTHREAD_CREATE_DETACHED   1
#define UIX_PTHREAD_CANCEL_ENABLE     0
#define UIX_PTHREAD_CANCEL_DISABLE    1
#define UIX_PTHREAD_CANCELED          ((void*)-1)

int            uix_pthread_create      (uix_pthread_t *t,
                                         const uix_pthread_attr_t *a,
                                         void *(*fn)(void *), void *arg);  // Creates new thread executing fn(arg)
int            uix_pthread_join        (uix_pthread_t t, void **retval);  // Waits for thread to exit, gets return value
int            uix_pthread_detach      (uix_pthread_t t);    // Marks thread resources for automatic release
void           uix_pthread_exit        (void *retval) __attribute__((noreturn)); //Terminates calling thread
uix_pthread_t  uix_pthread_self        (void);  // Returns calling thread's ID
int            uix_pthread_equal       (uix_pthread_t t1, uix_pthread_t t2);
int            uix_pthread_cancel      (uix_pthread_t t);
int            uix_pthread_kill        (uix_pthread_t t, int sig);
int            uix_pthread_once        (uix_pthread_once_t *oc,
                                         void (*init)(void));  // Runs fn exactly once across all threads

int            uix_pthread_mutex_init    (uix_pthread_mutex_t *m,
                                           const uix_pthread_mutexattr_t *a);
int            uix_pthread_mutex_destroy (uix_pthread_mutex_t *m);
int            uix_pthread_mutex_lock    (uix_pthread_mutex_t *m); // Acquires mutex — blocks if held
int            uix_pthread_mutex_trylock (uix_pthread_mutex_t *m); // Non-blocking acquire — returns EBUSY if locked
int            uix_pthread_mutex_unlock  (uix_pthread_mutex_t *m); // Releases mutex
int            uix_pthread_mutex_timedlock(uix_pthread_mutex_t *m,
                                            const uix_timespec_t *abs);

int            uix_pthread_cond_init     (uix_pthread_cond_t *c,
                                           const uix_pthread_condattr_t *a);
int            uix_pthread_cond_destroy  (uix_pthread_cond_t *c);
int            uix_pthread_cond_wait     (uix_pthread_cond_t *c,
                                           uix_pthread_mutex_t *m);  // Atomically releases mutex and waits for signal
int            uix_pthread_cond_timedwait(uix_pthread_cond_t *c,
                                           uix_pthread_mutex_t *m,
                                           const uix_timespec_t *t);
int            uix_pthread_cond_signal   (uix_pthread_cond_t *c);  // Wakes one waiter
int            uix_pthread_cond_broadcast(uix_pthread_cond_t *c);  // Wakes all waiters

int            uix_pthread_key_create    (uix_pthread_key_t *k,
                                           void (*dtor)(void *));  // Creates thread-local storage key
int            uix_pthread_key_delete    (uix_pthread_key_t k);
void          *uix_pthread_getspecific   (uix_pthread_key_t k);  // Retrieves thread-local value
int            uix_pthread_setspecific   (uix_pthread_key_t k,
                                           const void *val); // Stores thread-local value

int            uix_pthread_attr_init          (uix_pthread_attr_t *a);
int            uix_pthread_attr_destroy       (uix_pthread_attr_t *a);
int            uix_pthread_attr_setstacksize  (uix_pthread_attr_t *a,
                                               uix_size_t ss);
int            uix_pthread_attr_getstacksize  (const uix_pthread_attr_t *a,
                                               uix_size_t *ss);
int            uix_pthread_attr_setdetachstate(uix_pthread_attr_t *a, int d);
int            uix_pthread_attr_getdetachstate(const uix_pthread_attr_t *a,
                                               int *d);



#endif /* End of __UIX_PTHREAD__H */
/* ***This is End of file, there is no more line should be added after this line*** */
