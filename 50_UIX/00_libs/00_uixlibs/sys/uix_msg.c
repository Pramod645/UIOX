/* demo Source Code Using sys/msg.h */

//This short example sends and then receives a message from a System V message queue.

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
A queue with key 1234 is created.
A message of type 1 is sent.
The same process (or another one) receives it back.
The queue is deleted with msgctl()
*/
#define MSGKEY 1234   // Arbitrary key for example /

/* Define our custom message structure */
struct mymsg {
    long mtype;
    char mtext[100];
};

int msg(void) {
    int msqid;
    struct mymsg msg;

    /* Create or get a message queue */
    msqid = msgget(MSGKEY, IPCCREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(EXITFAILURE);
    }

    /* Prepare and send a message */
    msg.mtype = 1;
    strncpy(msg.mtext, "Hello via message queue!", sizeof(msg.mtext)-1);
    msg.mtext[sizeof(msg.mtext)-1] = '\0';

    if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
        perror("msgsnd");
        exit(EXITFAILURE);
    }
    printf("Sent: %s\n", msg.mtext);

    /* Receive the message */
    memset(&msg, 0, sizeof(msg));
    if (msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
        perror("msgrcv");
        exit(EXITFAILURE);
    }
    printf("Received: %s\n", msg.mtext);

    /* Remove the message queue */
    if (msgctl(msqid, IPCRMID, NULL) == -1) {
        perror("msgctl");
        exit(EXITFAILURE);
    }

    return 0;
}
/*
int main(int argc, char argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [send|recv]\n", argv[0]);
        exit(EXITFAILURE);
    }

    if (strcmp(argv[1], "send") == 0)
        sender();
    else if (strcmp(argv[1], "recv") == 0)
        receiver();
    else {
        fprintf(stderr, "Unknown mode: %s (use 'send' or 'recv')\n", argv[1]);
        exit(EXITFAILURE);
    }

    return 0;
}

void sender(void) {
    int msqid;
    struct mymsg msg;

    msqid = msgget(MSGKEY, IPCCREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(EXITFAILURE);
    }

    printf("Enter a message: ");
    fflush(stdout);

    if (fgets(msg.mtext, MAXTEXT, stdin) == NULL) {
        fprintf(stderr, "No input.\n");
        exit(EXITFAILURE);
    }

    msg.mtype = 1;

    if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
        perror("msgsnd");
        exit(EXITFAILURE);
    }

    printf("Message sent: %s", msg.mtext);
}

void receiver(void) {
    int msqid;
    struct mymsg msg;

    msqid = msgget(MSGKEY, 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(EXITFAILURE);
    }

    if (msgrcv(msqid, &msg, sizeof(msg.mtext), 0, 0) == -1) {
        perror("msgrcv");
        exit(EXITFAILURE);
    }

    printf("Received message: %s\n", msg.mtext);

    // Optional cleanup: remove queue after reading 
    if (msgctl(msqid, IPCRMID, NULL) == -1)
        perror("msgctl (remove)");
}
*/
////////////////////////
/* src/uix_msg.c */
#include "uix_msg.h"
#include "uix_errno.h"

int uix_msgget(uix_key_t key, int msgflg)
{
    extern int sys_msgget(uix_key_t,int) __attribute__((weak));
    if (sys_msgget) return sys_msgget(key, msgflg);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_msgsnd(int msqid, const void *msgp,
               uix_size_t msgsz, int msgflg)
{
    extern int sys_msgsnd(int,const void*,uix_size_t,int)
        __attribute__((weak));
    if (sys_msgsnd) return sys_msgsnd(msqid,msgp,msgsz,msgflg);
    uix_errno = UIX_ENOSYS; return -1;
}

uix_ssize_t uix_msgrcv(int msqid, void *msgp, uix_size_t msgsz,
                        long msgtyp, int msgflg)
{
    extern long sys_msgrcv(int,void*,uix_size_t,long,int)
        __attribute__((weak));
    if (sys_msgrcv)
        return (uix_ssize_t)sys_msgrcv(msqid,msgp,msgsz,msgtyp,msgflg);
    uix_errno = UIX_ENOSYS; return -1;
}

int uix_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
{
    extern int sys_msgctl(int,int,void*) __attribute__((weak));
    if (sys_msgctl) return sys_msgctl(msqid, cmd, buf);
    uix_errno = UIX_ENOSYS; return -1;
}


