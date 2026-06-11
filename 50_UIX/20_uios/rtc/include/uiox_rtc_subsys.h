/**
 * @file  uiox_rtc_subsys.h
 * @brief UIOX RTC Subsystem — battery monitor, events, power management.
 * @date  2026-06-10
 */

 #ifndef UIOX_RTC_SUBSYS_H
 #define UIOX_RTC_SUBSYS_H
 
 #include "uiox_rtc_clock.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Events
  * ====================================================================== */
 
 typedef enum {
     UIOX_RTC_EV_ALARM_FIRED      = 0,
     UIOX_RTC_EV_PERIODIC_TICK,
     UIOX_RTC_EV_UPDATE_TICK,
     UIOX_RTC_EV_TIME_SET,
     UIOX_RTC_EV_BATTERY_LOW,
     UIOX_RTC_EV_BATTERY_RESTORED,
     UIOX_RTC_EV_OSCILLATOR_FAIL,
     UIOX_RTC_EV_ERROR,
 } uiox_rtc_ev_t;
 
 typedef void (*uiox_rtc_evt_cb_t)(uiox_rtc_ev_t ev,
                                    const uiox_rtc_tm_t *tm, void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_RTC_STATE_OFF   = 0,
     UIOX_RTC_STATE_INIT,
     UIOX_RTC_STATE_READY,
     UIOX_RTC_STATE_ERROR,
 } uiox_rtc_state_t;
 
 typedef struct {
     uiox_rtc_if_t       rif;
     uiox_rtc_clock_t    clk;
     uiox_rtc_state_t    state;
     uiox_rtc_bat_t      bat_state;
     uiox_rtc_evt_cb_t   evt_cb;
     void               *evt_ctx;
     uint32_t            tick_count;
     uint64_t            uptime_ms;
     uint32_t            alarm_fire_count;
     uint32_t            periodic_count;
 } uiox_rtc_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_rtc_subsys_init   (uiox_rtc_subsys_t *sys, uiox_rtc_hw_t *hw);
 int  uiox_rtc_subsys_start  (uiox_rtc_subsys_t *sys);
 void uiox_rtc_subsys_stop   (uiox_rtc_subsys_t *sys);
 void uiox_rtc_subsys_tick   (uiox_rtc_subsys_t *sys, uint32_t now_ms);
 void uiox_rtc_subsys_set_cb (uiox_rtc_subsys_t *sys,
                               uiox_rtc_evt_cb_t cb, void *ctx);
 int  uiox_rtc_subsys_set_time(uiox_rtc_subsys_t *sys,
                                const uiox_rtc_tm_t *tm);
 int  uiox_rtc_subsys_get_time(uiox_rtc_subsys_t *sys, uiox_rtc_tm_t *tm);
 int  uiox_rtc_subsys_set_alarm(uiox_rtc_subsys_t *sys,
                                 const uiox_rtc_alarm_t *alm);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_SUBSYS_H */
 