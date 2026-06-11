/**
 * @file  uiox_rtc_clock.h
 * @brief UIOX RTC clock layer — broken-down time, alarm, epoch conversion.
 * @date  2026-06-10
 */

 #ifndef UIOX_RTC_CLOCK_H
 #define UIOX_RTC_CLOCK_H
 
 #include "uiox_rtc_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Broken-down time (mirrors struct tm, no tz)
  * ====================================================================== */
 
 typedef struct {
     int tm_sec;    /**< [0, 59]                                            */
     int tm_min;    /**< [0, 59]                                            */
     int tm_hour;   /**< [0, 23]                                            */
     int tm_mday;   /**< [1, 31]                                            */
     int tm_mon;    /**< [0, 11]  0 = January                              */
     int tm_year;   /**< years since 1900                                   */
     int tm_wday;   /**< [0, 6]   0 = Sunday                               */
     int tm_isdst;  /**< -1/0/1                                             */
 } uiox_rtc_tm_t;
 
 typedef struct {
     uiox_rtc_tm_t time;
     bool          enabled;
     bool          pending;
 } uiox_rtc_alarm_t;
 
 typedef struct {
     uiox_rtc_if_t  *rif;
 } uiox_rtc_clock_t;
 
 /* =========================================================================
  * Clock API
  * ====================================================================== */
 
 int  uiox_rtc_clock_init        (uiox_rtc_clock_t *clk,
                                   uiox_rtc_if_t *rif);
 int  uiox_rtc_clock_read        (uiox_rtc_clock_t *clk, uiox_rtc_tm_t *tm);
 int  uiox_rtc_clock_write       (uiox_rtc_clock_t *clk,
                                   const uiox_rtc_tm_t *tm);
 int  uiox_rtc_clock_alarm_read  (uiox_rtc_clock_t *clk,
                                   uiox_rtc_alarm_t *alm);
 int  uiox_rtc_clock_alarm_write (uiox_rtc_clock_t *clk,
                                   const uiox_rtc_alarm_t *alm);
 int  uiox_rtc_clock_valid       (const uiox_rtc_tm_t *tm);
 
 /* Epoch helpers */
 int64_t uiox_rtc_tm_to_epoch (const uiox_rtc_tm_t *tm);
 void    uiox_rtc_epoch_to_tm (int64_t epoch, uiox_rtc_tm_t *tm);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_CLOCK_H */
 