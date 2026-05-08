
#ifndef __THREADS__H
#define __THREADS__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"

#if  (define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif



#ifdef cplusplus
}
#endif


#endif /* End  of STDLIB*/

/* include/uix_threads.h — C11 <threads.h> */
#ifndef UIX_THREADS_H
#define UIX_THREADS_H

#include "uix_types.h"
#include "uix_time.h"
#include "uix_pthread.h"

typedef uix_pthread_t      uix_thrd_t;
typedef uix_pthread_mutex_t uix_mtx_t;
typedef uix_pthread_cond_t  uix_cnd_t;
typedef uix_pthread_key_t   uix_tss_t;
typedef uix_pthread_once_t  uix_once_flag;
typedef int (*uix_thrd_start_t)(void *);

#define UIX_ONCE_FLAG_INIT    0
#define UIX_TSS_DTOR_ITERATIONS 4

/* Return codes */
#define UIX_thrd_success  0
#define UIX_thrd_error    1
#define UIX_thrd_nomem    2
#define UIX_thrd_timedout 3
#define UIX_thrd_busy     4

/* Mutex types */
#define UIX_mtx_plain     0
#define UIX_mtx_recursive 1
#define UIX_mtx_timed     2

int         uix_thrd_create (uix_thrd_t *thr, uix_thrd_start_t func, void *arg); // Creates thread — wraps pthread_create, adapter converts int return
int         uix_thrd_join   (uix_thrd_t thr, int *res);  // Joins thread, gets int exit code
int         uix_thrd_detach (uix_thrd_t thr);
void        uix_thrd_exit   (int res) __attribute__((noreturn));  // Exits thread with int code
uix_thrd_t  uix_thrd_current(void);
int         uix_thrd_equal  (uix_thrd_t a, uix_thrd_t b);
int         uix_thrd_sleep  (const uix_timespec_t *d, uix_timespec_t *rem);  // Sleeps using nanosleep
void        uix_thrd_yield  (void);  // Yields CPU — calls sched_yield()

int  uix_mtx_init   (uix_mtx_t *m, int type);  // Initializes mutex
void uix_mtx_destroy(uix_mtx_t *m);
int  uix_mtx_lock   (uix_mtx_t *m);
int  uix_mtx_trylock(uix_mtx_t *m);
int  uix_mtx_unlock (uix_mtx_t *m);
int  uix_mtx_timedlock(uix_mtx_t *m, const uix_timespec_t *ts);

int  uix_cnd_init      (uix_cnd_t *c);
void uix_cnd_destroy   (uix_cnd_t *c);
int  uix_cnd_wait      (uix_cnd_t *c, uix_mtx_t *m);  // Waits on condition
int  uix_cnd_timedwait (uix_cnd_t *c, uix_mtx_t *m, const uix_timespec_t *ts);
int  uix_cnd_signal    (uix_cnd_t *c);
int  uix_cnd_broadcast (uix_cnd_t *c);

int  uix_tss_create(uix_tss_t *key, void (*dtor)(void *));  // Thread-local storage key
void uix_tss_delete(uix_tss_t key);
void *uix_tss_get  (uix_tss_t key);
int  uix_tss_set   (uix_tss_t key, void *val);

void uix_call_once(uix_once_flag *flag, void (*func)(void));  // One-time initialization

#endif /* UIX_THREADS_H */



#endif /* End of __THREADS__H */
/* ***This is End of file, there is no more line should be added after this line*** */