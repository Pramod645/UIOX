#include "sys/select.h"

#include <unistd.h>
#include <sys/syscall.h>

int select(int nfds,
           fdset readfds,
           fdset writefds,
           fdset exceptfds,
           struct timeval timeout)
{
    return syscall(SYSselect, nfds, readfds, writefds, exceptfds, timeout);
}

int pselect(int nfds,
            fdset readfds,
            fdset writefds,
            fdset exceptfds,
            const struct timespec timeout,
            const sigsett sigmask)
{
    return syscall(SYSpselect6,
                   nfds,
                   readfds,
                   writefds,
                   exceptfds,
                   timeout,
                   sigmask);
}
/*
Important caveats

This is a minimal educational version. Real Linux/libc code is more complicated:

• on modern Linux, select() may be implemented using pselect6 or other internal helpers
• pselect() on Linux usually needs a special kernel argument structure, not just a raw sigsett 
• sigsett here is only a placeholder and should really come from signal.h
• struct timeval is usually declared in sys/time.h, not directly here
• syscall availability varies by architecture

So the pselect() wrapper above is not fully ABI-correct for a production libc.

A more Linux-like pselect6 kernel argument looks like this:

truct pselect6arg {
    const sigsett ss;
    sizet sslen;
};


and then:


int pselect(int nfds,
            fdset readfds,
            fdset writefds,
            fdset exceptfds,
            const struct timespec timeout,
            const sigsett sigmask)
{
    struct pselect6arg arg;
    arg.ss = sigmask;
    arg.sslen = sizeof(sigsett);

    return syscall(SYSpselect6,
                   nfds,
                   readfds,
                   writefds,
                   exceptfds,
                   timeout,
                   &arg);
}
*/
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>

int select(void)
{
    fdset rfds;
    struct timeval tv;
    int ret;

    FDZERO(&rfds);
    FDSET(0, &rfds); / stdin /

    tv.tvsec = 5;
    tv.tvusec = 0;

    ret = select(1, &rfds, 0, 0, &tv);
    if (ret == -1) {
        perror("select");
        return 1;
    } else if (ret == 0) {
        printf("timeout\n");
    } else {
        printf("stdin is ready\n");
    }

    return 0;
}