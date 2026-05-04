
#ifndef __SYS_UIX_TIME__H
#define __SYS_UIX_TIME__H
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

#ifndef UIX_TIME_H
#define UIX_TIME_H

#include "uix_types.h"

typedef struct uix_timespec {
    uix_time_t tv_sec;
    long       tv_nsec;
} uix_timespec_t;

typedef struct uix_timeval {
    uix_time_t tv_sec;
    long       tv_usec;
} uix_timeval_t;

typedef struct uix_tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
} uix_tm_t;

typedef struct uix_itimerval {
    uix_timeval_t it_interval;
    uix_timeval_t it_value;
} uix_itimerval_t;

#define UIX_CLOCK_REALTIME           0
#define UIX_CLOCK_MONOTONIC          1
#define UIX_CLOCK_PROCESS_CPUTIME_ID 2
#define UIX_CLOCK_THREAD_CPUTIME_ID  3

#define UIX_ITIMER_REAL    0
#define UIX_ITIMER_VIRTUAL 1
#define UIX_ITIMER_PROF    2

uix_time_t   uix_time        (uix_time_t *tloc);
int          uix_gettimeofday(uix_timeval_t *tv, void *tz);
int          uix_settimeofday(const uix_timeval_t *tv, const void *tz);
int          uix_clock_gettime(int clkid, uix_timespec_t *tp);
int          uix_clock_settime(int clkid, const uix_timespec_t *tp);
int          uix_clock_getres (int clkid, uix_timespec_t *res);
int          uix_nanosleep   (const uix_timespec_t *req, uix_timespec_t *rem);
int          uix_getitimer   (int which, uix_itimerval_t *curr);
int          uix_setitimer   (int which, const uix_itimerval_t *new_val,
                               uix_itimerval_t *old_val);
uix_tm_t    *uix_gmtime      (const uix_time_t *timep);
uix_tm_t    *uix_localtime   (const uix_time_t *timep);
uix_time_t   uix_mktime      (uix_tm_t *tm);
double       uix_difftime    (uix_time_t t1, uix_time_t t0);
uix_size_t   uix_strftime    (char *s, uix_size_t max,
                               const char *fmt, const uix_tm_t *tm);
uix_clock_t  uix_clock       (void);

#endif /* UIX_TIME_H */



#endif /* End of __SYS_UIX_TIME__H */
/* ***This is End of file, there is no more line should be added after this line*** */