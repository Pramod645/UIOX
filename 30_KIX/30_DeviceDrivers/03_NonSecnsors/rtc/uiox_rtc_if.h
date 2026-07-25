/**
 * @file  uiox_rtc_if.h
 * @brief UIOX RTC interface driver — register access, IRQ, UIP handling.
 * @date  2026-06-10
 */

 #ifndef UIOX_RTC_IF_H
 #define UIOX_RTC_IF_H
 
 #include "uiox_rtc_hw.h"
 #include "uiox_rtc_buf.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  irq_alarm;
     uint64_t  irq_periodic;
     uint64_t  irq_update;
     uint32_t  uip_timeouts;
     uint32_t  bat_low_events;
     uint32_t  errors;
 } uiox_rtc_if_stats_t;
 
 typedef struct {
     uiox_rtc_hw_t      *hw;
     uiox_rtc_if_stats_t stats;
     bool                primed;
 } uiox_rtc_if_t;
 
 int  uiox_rtc_if_config    (uiox_rtc_if_t *rif, uiox_rtc_hw_t *hw);
 int  uiox_rtc_if_start     (uiox_rtc_if_t *rif);
 void uiox_rtc_if_stop      (uiox_rtc_if_t *rif);
 
 /* Read/write time through UIP guard */
 int  uiox_rtc_if_time_read (uiox_rtc_if_t *rif,
                              uint8_t *s, uint8_t *m, uint8_t *h,
                              uint8_t *md, uint8_t *mo, uint16_t *yr);
 int  uiox_rtc_if_time_write(uiox_rtc_if_t *rif,
                              uint8_t s, uint8_t m, uint8_t h,
                              uint8_t md, uint8_t mo, uint16_t yr);
 
 /* Alarm */
 int  uiox_rtc_if_alarm_read (uiox_rtc_if_t *rif,
                               uint8_t *s, uint8_t *m, uint8_t *h);
 int  uiox_rtc_if_alarm_write(uiox_rtc_if_t *rif,
                               uint8_t s, uint8_t m, uint8_t h, bool en);
 
 /* IRQ handler — call from platform ISR */
 uiox_rtc_evt_t *uiox_rtc_if_irq_handle(uiox_rtc_if_t *rif,
                                          uint32_t now_ms);
 
 void uiox_rtc_if_stats_get (const uiox_rtc_if_t *rif,
                               uiox_rtc_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_IF_H */
 