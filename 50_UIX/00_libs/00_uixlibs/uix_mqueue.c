#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>      // for O constants /
#include <sys/stat.h>   // for mode constants /

#define QUEUENAME  "/demoqueue"
#define MAXSIZE    256

int libmqueue(void) {
    mqdt mq;
    struct mqattr attr;
    char message[MAXSIZE];

    attr.mqflags = 0;
    attr.mqmaxmsg = 10;
    attr.mqmsgsize = MAXSIZE;
    attr.mqcurmsgs = 0;

    // Create or open message queue /
    mq = mqopen(QUEUENAME, OCREAT | OWRONLY, 0644, &attr);
    if (mq == (mqdt)-1) {
        perror("mqopen");
        exit(EXITFAILURE);
    }

    printf("Enter message: ");
    if (fgets(message, MAXSIZE, stdin) == NULL) {
        perror("fgets");
        exit(EXITFAILURE);
    }

    // Remove trailing newline /
    message[strcspn(message, "\n")] = '\0';

    if (mqsend(mq, message, strlen(message) + 1, 0) == -1) {
        perror("mqsend");
        exit(EXITFAILURE);
    }

    printf("Sent: %s\n", message);

    mqclose(mq);
    return 0;
}

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

#define QUEUENAME  "/demoqueue"
#define MAXSIZE    256

int main(void) {
    mqdt mq;
    char buf[MAXSIZE];
    struct mqattr attr;

    / Open the existing queue for reading /
    mq = mqopen(QUEUENAME, ORDONLY);
    if (mq == (mqdt)-1) {
        perror("mqopen");
        exit(EXITFAILURE);
    }

    / Get queue attributes for info (optional) /
    mqgetattr(mq, &attr);
    printf("Waiting for messages (max size: %ld bytes)...\n", attr.mqmsgsize);

    / Receive a message /
    ssizet bytesread = mqreceive(mq, buf, MAXSIZE, NULL);
    if (bytesread >= 0) {
        printf("Received: %s\n", buf);
    } else {
        perror("mqreceive");
    }

    mqclose(mq);

    / Optional cleanup /
    mqunlink(QUEUENAME);
    return 0;
}
`

How to Build and Run

You need to link with -lrt (the POSIX realtime library):

`bash
gcc mqsendexample.c -o mqsend -lrt
gcc mqreceiveexample.c -o mqrecv -lrt
`

Run each in separate terminals:

Terminal 1 (receiver):
`bash
./mqrecv
`

Terminal 2 (sender):
`bash
./mq_send
`

You’ll see:
`
Enter message: Hello from sender!
Sent: Hello from sender!
`

Receiver will print:
`
Waiting for messages (max size: 256 bytes)...
Received: Hello from sender!
`
*/

////////////////////////////////
/* src/uix_mqueue.c */
#include "uix_mqueue.h"
#include "uix_errno.h"
#include "uix_fcntl.h"
#include "uix_stdarg.h"

uix_mqd_t uix_mq_open(const char *name, int oflag, ...)
{
    uix_va_list ap; uix_va_start(ap, oflag);
    uix_mode_t mode = (oflag & UIX_O_CREAT) ?
                       uix_va_arg(ap, uix_mode_t) : 0;
    void *attr = (oflag & UIX_O_CREAT) ? uix_va_arg(ap, void*) : NULL;
    uix_va_end(ap);
    extern long sys_mq_open(const char*,int,uix_mode_t,void*)
        __attribute__((weak));
    if (sys_mq_open) return (uix_mqd_t)sys_mq_open(name,oflag,mode,attr);
    uix_errno = UIX_ENOSYS; return UIX_MQD_INVALID;
}

int uix_mq_close(uix_mqd_t mqdes)
{
    extern int sys_mq_close(uix_mqd_t) __attribute__((weak));
    if (sys_mq_close) return sys_mq_close(mqdes);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_mq_unlink(const char *name)
{
    extern int sys_mq_unlink(const char*) __attribute__((weak));
    if (sys_mq_unlink) return sys_mq_unlink(name);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_mq_send(uix_mqd_t mqdes, const char *msg_ptr,
                uix_size_t msg_len, unsigned int msg_prio)
{
    extern int sys_mq_send(uix_mqd_t,const char*,uix_size_t,unsigned)
        __attribute__((weak));
    if (sys_mq_send) return sys_mq_send(mqdes,msg_ptr,msg_len,msg_prio);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
                            uix_size_t msg_len, unsigned int *msg_prio)
{
    extern long sys_mq_receive(uix_mqd_t,char*,uix_size_t,unsigned*)
        __attribute__((weak));
    if (sys_mq_receive)
        return (uix_ssize_t)sys_mq_receive(mqdes,msg_ptr,msg_len,msg_prio);
    uix_errno = UIX_ENOSYS; return -1;
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


