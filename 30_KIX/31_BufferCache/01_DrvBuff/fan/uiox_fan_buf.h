/**
 * @file    uiox_fan_buf.h
 * @brief   UIOX Fan Controller telemetry log and event pool.
 * @date    2026-06-05
 */

 #ifndef UIOX_FAN_BUF_H
 #define UIOX_FAN_BUF_H
 
 #include "uiox_fan_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FAN_EVENT_LOG_SIZE    64
 #define UIOX_FAN_TELEM_POOL_SIZE   8
 
 /* =========================================================================
  * Fan event types
  * ====================================================================== */
 
 typedef enum {
     UIOX_FAN_EV_START        = 0,
     UIOX_FAN_EV_STOP,
     UIOX_FAN_EV_STALL,
     UIOX_FAN_EV_STALL_CLEAR,
     UIOX_FAN_EV_SPIN_UP_FAIL,
     UIOX_FAN_EV_OVERHEAT,
     UIOX_FAN_EV_TEMP_OK,
     UIOX_FAN_EV_PWM_CHANGE,
     UIOX_FAN_EV_FAULT,
     UIOX_FAN_EV_WATCHDOG,
     UIOX_FAN_EV_MANUAL_OVERRIDE,
     UIOX_FAN_EV_AUTO_RESTORE,
 } uiox_fan_ev_t;
 
 /* =========================================================================
  * Event log entry
  * ====================================================================== */
 
 typedef struct {
     uiox_fan_ev_t  type;
     uint8_t        fan_id;     /**< Fan channel index (0xFF = global)      */
     uint8_t        pwm_duty;
     uint16_t       rpm;
     int16_t        temp_dc;    /**< Temperature at event time (°C × 10)    */
     uint32_t       ts_ms;
     uint32_t       fault_flags;
     bool           valid;
 } uiox_fan_event_t;
 
 /* =========================================================================
  * Telemetry snapshot
  * ====================================================================== */
 
 typedef struct uiox_fan_telem {
     uint32_t  ts_ms;
     uint16_t  rpm[UIOX_FAN_MAX_CHANNELS];
     uint8_t   pwm[UIOX_FAN_MAX_CHANNELS];
     int16_t   temp_dc[UIOX_FAN_MAX_TEMP_SENSORS];
     uint32_t  fault_flags;
     uint8_t   in_use;
     struct uiox_fan_telem *next;
 } uiox_fan_telem_t;
 
 /* =========================================================================
  * Buffer API
  * ====================================================================== */
 
 void uiox_fan_buf_init   (void);
 void uiox_fan_event_push (const uiox_fan_event_t *ev);
 bool uiox_fan_event_pop  (uiox_fan_event_t *ev);
 bool uiox_fan_event_empty(void);
 uint8_t uiox_fan_event_count(void);
 
 uiox_fan_telem_t *uiox_fan_telem_alloc(void);
 void              uiox_fan_telem_free (uiox_fan_telem_t *t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FAN_BUF_H */
 