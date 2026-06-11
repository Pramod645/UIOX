/**
 * @file  uiox_rtc_subsys.c
 * @brief UIOX RTC Subsystem — battery monitor, events, power.
 * @date  2026-06-10
 */

 #include "uiox_rtc_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_rtc_subsys_t *sys, uiox_rtc_ev_t ev,
                  const uiox_rtc_tm_t *tm)
 { if (sys->evt_cb) sys->evt_cb(ev, tm, sys->evt_ctx); }
 
 int uiox_rtc_subsys_init(uiox_rtc_subsys_t *sys, uiox_rtc_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_rtc_if_config(&sys->rif, hw);
     if (rc < 0) return rc;
     rc = uiox_rtc_clock_init(&sys->clk, &sys->rif);
     if (rc < 0) return rc;
     sys->bat_state = UIOX_RTC_BAT_UNKNOWN;
     sys->state     = UIOX_RTC_STATE_OFF;
     return 0;
 }
 
 int uiox_rtc_subsys_start(uiox_rtc_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_RTC_STATE_INIT;
 
     /* Battery check before anything else */
     sys->bat_state = uiox_rtc_hw_bat_check(sys->rif.hw);
     if (sys->bat_state == UIOX_RTC_BAT_LOW)
         fire(sys, UIOX_RTC_EV_BATTERY_LOW, NULL);
 
     int rc = uiox_rtc_if_start(&sys->rif);
     if (rc < 0) { sys->state = UIOX_RTC_STATE_ERROR; return rc; }
 
     sys->state = UIOX_RTC_STATE_READY;
     return 0;
 }
 
 void uiox_rtc_subsys_stop(uiox_rtc_subsys_t *sys)
 {
     if (!sys) return;
     uiox_rtc_if_stop(&sys->rif);
     sys->state = UIOX_RTC_STATE_OFF;
 }
 
 void uiox_rtc_subsys_tick(uiox_rtc_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_RTC_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* Poll IRQ status register */
     uiox_rtc_evt_t *e = uiox_rtc_if_irq_handle(&sys->rif, now_ms);
     if (!e) return;
 
     uiox_rtc_tm_t tm;
     bool have_time = (uiox_rtc_clock_read(&sys->clk, &tm) == 0);
 
     switch (e->type) {
     case UIOX_RTC_EVT_ALARM:
         sys->alarm_fire_count++;
         fire(sys, UIOX_RTC_EV_ALARM_FIRED, have_time ? &tm : NULL);
         break;
     case UIOX_RTC_EVT_PERIODIC:
         sys->periodic_count++;
         fire(sys, UIOX_RTC_EV_PERIODIC_TICK, have_time ? &tm : NULL);
         break;
     case UIOX_RTC_EVT_UPDATE:
         fire(sys, UIOX_RTC_EV_UPDATE_TICK, have_time ? &tm : NULL);
         /* Re-check battery every update tick */
         {
             uiox_rtc_bat_t b = uiox_rtc_hw_bat_check(sys->rif.hw);
             if (b != sys->bat_state) {
                 sys->bat_state = b;
                 fire(sys, (b == UIOX_RTC_BAT_LOW)
                           ? UIOX_RTC_EV_BATTERY_LOW
                           : UIOX_RTC_EV_BATTERY_RESTORED,
                      have_time ? &tm : NULL);
             }
         }
         break;
     default: break;
     }
     uiox_rtc_evt_free(e);
 }
 
 void uiox_rtc_subsys_set_cb(uiox_rtc_subsys_t *sys,
                               uiox_rtc_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_rtc_subsys_set_time(uiox_rtc_subsys_t *sys,
                                const uiox_rtc_tm_t *tm)
 {
     if (!sys || !tm) return -EINVAL;
     int rc = uiox_rtc_clock_write(&sys->clk, tm);
     if (rc == 0) fire(sys, UIOX_RTC_EV_TIME_SET, tm);
     return rc;
 }
 
 int uiox_rtc_subsys_get_time(uiox_rtc_subsys_t *sys, uiox_rtc_tm_t *tm)
 {
     if (!sys || !tm) return -EINVAL;
     return uiox_rtc_clock_read(&sys->clk, tm);
 }
 
 int uiox_rtc_subsys_set_alarm(uiox_rtc_subsys_t *sys,
                                 const uiox_rtc_alarm_t *alm)
 {
     if (!sys || !alm) return -EINVAL;
     return uiox_rtc_clock_alarm_write(&sys->clk, alm);
 }
 