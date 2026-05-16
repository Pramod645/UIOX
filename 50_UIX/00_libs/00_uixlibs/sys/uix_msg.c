/********************** uix_msg.c ********************************************/
#include "uix_msg.h"
#include "../PoStd/uix_errno.h"

#include "../uix_sys.h"

int uix_msgget(uix_key_t key, int msgflg)
{
   // extern int sys_msgget(uix_key_t,int) __attribute__((weak));

    if (SYS_MSGGET) return sys_msgget(key, msgflg); 
    
    uix_errno = UIX_ENOSYS; 
    return -1;

}

int uix_msgsnd(int msqid, const void *msgp,
               uix_size_t msgsz, int msgflg)
{
    //extern int sys_msgsnd(int,const void*,uix_size_t,int)
    //    __attribute__((weak));
    if (SYS_MSGSND) 
        return sys_msgsnd(msqid,msgp,msgsz,msgflg);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_msgrcv(int msqid, void *msgp, uix_size_t msgsz,
                        long msgtyp, int msgflg)
{
    //extern long sys_msgrcv(int,void*,uix_size_t,long,int)
    //    __attribute__((weak));
    if (SYS_MSGRCV)
        return (uix_ssize_t)sys_msgrcv(msqid,msgp,msgsz,msgtyp,msgflg);
    uix_errno = UIX_ENOSYS; return -1;

}

int uix_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
{
    //extern int sys_msgctl(int,int,void*) __attribute__((weak));
    if(SYS_MSGCTL) return sys_msgctl(msqid, cmd, buf);
    uix_errno = UIX_ENOSYS; return -1;

}

/* ***This is End of file, there is no more line should be added after this line*** */
