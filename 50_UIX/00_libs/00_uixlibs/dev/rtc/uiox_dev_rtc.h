/*
 * 50_UIX/00_libs/00_uixlibs/dev/rtc/uiox_dev_rtc.h
 *
 * RTC — the application's view of the real-time clock, at EL0.
 *
 * ── FAMILY 4 (REG) ──────────────────────────────────────────────────
 * Five commands, and get/set symmetry for both objects — the first
 * class in this layer where an app can set what it reads.
 *
 *   GET_TIME  0x01  uiox_rtc_time_t
 *   SET_TIME  0x02  uiox_rtc_time_t
 *   GET_ALARM 0x03  uiox_rtc_alarm_t
 *   SET_ALARM 0x04  uiox_rtc_alarm_t
 *   GET_STATS 0x05  uiox_rtc_if_stats_t
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_RTC_H
#define UIOX_DEV_RTC_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/03_NonSensors/rtc/uiox_rtc_if.h:58-62. */
#define UIOX_DEV_RTC_IOC_GET_TIME  0x01u
#define UIOX_DEV_RTC_IOC_SET_TIME  0x02u
#define UIOX_DEV_RTC_IOC_GET_ALARM 0x03u
#define UIOX_DEV_RTC_IOC_SET_ALARM 0x04u
#define UIOX_DEV_RTC_IOC_GET_STATS 0x05u

#define UIOX_DEV_RTC_DEVPATH  "rtc"

/* The kernel's uiox_rtc_time_t (uiox_rtc_if.h:64-67), mirrored.
 * FIVE SINGLE BYTES then a uint16_t — so there is one byte of padding
 * before `yr` and the struct is 8 bytes.  A copy using word-sized
 * members would read yr from the wrong offset and produce a plausible
 * wrong year rather than an error. */
typedef struct {
    unsigned char  s;
    unsigned char  m;
    unsigned char  h;
    unsigned char  md;
    unsigned char  mo;
    unsigned short yr;
} uiox_dev_rtc_time_t;

/* The kernel's uiox_rtc_alarm_t (uiox_rtc_if.h:69-72), mirrored.
 * NOTE THE FIELD ORDER: s, m, h, en — not h, m, s. */
typedef struct {
    unsigned char  s;
    unsigned char  m;
    unsigned char  h;
    unsigned char  en;
} uiox_dev_rtc_alarm_t;

typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_rtc_t;

uiox_dev_err_t uiox_dev_rtc_open (uiox_dev_rtc_t *r, int unit, int flags);
uiox_dev_err_t uiox_dev_rtc_close(uiox_dev_rtc_t *r);

uiox_dev_err_t uiox_dev_rtc_get_time(uiox_dev_rtc_t *r,
                                     uiox_dev_rtc_time_t *out);
uiox_dev_err_t uiox_dev_rtc_set_time(uiox_dev_rtc_t *r,
                                     const uiox_dev_rtc_time_t *in);

uiox_dev_err_t uiox_dev_rtc_get_alarm(uiox_dev_rtc_t *r,
                                      uiox_dev_rtc_alarm_t *out);
uiox_dev_err_t uiox_dev_rtc_set_alarm(uiox_dev_rtc_t *r,
                                      const uiox_dev_rtc_alarm_t *in);

uiox_dev_err_t uiox_dev_rtc_get_stats(uiox_dev_rtc_t *r, void *out);

#endif
