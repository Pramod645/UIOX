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