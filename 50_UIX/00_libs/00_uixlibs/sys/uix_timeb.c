// timeb.c — Demonstrate <sys/timeb.h> and ftime() /

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>

int timeb(void) {
    struct timeb tb;
    ftime(&tb);

    printf("Seconds since epoch: %lld\n", (long long)tb.time);
    printf("Milliseconds: %u\n", tb.millitm);
    printf("Time zone offset (minutes west of UTC): %d\n", tb.timezone);
    printf("Daylight saving flag: %d\n", tb.dstflag);

    // Combine both to show full timestamp /
    struct tm local = localtime(&tb.time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", local);

    printf("Local time: %s.%03u\n", buf, tb.millitm);
    return 0;
}
/*
Seconds since epoch: 1714065613
Milliseconds: 523
Time zone offset (minutes west of UTC): 480
Daylight saving flag: 1
Local time: 2024-04-25 15:20:13.523

Notes
• ftime() is deprecated on most modern systems.  
  Prefer:

  - clockgettime(CLOCKREALTIME, …) for POSIX high-resolution timing.
  - gettimeofday() for subsecond wall-clock time (older but more standard).

Still, if you’re maintaining legacy C code or exploring historical UNIX APIs, <sys/timeb.h> is safe to use where available.
*/
/////////////////////////
/* src/uix_timeb.c */
#include "uix_timeb.h"
#include "uix_time.h"

int uix_ftime(uix_timeb_t *tp)
{
    if (!tp) return -1;
    uix_timeval_t tv;
    uix_gettimeofday(&tv, NULL);
    tp->time     = tv.tv_sec;
    tp->millitm  = (unsigned short)(tv.tv_usec / 1000);
    tp->timezone = 0;
    tp->dstflag  = 0;
    return 0;
}


