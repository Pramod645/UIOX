
#ifndef __SYS_TIME__H
#define __SYS_TIME__H
/*
Here a Linux-style time.h, the header is real, but the “source file” is usually just libc wrappers around kernel syscalls or vDSO calls.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <stddef.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Basic time-related types */
#ifndef CLOCKTDEFINED
#define CLOCKTDEFINED
typedef long clockt;
#endif

#ifndef TIMETDEFINED
#define TIMETDEFINED
typedef long timet;
#endif

/* Broken-down time */
struct tm {
    int tmsec;    // seconds after the minute [0, 60] /
    int tmmin;    // minutes after the hour [0, 59] /
    int tmhour;   // hours since midnight [0, 23] /
    int tmmday;   // day of the month [1, 31] /
    int tmmon;    // months since January [0, 11] /
    int tmyear;   // years since 1900 /
    int tmwday;   // days since Sunday [0, 6] /
    int tmyday;   // days since January 1 [0, 365] /
    int tmisdst;  // daylight saving time flag /
};

/* High-resolution time */
struct timespec {
    timet tvsec; // seconds /
    long   tvnsec;// nanoseconds /
};

/* Clock IDs */
#define CLOCKREALTIME           0
#define CLOCKMONOTONIC          1
#define CLOCKPROCESSCPUTIMEID 2
#define CLOCKTHREADCPUTIMEID  3

/* timer values */
struct itimerspec {
    struct timespec itinterval;
    struct timespec itvalue;
};

/* Standard functions */
clockt clock(void);
timet time(timet tloc);
double difftime(timet time1, timet time0);
timet mktime(struct tm tm);

char asctime(const struct tm tm);
char ctime(const timet timep);

struct tm gmtime(const timet timep);
struct tm localtime(const timet timep);

sizet strftime(char s, sizet max, const char format, const struct tm tm);

/* POSIX clock functions */
int nanosleep(const struct timespec req, struct timespec rem);
int clockgettime(int clkid, struct timespec tp);
int clocksettime(int clkid, const struct timespec tp);
int clockgetres(int clkid, struct timespec res);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_TIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */