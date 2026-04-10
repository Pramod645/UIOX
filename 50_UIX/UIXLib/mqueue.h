
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

#endif /* End of __MQUEUE__H */
/* ***This is End of file, there is no more line should be added after this line*** */