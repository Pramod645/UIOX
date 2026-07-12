/**
 * @file  uix_posix_time.c
 * @brief UIOX POSIX — time syscall implementations.
 *        clock_gettime, clock_settime, nanosleep, gettimeofday,
 *        settimeofday, getitimer, setitimer, times.
 */

 #include "uix_syscall.h"
 #include "../sys/uix_time.h"
 #include "../sys/uix_times.h"
 
 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 int uix_clock_gettime(clockid_t clk_id, struct uix_timespec *tp)
 {
     return (int)_ret(uix_syscall2(SYS_CLOCK_GETTIME,
                                    (long)clk_id,(long)tp));
 }
 
 int uix_clock_settime(clockid_t clk_id, const struct uix_timespec *tp)
 {
     return (int)_ret(uix_syscall2(SYS_CLOCK_SETTIME,
                                    (long)clk_id,(long)tp));
 }
 
 int uix_clock_getres(clockid_t clk_id, struct uix_timespec *res)
 {
     return (int)_ret(uix_syscall2(SYS_CLOCK_GETRES,
                                    (long)clk_id,(long)res));
 }
 
 int uix_nanosleep(const struct uix_timespec *req, struct uix_timespec *rem)
 {
     return (int)_ret(uix_syscall2(SYS_NANOSLEEP,(long)req,(long)rem));
 }
 
 int uix_gettimeofday(struct uix_timeval *tv, struct uix_timezone *tz)
 {
     return (int)_ret(uix_syscall2(SYS_GETTIMEOFDAY,(long)tv,(long)tz));
 }
 
 int uix_settimeofday(const struct uix_timeval *tv,
                       const struct uix_timezone *tz)
 {
     return (int)_ret(uix_syscall2(SYS_SETTIMEOFDAY,(long)tv,(long)tz));
 }
 
 clock_t uix_times(struct uix_tms *buf)
 {
     long r = uix_syscall1(SYS_TIMES,(long)buf);
     if (r < 0L) { uix_errno = (int)(-r); return (clock_t)-1L; }
     return (clock_t)r;
 }
 
 unsigned int uix_sleep(unsigned int seconds)
 {
     struct uix_timespec req = { .tv_sec = seconds, .tv_nsec = 0 };
     struct uix_timespec rem = { 0, 0 };
     if (uix_nanosleep(&req, &rem) == 0) return 0;
     return (unsigned int)rem.tv_sec;
 }
 
 int uix_usleep(unsigned int useconds)
 {
     struct uix_timespec req = {
         .tv_sec  = useconds / 1000000u,
         .tv_nsec = (useconds % 1000000u) * 1000L
     };
     return uix_nanosleep(&req, NULL);
 }
 
 #define clock_gettime  uix_clock_gettime
 #define clock_settime  uix_clock_settime
 #define nanosleep      uix_nanosleep
 #define gettimeofday   uix_gettimeofday
 #define sleep          uix_sleep
 #define usleep         uix_usleep
 