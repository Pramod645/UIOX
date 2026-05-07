#include "sys/times.h"

#include <unistd.h>
#include <sys/syscall.h>

clockt times(struct tms *buf)
{
    return (clockt)syscall(SYStimes, buf);
}

#include <sys/times.h>
#include <unistd.h>
#include <stdio.h>

int libtimes(void)
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

/*
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
*/
//////////////
/* src/uix_times.c */
#include "uix_times.h"
#include "uix_string.h"
#include "uix_errno.h"

uix_clock_t uix_times(uix_tms_t *buf)
{
    extern long sys_times(void *) __attribute__((weak));
    if (sys_times) return (uix_clock_t)sys_times(buf);
    if (buf) uix_memset(buf, 0, sizeof(uix_tms_t));
    return 0;
}
