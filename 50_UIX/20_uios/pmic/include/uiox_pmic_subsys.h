/**
 * @file    uiox_pmic_subsys.h
 * @brief   UIOX PMIC subsystem — sequencing, thermal, events.
 * @date    2026-06-04
 */

 #ifndef UIOX_PMIC_SUBSYS_H
 #define UIOX_PMIC_SUBSYS_H
 
 #include "uiox_pmic_policy.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef void (*uiox_pmic_evt_cb_t)(uiox_pmic_ev_t evt,
                                     uint8_t rail_id, void *ctx);
 
 typedef enum {
     UIOX_PMIC_SUBSYS_STOPPED = 0,
     UIOX_PMIC_SUBSYS_RUNNING,
     UIOX_PMIC_SUBSYS_FAULT,
     UIOX_PMIC_SUBSYS_SLEEP,
 } uiox_pmic_subsys_state_t;
 
 typedef struct {
     uiox_pmic_if_t           pif;
     uiox_pmic_rail_mgr_t     mgr;
     uiox_pmic_policy_t       policy;
     uiox_pmic_subsys_state_t state;
     uiox_pmic_evt_cb_t       evt_cb;
     void                    *evt_ctx;
     uint32_t                 tick_count;
     uint64_t                 uptime_ms;
     uint32_t                 wdt_kick_interval_ms;
     uint32_t                 last_wdt_kick_ms;
     uint32_t                 telem_interval_ms;
     uint32_t                 last_telem_ms;
 } uiox_pmic_subsys_t;
 
 int  uiox_pmic_subsys_init   (uiox_pmic_subsys_t *sys,
                                uiox_pmic_hw_t     *hw);
 int  uiox_pmic_subsys_start  (uiox_pmic_subsys_t *sys);
 void uiox_pmic_subsys_stop   (uiox_pmic_subsys_t *sys);
 void uiox_pmic_subsys_tick   (uiox_pmic_subsys_t *sys, uint32_t now_ms);
 
 void uiox_pmic_subsys_set_cb (uiox_pmic_subsys_t *sys,
                                uiox_pmic_evt_cb_t cb, void *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_SUBSYS_H */
 