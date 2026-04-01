//can you provide the header file and source code for this times.h file?
Here’s a Linux-style sys/times.h and a matching minimal source file.

Unlike time.h, this header is specifically for the times() function, which reports process CPU usage.

sys/times.h

``c
#ifndef SYSTIMESH
#define SYSTIMESH 1

#ifdef _cplusplus
extern "C" {
#endif

#include <sys/types.h>

/
  Structure filled by times().
  All values are measured in clock ticks.
 /
struct tms {
    clockt tmsutime;   / user CPU time /
    clockt tmsstime;   / system CPU time /
    clockt tmscutime;  / user CPU time of children /
    clockt tmscstime;  / system CPU time of children /
};

/
  Return value:
    elapsed real time in clock ticks since an arbitrary point,
    typically system boot time.
 /
clockt times(struct tms buf);

#ifdef _cplusplus
}
#endif

#endif / SYSTIMESH /
`

times.c

`c
#include "sys/times.h"

#include <unistd.h>
#include <sys/syscall.h>

clockt times(struct tms *buf)
{
    return (clockt)syscall(SYStimes, buf);
}
`

Example usage

`c
#include <sys/times.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    struct tms t;
    clockt now = times(&t);

    if (now == (clockt)-1) {
        perror("times");
        return 1;
    }

    printf("Elapsed ticks: %ld\n", (long)now);
    printf("User CPU:      %ld\n", (long)t.tmsutime);
    printf("System CPU:    %ld\n", (long)t.tmsstime);
    printf("Child user:    %ld\n", (long)t.tmscutime);
    printf("Child system:  %ld\n", (long)t.tmscstime);

    return 0;
}
`

Notes
• times() reports values in clock ticks, not seconds.
• To convert ticks to seconds, programs usually divide by the result of:
  - sysconf(SCCLKTCK)
• clockt must be defined consistently with your sys/types.h or time.h.

For example:

`c
long tickspersec = sysconf(SCCLKTCK);
double userseconds = (double)t.tmsutime / tickspersec;
`

Important caveat

This is a minimal libc-style wrapper. Real libc may include:

• symbol versioning
• cancellation points
• architecture-specific syscall glue
• large ABI compatibility handling

But for a small Linux-style libc project, the header and source above are the normal shape.

If you want, I can keep going and provide the next one, like:

• signal.h
• errno.h
• unistd.h
• sys/utsname.h`