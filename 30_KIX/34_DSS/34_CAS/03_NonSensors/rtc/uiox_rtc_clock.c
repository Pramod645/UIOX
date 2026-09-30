/**
 * @file  uiox_rtc_clock.c
 * @brief UIOX RTC clock layer — broken-down time, alarm, epoch conversion.
 * @date  2026-06-10
 */

 #include "uiox_rtc_clock.h"
 #include <string.h>
 #include <errno.h>
 
 /* -------------------------------------------------------------------------
  * Epoch helpers
  * ---------------------------------------------------------------------- */
 
 #define EPOCH_YEAR  1970
 #define SECS_DAY    86400LL
 #define SECS_HOUR   3600LL
 #define SECS_MIN    60LL
 
 static const uint8_t s_dim[12] = {
     31,28,31,30,31,30,31,31,30,31,30,31
 };
 static int leap(int y)
 { return (y%4==0 && y%100!=0) || (y%400==0); }
 
 int64_t uiox_rtc_tm_to_epoch(const uiox_rtc_tm_t *tm)
 {
     int y = tm->tm_year + 1900;
     int64_t days = 0;
     for (int i = EPOCH_YEAR; i < y; i++)
         days += leap(i) ? 366 : 365;
     for (int i = 0; i < tm->tm_mon; i++) {
         days += s_dim[i];
         if (i == 1 && leap(y)) days++;
     }
     days += tm->tm_mday - 1;
     return days * SECS_DAY
          + (int64_t)tm->tm_hour * SECS_HOUR
          + (int64_t)tm->tm_min  * SECS_MIN
          + (int64_t)tm->tm_sec;
 }
 
 void uiox_rtc_epoch_to_tm(int64_t epoch, uiox_rtc_tm_t *tm)
 {
     int64_t days = epoch / SECS_DAY;
     int64_t rem  = epoch % SECS_DAY;
     tm->tm_sec   = (int)(rem % 60); rem /= 60;
     tm->tm_min   = (int)(rem % 60);
     tm->tm_hour  = (int)(rem / 60);
     tm->tm_wday  = (int)((days + 4) % 7);
     int y = EPOCH_YEAR;
     while (1) {
         int yd = leap(y) ? 366 : 365;
         if (days < yd) break;
         days -= yd; y++;
     }
     tm->tm_year = y - 1900;
     int mo = 0;
     while (1) {
         int md = s_dim[mo];
         if (mo == 1 && leap(y)) md++;
         if (days < md) break;
         days -= md; mo++;
     }
     tm->tm_mon  = mo;
     tm->tm_mday = (int)days + 1;
     tm->tm_isdst = 0;
 }
 
 /* -------------------------------------------------------------------------
  * Validation
  * ---------------------------------------------------------------------- */
 
 int uiox_rtc_clock_valid(const uiox_rtc_tm_t *tm)
 {
     if (!tm) return -EINVAL;
     if (tm->tm_sec  < 0  || tm->tm_sec  > 59) return -ERANGE;
     if (tm->tm_min  < 0  || tm->tm_min  > 59) return -ERANGE;
     if (tm->tm_hour < 0  || tm->tm_hour > 23) return -ERANGE;
     if (tm->tm_mon  < 0  || tm->tm_mon  > 11) return -ERANGE;
     if (tm->tm_mday < 1)                       return -ERANGE;
     int y   = tm->tm_year + 1900;
     int max = s_dim[tm->tm_mon];
     if (tm->tm_mon == 1 && leap(y)) max = 29;
     if (tm->tm_mday > max)          return -ERANGE;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * Clock API
  * ---------------------------------------------------------------------- */
 
 int uiox_rtc_clock_init(uiox_rtc_clock_t *clk, uiox_rtc_if_t *rif)
 {
     if (!clk || !rif) return -EINVAL;
     clk->rif = rif;
     return 0;
 }
 
 int uiox_rtc_clock_read(uiox_rtc_clock_t *clk, uiox_rtc_tm_t *tm)
 {
     if (!clk || !tm) return -EINVAL;
     uint8_t s,m,h,md,mo; uint16_t yr;
     int rc = uiox_rtc_if_time_read(clk->rif, &s,&m,&h,&md,&mo,&yr);
     if (rc < 0) return rc;
     tm->tm_sec  = s;
     tm->tm_min  = m;
     tm->tm_hour = h;
     tm->tm_mday = md;
     tm->tm_mon  = mo - 1;           /* hw: 1–12 → struct: 0–11 */
     tm->tm_year = (int)yr - 1900;
     tm->tm_wday = tm->tm_isdst = 0;
     return uiox_rtc_clock_valid(tm);
 }
 
 int uiox_rtc_clock_write(uiox_rtc_clock_t *clk, const uiox_rtc_tm_t *tm)
 {
     if (!clk || !tm) return -EINVAL;
     int rc = uiox_rtc_clock_valid(tm);
     if (rc < 0) return rc;
     return uiox_rtc_if_time_write(clk->rif,
                                    (uint8_t)tm->tm_sec,
                                    (uint8_t)tm->tm_min,
                                    (uint8_t)tm->tm_hour,
                                    (uint8_t)tm->tm_mday,
                                    (uint8_t)(tm->tm_mon + 1),
                                    (uint16_t)(tm->tm_year + 1900));
 }
 
 int uiox_rtc_clock_alarm_read(uiox_rtc_clock_t *clk, uiox_rtc_alarm_t *alm)
 {
     if (!clk || !alm) return -EINVAL;
     uint8_t s,m,h;
     int rc = uiox_rtc_if_alarm_read(clk->rif, &s,&m,&h);
     if (rc < 0) return rc;
     alm->time.tm_sec  = (s == 0xFFu) ? -1 : (int)s;
     alm->time.tm_min  = (m == 0xFFu) ? -1 : (int)m;
     alm->time.tm_hour = (h == 0xFFu) ? -1 : (int)h;
     return 0;
 }
 
 int uiox_rtc_clock_alarm_write(uiox_rtc_clock_t *clk,
                                 const uiox_rtc_alarm_t *alm)
 {
     if (!clk || !alm) return -EINVAL;
     uint8_t s = (alm->time.tm_sec  < 0) ? 0xFFu : (uint8_t)alm->time.tm_sec;
     uint8_t m = (alm->time.tm_min  < 0) ? 0xFFu : (uint8_t)alm->time.tm_min;
     uint8_t h = (alm->time.tm_hour < 0) ? 0xFFu : (uint8_t)alm->time.tm_hour;
     return uiox_rtc_if_alarm_write(clk->rif, s, m, h, alm->enabled);
 }
 