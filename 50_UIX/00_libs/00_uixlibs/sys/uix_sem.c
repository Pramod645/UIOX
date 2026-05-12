/*******************  uix_sem.c ***************************************/

//only one process enters the “critical section” at a time — the semaphore enforces mutual exclusion.


#include "uix_sem.h"
#include "../uix_errno.h"
#include "../uix_stdarg.h"

int uix_semget(uix_key_t key, int nsems, int semflg)
{
    extern int sys_semget(uix_key_t,int,int) __attribute__((weak));
    if (sys_semget) return sys_semget(key, nsems, semflg);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_semop(int semid, uix_sembuf_t *sops, uix_size_t nsops)
{
    extern int sys_semop(int,void*,uix_size_t) __attribute__((weak));
    if (sys_semop) return sys_semop(semid, sops, nsops);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_semctl(int semid, int semnum, int cmd, ...)
{
    uix_va_list ap; uix_va_start(ap, cmd);
    void *arg = uix_va_arg(ap, void *);
    uix_va_end(ap);
    extern int sys_semctl(int,int,int,void*) __attribute__((weak));
    if (sys_semctl) return sys_semctl(semid, semnum, cmd, arg);
    uix_errno = UIX_ENOSYS; return -1;
}


/* ***This is End of file, there is no more line should be added after this line*** */
