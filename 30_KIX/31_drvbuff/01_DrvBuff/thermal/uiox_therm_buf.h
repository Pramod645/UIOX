/**
 * @file    uiox_therm_buf.h
 * @brief   UIOX Thermal Sensor measurement log and alert pool.
 * @date    2026-06-05
 */

 #ifndef UIOX_THERM_BUF_H
 #define UIOX_THERM_BUF_H
 
 #include "uiox_therm_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_THERM_EVENT_LOG_SIZE    64
 #define UIOX_THERM_TELEM_POOL_SIZE   8
 
 /* =========================================================================
  * Thermal event types
  * ====================================================================== */
 
 typedef enum {
     UIOX_THERM_EV_ALERT_HIGH    = 0,
     UIOX_THERM_EV_ALERT_CLEAR,
     UIOX_THERM_EV_CRITICAL,
     UIOX_THERM_EV_TRIP_CROSSED,
     UIOX_THERM_EV_TRIP_CLEARED,
     UIOX_THERM_EV_THROTTLE_ON,
     UIOX_THERM_EV_THROTTLE_OFF,
     UIOX_THERM_EV_SENSOR_ERROR,
     UIOX_THERM_EV_SENSOR_RECOVER,
     UIOX_THERM_EV_ZONE_HOT,
     UIOX_THERM_EV_ZONE_COOL,
 } uiox_therm_ev_t;
 
 /* =========================================================================
  * Event log entry
  * ====================================================================== */
 
 typedef struct {
     uiox_therm_ev_t  type;
     uint8_t          sensor_id;
     uint8_t          zone_id;
     int16_t          temp_dc;    /**< Temperature at event (°C × 10)      */
     int16_t          threshold_dc;
     uint32_t         ts_ms;
     bool             valid;
 } uiox_therm_event_t;
 
 /* =========================================================================
  * Telemetry snapshot
  * ====================================================================== */
 
 typedef struct uiox_therm_telem {
     uint32_t  ts_ms;
     int16_t   temp_dc[UIOX_THERM_MAX_CHANNELS];
     uint8_t   num_channels;
     bool      alert[UIOX_THERM_MAX_CHANNELS];
     uint8_t   in_use;
     struct uiox_therm_telem *next;
 } uiox_therm_telem_t;
 
 /* =========================================================================
  * Buffer API
  * ====================================================================== */
 
 void uiox_therm_buf_init   (void);
 void uiox_therm_event_push (const uiox_therm_event_t *ev);
 bool uiox_therm_event_pop  (uiox_therm_event_t *ev);
 bool uiox_therm_event_empty(void);
 uint8_t uiox_therm_event_count(void);
 
 uiox_therm_telem_t *uiox_therm_telem_alloc(void);
 void                uiox_therm_telem_free (uiox_therm_telem_t *t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_THERM_BUF_H */
 