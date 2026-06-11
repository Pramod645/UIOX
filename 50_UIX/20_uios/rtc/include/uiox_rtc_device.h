/**
 * @file  uiox_rtc_device.h
 * @brief UIOX RTC application-facing device API (Layer 5).
 * @date  2026-06-10
 */

 #ifndef UIOX_RTC_DEVICE_H
 #define UIOX_RTC_DEVICE_H
 
 #include "uiox_rtc_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_rtc_hw_t            *hw;
     const uiox_rtc_hw_ops_t  *hw_ops;
     uiox_rtc_evt_cb_t         evt_cb;
     void                     *evt_ctx;
 } uiox_rtc_open_params_t;
 
 typedef struct {
     uiox_rtc_subsys_t  subsys;
     uiox_rtc_hw_t     *hw;
     bool               open;
 } uiox_rtc_device_t;
 
 /* Lifecycle */
 int  uiox_rtc_open        (uiox_rtc_device_t *dev,
                             const uiox_rtc_open_params_t *p);
 int  uiox_rtc_start       (uiox_rtc_device_t *dev);
 void uiox_rtc_stop        (uiox_rtc_device_t *dev);
 void uiox_rtc_close       (uiox_rtc_device_t *dev);
 void uiox_rtc_tick        (uiox_rtc_device_t *dev, uint32_t now_ms);
 
 /* Time access */
 int  uiox_rtc_get_time    (uiox_rtc_device_t *dev, uiox_rtc_tm_t *tm);
 int  uiox_rtc_set_time    (uiox_rtc_device_t *dev,
                             const uiox_rtc_tm_t *tm);
 
 /* Alarm */
 int  uiox_rtc_set_alarm   (uiox_rtc_device_t *dev,
                             const uiox_rtc_alarm_t *alm);
 int  uiox_rtc_get_alarm   (uiox_rtc_device_t *dev, uiox_rtc_alarm_t *alm);
 
 /* Info / stats */
 void uiox_rtc_print_info  (const uiox_rtc_device_t *dev);
 void uiox_rtc_print_stats (uiox_rtc_device_t *dev);
 
 /* Name helpers */
 const char *uiox_rtc_state_name(uiox_rtc_state_t s);
 const char *uiox_rtc_ev_name   (uiox_rtc_ev_t ev);
 const char *uiox_rtc_bat_name  (uiox_rtc_bat_t b);
 const char *uiox_rtc_ver_name  (uiox_rtc_ver_t v);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_DEVICE_H */
 