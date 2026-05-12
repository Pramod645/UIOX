/********************* uix_shm.c **************************************/
#include "uix_shm.h"
#include "../uix_errno.h"

int uix_shmget(uix_key_t key, uix_size_t size, int shmflg)
{
    extern int sys_shmget(uix_key_t,uix_size_t,int) __attribute__((weak));
    if (sys_shmget) return sys_shmget(key, size, shmflg);
    uix_errno = UIX_ENOSYS; return -1;
}

void *uix_shmat(int shmid, const void *shmaddr, int shmflg)
{
    extern void *sys_shmat(int,const void*,int) __attribute__((weak));
    if (sys_shmat) return sys_shmat(shmid, shmaddr, shmflg);
    uix_errno = UIX_ENOSYS; return (void*)-1;
}

int uix_shmdt(const void *shmaddr)
{
    extern int sys_shmdt(const void*) __attribute__((weak));
    if (sys_shmdt) return sys_shmdt(shmaddr);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
{
    extern int sys_shmctl(int,int,void*) __attribute__((weak));
    if (sys_shmctl) return sys_shmctl(shmid, cmd, buf);
    uix_errno = UIX_ENOSYS; return -1;
}

/* ***This is End of file, there is no more line should be added after this line*** */
