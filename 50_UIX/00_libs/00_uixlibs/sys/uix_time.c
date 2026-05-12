#include "uix_time.h"
#include "../uix_errno.h"
#include "../uix_string.h"

#include "../uix_stdio.h"

static uix_time_t base_time = 1000000000L; /* simulated epoch  */

uix_time_t uix_time(uix_time_t *tloc)
{
    extern long sys_time(uix_time_t *) __attribute__((weak));
    uix_time_t t = sys_time ? (uix_time_t)sys_time(NULL) : base_time++;
    if (tloc) *tloc = t;
    return t;
}

int uix_gettimeofday(uix_timeval_t *tv, void *tz)
{
    (void)tz;
    if (!tv) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_gettimeofday(uix_timeval_t *, void *)
        __attribute__((weak));
    if (sys_gettimeofday) return sys_gettimeofday(tv, NULL);
    tv->tv_sec  = uix_time(NULL);
    tv->tv_usec = 0;
    return 0;
}

int uix_settimeofday(const uix_timeval_t *tv, const void *tz)
{
    (void)tz;
    if (!tv) { uix_errno = UIX_EFAULT; return -1; }
    base_time = tv->tv_sec;
    return 0;
}

int uix_clock_gettime(int clkid, uix_timespec_t *tp)
{
    if (!tp) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_clock_gettime(int, uix_timespec_t *)
        __attribute__((weak));
    if (sys_clock_gettime) return sys_clock_gettime(clkid, tp);
    tp->tv_sec  = uix_time(NULL);
    tp->tv_nsec = 0;
    return 0;
}

int uix_clock_settime(int clkid, const uix_timespec_t *tp)
{
    (void)clkid;
    if (!tp) { uix_errno = UIX_EFAULT; return -1; }
    base_time = tp->tv_sec;
    return 0;
}

int uix_clock_getres(int clkid, uix_timespec_t *res)
{
    (void)clkid;
    if (!res) { uix_errno = UIX_EFAULT; return -1; }
    res->tv_sec  = 0;
    res->tv_nsec = 1000000L;    /* 1 ms resolution */
    return 0;
}

int uix_nanosleep(const uix_timespec_t *req, uix_timespec_t *rem)
{
    if (!req) { uix_errno = UIX_EFAULT; return -1; }
    volatile unsigned long cnt =
        (unsigned long)(req->tv_sec * 1000000UL +
                        req->tv_nsec / 1000UL) * 10UL;
    while (cnt--) {}
    if (rem) { rem->tv_sec = 0; rem->tv_nsec = 0; }
    return 0;
}

int uix_getitimer(int which, uix_itimerval_t *curr)
{
    (void)which;
    if (!curr) { uix_errno = UIX_EFAULT; return -1; }
    uix_memset(curr, 0, sizeof(*curr));
    return 0;
}

int uix_setitimer(int which, const uix_itimerval_t *new_v,
                  uix_itimerval_t *old_v)
{
    (void)which; (void)new_v;
    if (old_v) uix_memset(old_v, 0, sizeof(*old_v));
    return 0;
}

static uix_tm_t _tm;

uix_tm_t *uix_gmtime(const uix_time_t *timep)
{
    uix_time_t t = timep ? *timep : uix_time(NULL);
    _tm.tm_sec   = (int)(t % 60);  t /= 60;
    _tm.tm_min   = (int)(t % 60);  t /= 60;
    _tm.tm_hour  = (int)(t % 24);  t /= 24;
    _tm.tm_wday  = (int)((t + 4) % 7);
    /* Simplified: month/day from day count */
    _tm.tm_year  = 70;
    int days = (int)t;
    while (days >= 365) { days -= 365; _tm.tm_year++; }
    _tm.tm_yday  = days;
    int months[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    _tm.tm_mon   = 0;
    while (days >= months[_tm.tm_mon]) {
        days -= months[_tm.tm_mon]; _tm.tm_mon++;
    }
    _tm.tm_mday  = days + 1;
    _tm.tm_isdst = 0;
    return &_tm;
}

uix_tm_t *uix_localtime(const uix_time_t *timep)
{
    return uix_gmtime(timep);  /* no timezone in UIOX */
}

uix_time_t uix_mktime(uix_tm_t *tm)
{
    uix_time_t t = (uix_time_t)(tm->tm_year - 70) * 365 * 86400;
    t += (uix_time_t)tm->tm_yday * 86400;
    t += (uix_time_t)tm->tm_hour * 3600;
    t += (uix_time_t)tm->tm_min  * 60;
    t += (uix_time_t)tm->tm_sec;
    return t;
}

double uix_difftime(uix_time_t t1, uix_time_t t0)
{
    return (double)(t1 - t0);
}

uix_clock_t uix_clock(void)
{
    return (uix_clock_t)uix_time(NULL) * 1000;
}

uix_size_t uix_strftime(char *s, uix_size_t max,
                        const char *format, const uix_tm_t *tm)
{
    /* Minimal implementation */
    uix_size_t pos = 0;
    while (*format && pos < max - 1) {
        if (*format == '%') {
            format++;
            char buf[8];
            switch (*format) {
            case 'Y': uix_snprintf(buf,8,"%04d",tm->tm_year+1900);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case 'm': uix_snprintf(buf,8,"%02d",tm->tm_mon+1);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case 'd': uix_snprintf(buf,8,"%02d",tm->tm_mday);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case 'H': uix_snprintf(buf,8,"%02d",tm->tm_hour);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case 'M': uix_snprintf(buf,8,"%02d",tm->tm_min);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case 'S': uix_snprintf(buf,8,"%02d",tm->tm_sec);
                      for(char*p=buf;*p&&pos<max-1;p++) s[pos++]=*p; break;
            case '%': s[pos++] = '%'; break;
            default:  s[pos++] = '%'; s[pos++] = *format; break;
            }
        } else {
            s[pos++] = *format;
        }
        format++;
    }
    s[pos] = '\0';
    return pos;
}

/* ***This is End of file, there is no more line should be added after this line*** */
