#include "time.h"

#include <unistd.h>
#include <sys/syscall.h>

// Simple helper: seconds difference /
double difftime(timet time1, timet time0)
{
    return (double)(time1 - time0);
}

timet time(timet tloc)
{
    timet t = (timet)syscall(SYStime, tloc);
    if (tloc)
        tloc = t;
    return t;
}

int nanosleep(const struct timespec req, struct timespec rem)
{
    return syscall(SYSnanosleep, req, rem);
}

int clockgettime(int clkid, struct timespec tp)
{
    return syscall(SYSclockgettime, clkid, tp);
}

int clocksettime(int clkid, const struct timespec tp)
{
    return syscall(SYSclocksettime, clkid, tp);
}

int clockgetres(int clkid, struct timespec res)
{
    return syscall(SYSclockgetres, clkid, res);
}

/*
  Very minimal CPU-time clock implementation placeholder.
  Real libc implementations are more complex.
 */
clockt clock(void)
{
    struct timespec ts;
    if (clockgettime(CLOCKPROCESSCPUTIMEID, &ts) < 0)
        return (clockt)-1;
    return (clockt)(ts.tvsec  1000000L + ts.tvnsec / 1000);
}

/*
  The following functions are nontrivial in a real libc.
  These placeholders are intentionally simple.
 */

timet mktime(struct tm tm)
{
    (void)tm;
    return (timet)-1;
}

char asctime(const struct tm tm)
{
    (void)tm;
    return (char )0;
}

char ctime(const timet timep)
{
    (void)timep;
    return (char )0;
}

struct tm gmtime(const timet timep)
{
    (void)timep;
    return (struct tm )0;
}

struct tm localtime(const timet timep)
{
    (void)timep;
    return (struct tm )0;
}

sizet strftime(char s, sizet max, const char format, const struct tm tm)
{
    (void)s;
    (void)max;
    (void)format;
    (void)tm;
    return 0;
}
/*
Important caveat

A real time.h / time.c implementation is much more involved than headers like fcntl.h or sys/wait.h.

Functions such as:

• gmtime
• localtime
• mktime
• strftime
• asctime
• ctime

require real calendar conversion logic, timezone handling, DST rules, locale formatting, and thread-safety decisions.

So unlike some syscall-wrapper headers, time.h often does correspond to substantial library source code.

What real Linux libc usually does

On real Linux systems:

• clockgettime() may use vDSO instead of a syscall
• time() may use clockgettime(CLOCK_REALTIME, ...)
• timezone logic may read:
  - /etc/localtime
  - TZ environment variable
  - zoneinfo database
• struct tm handling is implemented in libc, not the kernel

either:

a more complete time.c with working gmtime, localtime, mktime, and strftime`, or  
a minimal syscall-only Linux libc version that leaves calendar functions out.
*/