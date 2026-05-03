#include "uix_pthread.h"
#include "uix_errno.h"
#include "uix_string.h"
#include "sys/uix_time.h"

static uix_pthread_t current_thread_id = 1;

int uix_pthread_create(uix_pthread_t *thread,
                       const uix_pthread_attr_t *attr,
                       void *(*start)(void *), void *arg)
{
    extern int sys_clone(unsigned long, void*) __attribute__((weak));
    if (!thread || !start) { uix_errno = UIX_EINVAL; return -1; }
    (void)attr;
    *thread = ++current_thread_id;
    /* In real UIOX: clone() with CLONE_THREAD flags */
    (void)arg;
    return 0;
}

int uix_pthread_join(uix_pthread_t thread, void **retval)
{
    (void)thread;
    if (retval) *retval = NULL;
    return 0;
}

int uix_pthread_detach(uix_pthread_t thread)
    { (void)thread; return 0; }

void uix_pthread_exit(void *retval)
    { (void)retval; while(1) {} }

uix_pthread_t uix_pthread_self(void) { return current_thread_id; }

int uix_pthread_equal(uix_pthread_t t1, uix_pthread_t t2)
    { return t1 == t2; }

int uix_pthread_cancel(uix_pthread_t thread)
    { (void)thread; return 0; }

int uix_pthread_kill(uix_pthread_t thread, int sig)
    { (void)thread; (void)sig; return 0; }

int uix_pthread_once(uix_pthread_once_t *once_control,
                     void (*init_routine)(void))
{
    if (!once_control || !init_routine)
        { uix_errno = UIX_EINVAL; return -1; }
    if (*once_control == 0) { *once_control = 1; init_routine(); }
    return 0;
}

/* ── Mutex ──────────────────────────────────────────────────── */
int uix_pthread_mutex_init(uix_pthread_mutex_t *m,
                           const uix_pthread_mutexattr_t *a)
    { if (!m) return UIX_EINVAL; *m = 0; (void)a; return 0; }
int uix_pthread_mutex_destroy(uix_pthread_mutex_t *m)
    { (void)m; return 0; }
int uix_pthread_mutex_lock(uix_pthread_mutex_t *m)
{
    if (!m) return UIX_EINVAL;
    while (__atomic_exchange_n((int*)m, 1, __ATOMIC_SEQ_CST)) {}
    return 0;
}
int uix_pthread_mutex_trylock(uix_pthread_mutex_t *m)
{
    if (!m) return UIX_EINVAL;
    return __atomic_exchange_n((int*)m, 1, __ATOMIC_SEQ_CST)
           ? UIX_EBUSY : 0;
}
int uix_pthread_mutex_unlock(uix_pthread_mutex_t *m)
{
    if (!m) return UIX_EINVAL;
    __atomic_store_n((int*)m, 0, __ATOMIC_SEQ_CST);
    return 0;
}
int uix_pthread_mutex_timedlock(uix_pthread_mutex_t *m,
                                const uix_timespec_t *abs)
    { (void)abs; return uix_pthread_mutex_lock(m); }

/* ── Condition variable ─────────────────────────────────────── */
int uix_pthread_cond_init(uix_pthread_cond_t *c,
                          const uix_pthread_condattr_t *a)
    { if (!c) return UIX_EINVAL; *c = 0; (void)a; return 0; }
int uix_pthread_cond_destroy(uix_pthread_cond_t *c)
    { (void)c; return 0; }
int uix_pthread_cond_wait(uix_pthread_cond_t *c,
                          uix_pthread_mutex_t *m)
{
    uix_pthread_mutex_unlock(m);
    /* In real UIOX: futex_wait(); */
    uix_pthread_mutex_lock(m);
    (void)c; return 0;
}
int uix_pthread_cond_timedwait(uix_pthread_cond_t *c,
                               uix_pthread_mutex_t *m,
                               const uix_timespec_t *t)
    { (void)t; return uix_pthread_cond_wait(c, m); }
int uix_pthread_cond_signal(uix_pthread_cond_t *c)
    { (void)c; return 0; }
int uix_pthread_cond_broadcast(uix_pthread_cond_t *c)
    { (void)c; return 0; }

/* ── Thread-specific data ───────────────────────────────────── */
#define UIX_PTHREAD_KEYS_MAX 128
static void *tsd_values[UIX_PTHREAD_KEYS_MAX];
static void (*tsd_dtors[UIX_PTHREAD_KEYS_MAX])(void *);
static int tsd_used[UIX_PTHREAD_KEYS_MAX];

int uix_pthread_key_create(uix_pthread_key_t *key,
                           void (*dtor)(void *))
{
    for (int i = 0; i < UIX_PTHREAD_KEYS_MAX; i++) {
        if (!tsd_used[i]) {
            tsd_used[i]  = 1;
            tsd_dtors[i] = dtor;
            tsd_values[i] = NULL;
            *key = (uix_pthread_key_t)i;
            return 0;
        }
    }
    return UIX_EAGAIN;
}

int uix_pthread_key_delete(uix_pthread_key_t key)
{
    if (key >= UIX_PTHREAD_KEYS_MAX) return UIX_EINVAL;
    tsd_used[key]   = 0;
    tsd_dtors[key]  = NULL;
    tsd_values[key] = NULL;
    return 0;
}

void *uix_pthread_getspecific(uix_pthread_key_t key)
{
    return key < UIX_PTHREAD_KEYS_MAX ? tsd_values[key] : NULL;
}

int uix_pthread_setspecific(uix_pthread_key_t key, const void *val)
{
    if (key >= UIX_PTHREAD_KEYS_MAX) return UIX_EINVAL;
    tsd_values[key] = (void *)val;
    return 0;
}

/* ── Attribute functions ────────────────────────────────────── */
int uix_pthread_attr_init(uix_pthread_attr_t *a)
{
    if (!a) return UIX_EINVAL;
    a->stacksize   = 8*1024*1024;
    a->detachstate = UIX_PTHREAD_CREATE_JOINABLE;
    return 0;
}
int uix_pthread_attr_destroy(uix_pthread_attr_t *a)
    { (void)a; return 0; }
int uix_pthread_attr_setstacksize(uix_pthread_attr_t *a,
                                  uix_size_t ss)
    { if (!a) return UIX_EINVAL; a->stacksize=ss; return 0; }
int uix_pthread_attr_getstacksize(const uix_pthread_attr_t *a,
                                  uix_size_t *ss)
    { if (!a||!ss) return UIX_EINVAL; *ss=a->stacksize; return 0; }
int uix_pthread_attr_setdetachstate(uix_pthread_attr_t *a, int d)
    { if (!a) return UIX_EINVAL; a->detachstate=d; return 0; }
int uix_pthread_attr_getdetachstate(const uix_pthread_attr_t *a,
                                    int *d)
    { if (!a||!d) return UIX_EINVAL; *d=a->detachstate; return 0; }
