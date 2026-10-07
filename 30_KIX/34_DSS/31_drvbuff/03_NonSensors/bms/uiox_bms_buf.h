/**
 * @file    uiox_bms_buf.h
 * @brief   UIOX BMS measurement log and telemetry pool.
 * @date    2026-06-04
 */

 #ifndef UIOX_BMS_BUF_H
 #define UIOX_BMS_BUF_H
 
 #include "uiox_bms_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BMS_EVENT_LOG_SIZE    64
 #define UIOX_BMS_TELEM_POOL_SIZE   8
 
 /* =========================================================================
  * BMS event types
  * ====================================================================== */
 
 typedef enum {
     UIOX_BMS_EV_PACK_INSERT   = 0,
     UIOX_BMS_EV_PACK_REMOVE,
     UIOX_BMS_EV_CHG_START,
     UIOX_BMS_EV_CHG_STOP,
     UIOX_BMS_EV_DSG_START,
     UIOX_BMS_EV_DSG_STOP,
     UIOX_BMS_EV_FULL,
     UIOX_BMS_EV_EMPTY,
     UIOX_BMS_EV_OVP,
     UIOX_BMS_EV_UVP,
     UIOX_BMS_EV_OCP_CHG,
     UIOX_BMS_EV_OCP_DSG,
     UIOX_BMS_EV_SCP,
     UIOX_BMS_EV_OTP,
     UIOX_BMS_EV_UTP,
     UIOX_BMS_EV_CELL_IMBALANCE,
     UIOX_BMS_EV_SOC_LOW,
     UIOX_BMS_EV_SOC_CRITICAL,
     UIOX_BMS_EV_FAULT,
 } uiox_bms_ev_t;
 
 /* =========================================================================
  * Event log entry
  * ====================================================================== */
 
 typedef struct {
     uiox_bms_ev_t type;
     uint32_t      ts_ms;
     uint32_t      soc_pct;       /**< SoC at event time (0..100)          */
     uint32_t      pack_mv;
     int32_t       current_ma;
     int16_t       temp_dc;       /**< Max temperature (°C × 10)           */
     uint32_t      fault_flags;
     bool          valid;
 } uiox_bms_event_t;
 
 /* =========================================================================
  * Telemetry snapshot
  * ====================================================================== */
 
 typedef struct uiox_bms_telem {
     uint32_t  ts_ms;
     uint32_t  pack_mv;
     int32_t   current_ma;
     uint32_t  cell_mv[UIOX_BMS_MAX_CELLS];
     int16_t   temp_dc[UIOX_BMS_MAX_TEMPS];
     uint8_t   soc_pct;
     uint8_t   soh_pct;
     int32_t   remain_mah;
     int32_t   full_mah;
     uint16_t  balance_mask;
     uint32_t  fault_flags;
     uint8_t   in_use;
     struct uiox_bms_telem *next;
 } uiox_bms_telem_t;
 
 /* =========================================================================
  * Buffer API
  * ====================================================================== */
 
 void uiox_bms_buf_init   (void);
 void uiox_bms_event_push (const uiox_bms_event_t *ev);
 bool uiox_bms_event_pop  (uiox_bms_event_t *ev);
 bool uiox_bms_event_empty(void);
 uint8_t uiox_bms_event_count(void);
 
 uiox_bms_telem_t *uiox_bms_telem_alloc(void);
 void              uiox_bms_telem_free (uiox_bms_telem_t *t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_BUF_H */
 