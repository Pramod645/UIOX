
#ifndef __MQUEUE__H
#define __MQUEUE__H
/*
mqueue.h header defines the POSIX message queue API.  
This API provides a modern alternative to System V message queues (sys/msg.h), offering a simpler and faster way for 
processes to exchange messages through the kernel.

Below is a clear, representative version of the header and a full working example that sends and receives messages.
*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <time.h>
#include <fcntl.h>    // for O constants /


#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Message queue descriptor type /
typedef int mqdt;

// Message queue attributes structure /
struct mqattr {
    long mqflags;   // Message queue flags (e.g., ONONBLOCK)  /
    long mqmaxmsg;  // Maximum number of messages allowed in queue /
    long mqmsgsize; // Maximum message size in bytes /
    long mqcurmsgs; // Number of messages currently queued /
};

// Function prototypes /
mqdt mqopen(const char name, int oflag, ...);
int mqclose(mqdt mqdes);
int mqunlink(const char name);
int mqsend(mqdt mqdes, const char msgptr, sizet msglen, unsigned int msgprio);
ssizet mqreceive(mqdt mqdes, char msgptr, sizet msglen, unsigned int msgprio);
int mqgetattr(mqdt mqdes, struct mqattr attr);
int mqsetattr(mqdt mqdes, const struct mqattr newattr, struct mqattr oldattr);
int mqnotify(mqdt mqdes, const struct sigevent notification);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



/* include/uix_mqueue.h */
#ifndef UIX_MQUEUE_H
#define UIX_MQUEUE_H

#include "uix_types.h"
#include "uix_time.h"

typedef int uix_mqd_t;
#define UIX_MQD_INVALID ((uix_mqd_t)-1)

typedef struct uix_mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
} uix_mq_attr_t;

#define UIX_O_RDONLY  0
#define UIX_O_WRONLY  1
#define UIX_O_RDWR    2
#define UIX_O_CREAT   0x40
#define UIX_O_EXCL    0x80
#define UIX_O_NONBLOCK 0x800

uix_mqd_t uix_mq_open   (const char *name, int oflag, ...);
int        uix_mq_close  (uix_mqd_t mqdes);
int        uix_mq_unlink (const char *name);
int        uix_mq_send   (uix_mqd_t mqdes, const char *msg_ptr,
                           uix_size_t msg_len, unsigned int msg_prio);
uix_ssize_t uix_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
                            uix_size_t msg_len, unsigned int *msg_prio);
int        uix_mq_timedsend(uix_mqd_t mqdes, const char *msg_ptr,
                             uix_size_t msg_len, unsigned int msg_prio,
                             const uix_timespec_t *abs_timeout);
uix_ssize_t uix_mq_timedreceive(uix_mqd_t mqdes, char *msg_ptr,
                                 uix_size_t msg_len, unsigned int *msg_prio,
                                 const uix_timespec_t *abs_timeout);
int        uix_mq_getattr(uix_mqd_t mqdes, uix_mq_attr_t *attr);
int        uix_mq_setattr(uix_mqd_t mqdes, const uix_mq_attr_t *newattr,
                           uix_mq_attr_t *oldattr);

#endif /* UIX_MQUEUE_H */



#endif /* End of __MQUEUE__H */
/* ***This is End of file, there is no more line should be added after this line*** */