/**
 * @file  uiox_chg_buf.h
 * @brief UIOX Charger event / fault queue buffer pool.
 * @date  2026-06-11
 */

 #ifndef UIOX_CHG_BUF_H
 #define UIOX_CHG_BUF_H
 
 #include "uiox_chg_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CHG_EVT_POOL_SIZE      16u
 #define UIOX_CHG_FAULT_POOL_SIZE    8u
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_EVT_NONE        = 0,
     UIOX_CHG_EVT_PLUG_IN,            /**< Input source connected          */
     UIOX_CHG_EVT_PLUG_OUT,           /**< Input source removed            */
     UIOX_CHG_EVT_CHRG_START,         /**< Charging started                */
     UIOX_CHG_EVT_CHRG_DONE,          /**< Charge cycle complete           */
     UIOX_CHG_EVT_FAULT,              /**< Fault condition asserted        */
     UIOX_CHG_EVT_FAULT_CLEAR,        /**< Fault condition cleared         */
     UIOX_CHG_EVT_PD_CONTRACT,        /**< USB-C PD contract negotiated    */
     UIOX_CHG_EVT_PD_RESET,           /**< USB-C PD hard reset             */
     UIOX_CHG_EVT_OTG_ON,             /**< OTG boost output enabled        */
     UIOX_CHG_EVT_OTG_OFF,            /**< OTG boost output disabled       */
     UIOX_CHG_EVT_THERMAL_THROTTLE,   /**< Charge current derated          */
     UIOX_CHG_EVT_WATCHDOG,           /**< Host watchdog kicked            */
 } uiox_chg_evt_type_t;
 
 typedef struct {
     uiox_chg_evt_type_t type;
     uint32_t            timestamp_ms;
     uiox_chg_src_t      src;         /**< Source at event time            */
     uiox_chg_chrg_t     chrg;        /**< Charge state at event time      */
     uint32_t            fault_flags;
     int32_t             vbus_mv;
     int32_t             ibat_ma;
     uint8_t             in_use;
 } uiox_chg_evt_t;
 
 /* =========================================================================
  * Fault record
  * ====================================================================== */
 
 typedef struct {
     uint32_t fault_flags;
     uint32_t timestamp_ms;
     int32_t  vbus_mv;
     int32_t  tdie_mc;             /**< Die temperature in milli-°C        */
     uint8_t  in_use;
 } uiox_chg_fault_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void              uiox_chg_buf_init      (void);
 
 uiox_chg_evt_t   *uiox_chg_evt_alloc    (void);
 void              uiox_chg_evt_free      (uiox_chg_evt_t *e);
 uint8_t           uiox_chg_evt_free_cnt  (void);
 
 uiox_chg_fault_t *uiox_chg_fault_alloc  (void);
 void              uiox_chg_fault_free    (uiox_chg_fault_t *f);
 uint8_t           uiox_chg_fault_free_cnt(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_BUF_H */
 