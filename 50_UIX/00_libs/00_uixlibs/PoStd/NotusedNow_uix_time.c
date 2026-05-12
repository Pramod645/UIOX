#include "sys/uix_time.h"
#include "uix_errno.h"
#include "uix_string.h"
#include "uix_stdio.h"

/*
Current local time: Thu Apr 25 15:21:34 2024
Formatted time: 2024-04-25 15:21:34 PDT
CPU clock ticks since program start: 1234
`

In summary
• <time.h> defines C and POSIX mechanisms for working with time and date values.
• timet stores absolute time (seconds since epoch).
• struct tm represents broken-down calendar time.
• ctime(), strftime(), and localtime() help convert between formats.
*/

static uix_time_t base_time = 1000000000L; /* simulated epoch  */

uix_time_t uix_time(uix_time_t *tloc)
{
    extern long sys_time(uix_time_t *) __attribute__((weak));
    uix_time_t t = sys_time ? (uix_time_t)sys_time(NULL) : base_time++;
    if (tloc) *tloc = t;
    return t;
} //


int uix_clock_gettime(int clkid, uix_timespec_t *tp)
{
    if (!tp) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_clock_gettime(int, uix_timespec_t *)
        __attribute__((weak));
    if (sys_clock_gettime) return sys_clock_gettime(clkid, tp);
    tp->tv_sec  = uix_time(NULL);
    tp->tv_nsec = 0;
    return 0;
}//


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
}//

uix_tm_t *uix_localtime(const uix_time_t *timep)
{
    return uix_gmtime(timep);  /* no timezone in UIOX */
}//

uix_time_t uix_mktime(uix_tm_t *tm)
{
    uix_time_t t = (uix_time_t)(tm->tm_year - 70) * 365 * 86400;
    t += (uix_time_t)tm->tm_yday * 86400;
    t += (uix_time_t)tm->tm_hour * 3600;
    t += (uix_time_t)tm->tm_min  * 60;
    t += (uix_time_t)tm->tm_sec;
    return t;
}//

double uix_difftime(uix_time_t t1, uix_time_t t0)
{
    return (double)(t1 - t0);
}//

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
}//

/* ***This is End of file, there is no more line should be added after this line*** */
