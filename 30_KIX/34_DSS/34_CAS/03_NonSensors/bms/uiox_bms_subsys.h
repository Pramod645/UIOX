/**
 * @file    uiox_bms_subsys.h
 * @brief   UIOX BMS subsystem — protection, sequencing, events.
 * @date    2026-06-04
 */

 #ifndef UIOX_BMS_SUBSYS_H
 #define UIOX_BMS_SUBSYS_H
 
 #include "uiox_bms_algo.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef void (*uiox_bms_evt_cb_t)(uiox_bms_ev_t evt, void *ctx);
 
 typedef enum {
     UIOX_BMS_SUBSYS_STOPPED = 0,
     UIOX_BMS_SUBSYS_IDLE,
     UIOX_BMS_SUBSYS_CHARGING,
     UIOX_BMS_SUBSYS_DISCHARGING,
     UIOX_BMS_SUBSYS_FAULT,
     UIOX_BMS_SUBSYS_BALANCED,
 } uiox_bms_subsys_state_t;
 
 typedef struct {
     uiox_bms_if_t           bif;
     uiox_bms_bal_t           bal;
     uiox_bms_algo_t          algo;
     uiox_bms_subsys_state_t  state;
     uiox_bms_evt_cb_t        evt_cb;
     void                    *evt_ctx;
     uint32_t                 tick_count;
     uint64_t                 uptime_ms;
     uint32_t                 last_meas_ms;
     uint32_t                 meas_interval_ms;
     uint32_t                 soc_low_pct;     /**< Low SoC alert threshold */
     uint32_t                 soc_crit_pct;    /**< Critical SoC threshold  */
 } uiox_bms_subsys_t;
 
 int  uiox_bms_subsys_init    (uiox_bms_subsys_t      *sys,
                                uiox_bms_hw_t          *hw,
                                const uiox_bms_batt_t  *batt);
 int  uiox_bms_subsys_start   (uiox_bms_subsys_t *sys);
 void uiox_bms_subsys_stop    (uiox_bms_subsys_t *sys);
 void uiox_bms_subsys_tick    (uiox_bms_subsys_t *sys, uint32_t now_ms);
 void uiox_bms_subsys_set_cb  (uiox_bms_subsys_t *sys,
                                uiox_bms_evt_cb_t cb, void *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_SUBSYS_H */
 