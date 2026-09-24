/**
 * @file    uiox_pmic_buf.h
 * @brief   UIOX PMIC event log and telemetry buffer pool.
 * @date    2026-06-04
 */

 #ifndef UIOX_PMIC_BUF_H
 #define UIOX_PMIC_BUF_H
 
 #include "uiox_pmic_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_PMIC_EVENT_LOG_SIZE    64
 #define UIOX_PMIC_TELEM_POOL_SIZE   8
 
 /* =========================================================================
  * PMIC event types
  * ====================================================================== */
 
 typedef enum {
     UIOX_PMIC_EV_POWER_ON   = 0,
     UIOX_PMIC_EV_POWER_OFF,
     UIOX_PMIC_EV_RAIL_ON,
     UIOX_PMIC_EV_RAIL_OFF,
     UIOX_PMIC_EV_OTP,
     UIOX_PMIC_EV_OCP,
     UIOX_PMIC_EV_OVP,
     UIOX_PMIC_EV_UVP,
     UIOX_PMIC_EV_WDT,
     UIOX_PMIC_EV_PGOOD_LOST,
     UIOX_PMIC_EV_DVFS_UP,
     UIOX_PMIC_EV_DVFS_DOWN,
     UIOX_PMIC_EV_SLEEP,
     UIOX_PMIC_EV_WAKE,
     UIOX_PMIC_EV_FAULT,
 } uiox_pmic_ev_t;
 
 /* =========================================================================
  * Event log entry
  * ====================================================================== */
 
 typedef struct {
     uiox_pmic_ev_t  type;
     uint8_t         rail_id;   /**< Rail index (0xFF = global)            */
     uint32_t        mv;        /**< Voltage at event time (mV)            */
     uint32_t        ts_ms;     /**< Timestamp (ms since boot)             */
     uint32_t        fault_flags;
     bool            valid;
 } uiox_pmic_event_t;
 
 /* =========================================================================
  * Telemetry snapshot
  * ====================================================================== */
 
 typedef struct uiox_pmic_telem {
     uint32_t ts_ms;
     uint32_t vsys_mv;
     uint32_t vbat_mv;
     uint32_t ibat_ma;   /**< Battery current (positive=charging)         */
     uint32_t vbus_mv;
     int8_t   die_temp_c;
     int8_t   ntc_temp_c;
     uint8_t  in_use;
     struct uiox_pmic_telem *next;
 } uiox_pmic_telem_t;
 
 /* =========================================================================
  * Buffer API
  * ====================================================================== */
 
 void uiox_pmic_buf_init(void);
 
 /* Event log (circular, FIFO) */
 void uiox_pmic_event_push (const uiox_pmic_event_t *ev);
 bool uiox_pmic_event_pop  (uiox_pmic_event_t *ev);
 bool uiox_pmic_event_empty(void);
 uint8_t uiox_pmic_event_count(void);
 
 /* Telemetry pool */
 uiox_pmic_telem_t *uiox_pmic_telem_alloc(void);
 void               uiox_pmic_telem_free (uiox_pmic_telem_t *t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_BUF_H */
 