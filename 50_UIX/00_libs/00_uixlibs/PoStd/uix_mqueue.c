/************************ uix_mqueue.c ********************************/
#include "uix_mqueue.h"
#include "uix_errno.h"
#include "uix_fcntl.h"
#include "uix_stdarg.h"

#include "../uix_sys.h"

uix_mqd_t uix_mq_open(const char *name, int oflag, ...)
{
    uix_va_list ap; uix_va_start(ap, oflag);
    uix_mode_t mode = (oflag & UIX_O_CREAT) ?
                       uix_va_arg(ap, uix_mode_t) : 0;
    void *attr = (oflag & UIX_O_CREAT) ? uix_va_arg(ap, void*) : NULL;
    uix_va_end(ap);
    //extern long sys_mq_open(const char*,int,uix_mode_t,void*)
    //    __attribute__((weak));
    //if (sys_mq_open) return (uix_mqd_t)sys_mq_open(name,oflag,mode,attr);
    //uix_errno = UIX_ENOSYS; return UIX_MQD_INVALID;
    return (uix_mqd_t)sys_mq_open(name,oflag,mode,attr);
}

int uix_mq_close(uix_mqd_t mqdes)
{
   // extern int sys_mq_close(uix_mqd_t) __attribute__((weak));
   // if (sys_mq_close) return sys_mq_close(mqdes);
   // uix_errno = UIX_ENOSYS; return -1;
   return sys_mq_close(mqdes);
}

int uix_mq_unlink(const char *name)
{
    //extern int sys_mq_unlink(const char*) __attribute__((weak));
    //if (sys_mq_unlink) return sys_mq_unlink(name);
    //uix_errno = UIX_ENOSYS; return -1;
    return sys_mq_unlink(name);
}

int uix_mq_send(uix_mqd_t mqdes, const char *msg_ptr,
                uix_size_t msg_len, unsigned int msg_prio)
{
    //extern int sys_mq_send(uix_mqd_t,const char*,uix_size_t,unsigned)
    //    __attribute__((weak));
    //if (sys_mq_send) return sys_mq_send(mqdes,msg_ptr,msg_len,msg_prio);
    //uix_errno = UIX_ENOSYS; return -1;
    return sys_mq_send(mqdes,msg_ptr,msg_len,msg_prio);
}

uix_ssize_t uix_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
                            uix_size_t msg_len, unsigned int *msg_prio)
{
    //extern long sys_mq_receive(uix_mqd_t,char*,uix_size_t,unsigned*)
    //    __attribute__((weak));
    //if (sys_mq_receive)
    //    return (uix_ssize_t)sys_mq_receive(mqdes,msg_ptr,msg_len,msg_prio);
    //uix_errno = UIX_ENOSYS; return -1;
    return (uix_ssize_t)sys_mq_receive(mqdes,msg_ptr,msg_len,msg_prio);
}

uix_ssize_t uix_mq_timedreceive(uix_mqd_t mqdes, char *msg_ptr,
                                 uix_size_t msg_len, unsigned int *msg_prio,
                                 const uix_timespec_t *abs_timeout)
{
    (void)abs_timeout;
    return uix_mq_receive(mqdes, msg_ptr, msg_len, msg_prio);
}

int uix_mq_timedsend(uix_mqd_t mqdes, const char *msg_ptr,
                     uix_size_t msg_len, unsigned int msg_prio,
                     const uix_timespec_t *abs_timeout)
{
    (void)abs_timeout;
    return uix_mq_send(mqdes, msg_ptr, msg_len, msg_prio);
}

int uix_mq_getattr(uix_mqd_t mqdes, uix_mq_attr_t *attr)
{
    (void)mqdes;
    if (!attr) { uix_errno = UIX_EINVAL; return -1; }
    attr->mq_flags   = 0;
    attr->mq_maxmsg  = 10;
    attr->mq_msgsize = 256;
    attr->mq_curmsgs = 0;
    return 0;
}

int uix_mq_setattr(uix_mqd_t mqdes, const uix_mq_attr_t *newattr,
                   uix_mq_attr_t *oldattr)
{
    if (oldattr) return uix_mq_getattr(mqdes, oldattr);
    (void)newattr; return 0;
}

/* ***This is End of file, there is no more line should be added after this line*** */
