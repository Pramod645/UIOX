/**
 * @file  uiox_chg_subsys.h
 * @brief UIOX Charger Subsystem — safety, thermal, fault events.
 * @date  2026-06-11
 */

 #ifndef UIOX_CHG_SUBSYS_H
 #define UIOX_CHG_SUBSYS_H
 
 #include "uiox_chg_policy.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_EV_PLUG_IN          = 0,
     UIOX_CHG_EV_PLUG_OUT,
     UIOX_CHG_EV_CHRG_START,
     UIOX_CHG_EV_CHRG_DONE,
     UIOX_CHG_EV_FAULT,
     UIOX_CHG_EV_FAULT_CLEAR,
     UIOX_CHG_EV_PD_CONTRACT,
     UIOX_CHG_EV_PD_RESET,
     UIOX_CHG_EV_OTG_ON,
     UIOX_CHG_EV_OTG_OFF,
     UIOX_CHG_EV_THERMAL_THROTTLE,
     UIOX_CHG_EV_THERMAL_RESUME,
     UIOX_CHG_EV_ERROR,
 } uiox_chg_ev_t;
 
 typedef void (*uiox_chg_evt_cb_t)(uiox_chg_ev_t ev,
                                    const uiox_chg_evt_t *data,
                                    void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_STATE_OFF    = 0,
     UIOX_CHG_STATE_INIT,
     UIOX_CHG_STATE_READY,
     UIOX_CHG_STATE_FAULT,
     UIOX_CHG_STATE_ERROR,
 } uiox_chg_state_t;
 
 /* Thermal throttle thresholds */
 #define UIOX_CHG_TDIE_THROTTLE_MC   800000   /**< 80.0 °C in milli-°C    */
 #define UIOX_CHG_TDIE_RESUME_MC     700000   /**< 70.0 °C in milli-°C    */
 
 typedef struct {
     uiox_chg_if_t        cif;
     uiox_chg_policy_t    policy;
     uiox_chg_state_t     state;
     uiox_chg_evt_cb_t    evt_cb;
     void                *evt_ctx;
     /* Statistics */
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             fault_count;
     uint32_t             charge_cycles;
     uint32_t             pd_contracts;
     bool                 throttled;
     bool                 otg_active;
 } uiox_chg_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_chg_subsys_init      (uiox_chg_subsys_t *sys,
                                  uiox_chg_hw_t *hw,
                                  const uiox_chg_profile_t *profile);
 int  uiox_chg_subsys_start     (uiox_chg_subsys_t *sys);
 void uiox_chg_subsys_stop      (uiox_chg_subsys_t *sys);
 void uiox_chg_subsys_tick      (uiox_chg_subsys_t *sys, uint32_t now_ms);
 void uiox_chg_subsys_set_cb    (uiox_chg_subsys_t *sys,
                                  uiox_chg_evt_cb_t cb, void *ctx);
 int  uiox_chg_subsys_enable    (uiox_chg_subsys_t *sys, bool en);
 int  uiox_chg_subsys_otg       (uiox_chg_subsys_t *sys, bool en);
 int  uiox_chg_subsys_set_profile(uiox_chg_subsys_t *sys,
                                   const uiox_chg_profile_t *profile);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_SUBSYS_H */
 