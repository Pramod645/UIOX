#include "uix_semaphore.h"
#include "uix_errno.h"
#include "sys/uix_time.h"
#include <stdarg.h>

int uix_sem_init(uix_sem_t *sem, int pshared, unsigned int value)
{
    if (!sem) { uix_errno = UIX_EINVAL; return -1; }
    sem->value   = (int)value;
    sem->pshared = pshared;
    return 0;
}

int uix_sem_destroy(uix_sem_t *sem)
    { if (!sem) { uix_errno=UIX_EINVAL; return -1; } return 0; }

int uix_sem_wait(uix_sem_t *sem)
{
    if (!sem) { uix_errno = UIX_EINVAL; return -1; }
    while (__atomic_load_n(&sem->value, __ATOMIC_SEQ_CST) <= 0) {
        /* busy-wait — real impl: futex_wait */
    }
    __atomic_fetch_sub(&sem->value, 1, __ATOMIC_SEQ_CST);
    return 0;
}

int uix_sem_trywait(uix_sem_t *sem)
{
    if (!sem) { uix_errno = UIX_EINVAL; return -1; }
    int v = __atomic_load_n(&sem->value, __ATOMIC_SEQ_CST);
    if (v <= 0) { uix_errno = UIX_EAGAIN; return -1; }
    __atomic_fetch_sub(&sem->value, 1, __ATOMIC_SEQ_CST);
    return 0;
}

int uix_sem_timedwait(uix_sem_t *sem, const uix_timespec_t *abs)
{
    (void)abs;
    return uix_sem_trywait(sem);
}

int uix_sem_post(uix_sem_t *sem)
{
    if (!sem) { uix_errno = UIX_EINVAL; return -1; }
    __atomic_fetch_add(&sem->value, 1, __ATOMIC_SEQ_CST);
    return 0;
}

int uix_sem_getvalue(uix_sem_t *sem, int *sval)
{
    if (!sem || !sval) { uix_errno = UIX_EINVAL; return -1; }
    *sval = __atomic_load_n(&sem->value, __ATOMIC_SEQ_CST);
    return 0;
}

uix_sem_t *uix_sem_open(const char *name, int oflag, ...)
{
    (void)name; (void)oflag;
    uix_errno = UIX_ENOSYS;
    return UIX_SEM_FAILED;
}

int uix_sem_close(uix_sem_t *sem)  { (void)sem; return 0; }
int uix_sem_unlink(const char *n)  { (void)n; return 0; }

/* ***This is End of file, there is no more line should be added after this line*** */
