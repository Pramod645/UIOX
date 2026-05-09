/********************** uix_threads.c *******************************************/
#include "uix_threads.h"
#include "uix_errno.h"
#include "uix_stdlib.h"

typedef struct { uix_thrd_start_t fn; void *arg; } _thrd_arg_t;

static void *_thrd_wrapper(void *arg)
{
    _thrd_arg_t *a = (_thrd_arg_t*)arg;
    uix_thrd_start_t fn = a->fn;
    void *fn_arg = a->arg;
    uix_free(a);
    return (void*)(uix_intptr_t)fn(fn_arg);
}

int uix_thrd_create(uix_thrd_t *thr, uix_thrd_start_t fn, void *arg)
{
    _thrd_arg_t *a = (_thrd_arg_t*)uix_malloc(sizeof(*a));
    if (!a) return UIX_thrd_nomem;
    a->fn = fn; a->arg = arg;
    return uix_pthread_create(thr, NULL, _thrd_wrapper, a) == 0
           ? UIX_thrd_success : UIX_thrd_error;
}

int uix_thrd_join(uix_thrd_t thr, int *res)
{
    void *ret;
    if (uix_pthread_join(thr, &ret)) return UIX_thrd_error;
    if (res) *res = (int)(uix_intptr_t)ret;
    return UIX_thrd_success;
}

int  uix_thrd_detach (uix_thrd_t t) { return uix_pthread_detach(t)?UIX_thrd_error:UIX_thrd_success; }
void uix_thrd_exit   (int r) { uix_pthread_exit((void*)(uix_intptr_t)r); }
uix_thrd_t uix_thrd_current(void) { return uix_pthread_self(); }
int  uix_thrd_equal  (uix_thrd_t a, uix_thrd_t b) { return uix_pthread_equal(a,b); }
int  uix_thrd_sleep  (const uix_timespec_t *d, uix_timespec_t *rem)
    { return uix_nanosleep(d,rem)==0?UIX_thrd_success:UIX_thrd_error; }
void uix_thrd_yield  (void) { uix_sched_yield(); }

int  uix_mtx_init    (uix_mtx_t *m, int t) { (void)t; return uix_pthread_mutex_init(m,NULL)?UIX_thrd_error:UIX_thrd_success; }
void uix_mtx_destroy (uix_mtx_t *m) { uix_pthread_mutex_destroy(m); }
int  uix_mtx_lock    (uix_mtx_t *m) { return uix_pthread_mutex_lock(m)?UIX_thrd_error:UIX_thrd_success; }
int  uix_mtx_trylock (uix_mtx_t *m) { int r=uix_pthread_mutex_trylock(m); return r==0?UIX_thrd_success:(r==UIX_EBUSY?UIX_thrd_busy:UIX_thrd_error); }
int  uix_mtx_unlock  (uix_mtx_t *m) { return uix_pthread_mutex_unlock(m)?UIX_thrd_error:UIX_thrd_success; }
int  uix_mtx_timedlock(uix_mtx_t *m, const uix_timespec_t *ts)
    { return uix_pthread_mutex_timedlock(m,ts)?UIX_thrd_error:UIX_thrd_success; }

int  uix_cnd_init      (uix_cnd_t *c) { return uix_pthread_cond_init(c,NULL)?UIX_thrd_error:UIX_thrd_success; }
void uix_cnd_destroy   (uix_cnd_t *c) { uix_pthread_cond_destroy(c); }
int  uix_cnd_wait      (uix_cnd_t *c, uix_mtx_t *m) { return uix_pthread_cond_wait(c,m)?UIX_thrd_error:UIX_thrd_success; }
int  uix_cnd_timedwait (uix_cnd_t *c, uix_mtx_t *m, const uix_timespec_t *ts)
    { return uix_pthread_cond_timedwait(c,m,ts)?UIX_thrd_error:UIX_thrd_success; }
int  uix_cnd_signal    (uix_cnd_t *c) { return uix_pthread_cond_signal(c)?UIX_thrd_error:UIX_thrd_success; }
int  uix_cnd_broadcast (uix_cnd_t *c) { return uix_pthread_cond_broadcast(c)?UIX_thrd_error:UIX_thrd_success; }

int   uix_tss_create(uix_tss_t *k, void(*d)(void*)) { return uix_pthread_key_create(k,d)?UIX_thrd_error:UIX_thrd_success; }
void  uix_tss_delete(uix_tss_t k) { uix_pthread_key_delete(k); }
void *uix_tss_get   (uix_tss_t k) { return uix_pthread_getspecific(k); }
int   uix_tss_set   (uix_tss_t k, void *v) { return uix_pthread_setspecific(k,v)?UIX_thrd_error:UIX_thrd_success; }

void uix_call_once(uix_once_flag *flag, void (*func)(void))
    { uix_pthread_once(flag, func); }


/* ***This is End of file, there is no more line should be added after this line*** */
