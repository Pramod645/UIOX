
#ifndef __UIX_MQUEUE__H
#define __UIX_MQUEUE__H
/*
mqueue.h header defines the POSIX message queue API.  
This API provides a modern alternative to System V message queues (sys/msg.h), offering a simpler and faster way for 
processes to exchange messages through the kernel.

Below is a clear, representative version of the header and a full working example that sends and receives messages.
*/
/* This is for only POXIS */

//#include "uix_features.h"//??

#include "sys/uix_types.h"
#include "sys/uix_time.h"

typedef int uix_mqd_t;  // Message queue descriptor
#define UIX_MQD_INVALID ((uix_mqd_t)-1)      // Invalid descriptor sentinel

typedef struct uix_mq_attr {
    long mq_flags;    // Queue flags (O_NONBLOCK)
    long mq_maxmsg;       // Maximum messages in queue
    long mq_msgsize;      // Maximum message size in bytes
    long mq_curmsgs;      // Current message count (read-only)
} uix_mq_attr_t;

#define UIX_O_RDONLY  0
#define UIX_O_WRONLY  1
#define UIX_O_RDWR    2
#define UIX_O_CREAT   0x40
#define UIX_O_EXCL    0x80
#define UIX_O_NONBLOCK 0x800

uix_mqd_t uix_mq_open   (const char *name, int oflag, ...);   // Opens/creates POSIX message queue
int        uix_mq_close  (uix_mqd_t mqdes);                    // Closes message queue descriptor
int        uix_mq_unlink (const char *name);                          // Removes named message queue
int        uix_mq_send   (uix_mqd_t mqdes, const char *msg_ptr,
                           uix_size_t msg_len, unsigned int msg_prio);          // Sends message with priority
uix_ssize_t uix_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
                            uix_size_t msg_len, unsigned int *msg_prio);    // Receives highest priority message
int        uix_mq_timedsend(uix_mqd_t mqdes, const char *msg_ptr,
                             uix_size_t msg_len, unsigned int msg_prio,
                             const uix_timespec_t *abs_timeout);           // Send with absolute timeout
uix_ssize_t uix_mq_timedreceive(uix_mqd_t mqdes, char *msg_ptr,
                                 uix_size_t msg_len, unsigned int *msg_prio,
                                 const uix_timespec_t *abs_timeout);         // Receive with absolute timeout
int        uix_mq_getattr(uix_mqd_t mqdes, uix_mq_attr_t *attr);             // Gets queue attributes
int        uix_mq_setattr(uix_mqd_t mqdes, const uix_mq_attr_t *newattr,
                           uix_mq_attr_t *oldattr);                           // Sets queue attributes, returns old


#endif /* End of __UIX_MQUEUE__H */
/* ***This is End of file, there is no more line should be added after this line*** */
