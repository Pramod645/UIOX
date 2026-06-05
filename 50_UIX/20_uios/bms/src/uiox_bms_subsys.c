/**
 * @file    uiox_bms_subsys.c
 * @brief   UIOX BMS subsystem implementation.
 * @date    2026-06-04
 */

 #include "uiox_bms_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_bms_subsys_t *sys, uiox_bms_ev_t evt)
 { if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx); }
 
 int uiox_bms_subsys_init(uiox_bms_subsys_t     *sys,
                           uiox_bms_hw_t         *hw,
                           const uiox_bms_batt_t *batt)
 {
     if (!sys || !hw || !batt) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_bms_if_config(&sys->bif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_bms_bal_init(&sys->bal, &sys->bif,
                             UIOX_BMS_BAL_MODE_PASSIVE,
                             UIOX_BMS_BAL_DELTA_MV_DEFAULT,
                             UIOX_BMS_BAL_STOP_MV_DEFAULT);
     if (rc < 0) return rc;
 
     rc = uiox_bms_algo_init(&sys->algo, batt);
     if (rc < 0) return rc;
 
     sys->meas_interval_ms = 1000u;
     sys->soc_low_pct      = 15u;
     sys->soc_crit_pct     = 5u;
     sys->state            = UIOX_BMS_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_bms_subsys_start(uiox_bms_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_bms_if_start(&sys->bif);
     if (rc < 0) return rc;
     sys->state = uiox_bms_hw_pack_present(sys->bif.hw) ?
                  UIOX_BMS_SUBSYS_IDLE :
                  UIOX_BMS_SUBSYS_STOPPED;
     fire(sys, UIOX_BMS_EV_PACK_INSERT);
     return 0;
 }
 
 void uiox_bms_subsys_stop(uiox_bms_subsys_t *sys)
 {
     if (!sys) return;
     uiox_bms_bal_stop(&sys->bal);
     uiox_bms_if_stop(&sys->bif);
     sys->state = UIOX_BMS_SUBSYS_STOPPED;
     fire(sys, UIOX_BMS_EV_PACK_REMOVE);
 }
 
 void uiox_bms_subsys_tick(uiox_bms_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_BMS_SUBSYS_STOPPED) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* IRQ / fault check */
     int faults = uiox_bms_if_irq_handle(&sys->bif, now_ms);
     if (faults > 0) {
         sys->state = UIOX_BMS_SUBSYS_FAULT;
         fire(sys, UIOX_BMS_EV_FAULT);
         /* Disable FETs on unclearable fault */
         if (sys->bif.hw->fault_flags &
             (UIOX_BMS_FAULT_OVP | UIOX_BMS_FAULT_SCP)) {
             uiox_bms_hw_set_chg_fet(sys->bif.hw, false);
             uiox_bms_hw_set_dsg_fet(sys->bif.hw, false);
         }
     }
 
     /* Periodic measurement */
     if ((now_ms - sys->last_meas_ms) >= sys->meas_interval_ms) {
         sys->last_meas_ms = now_ms;
         uiox_bms_if_measure(&sys->bif);
 
         uint32_t dt = sys->meas_interval_ms;
         uiox_bms_algo_update_soc(&sys->algo,
                                    sys->bif.hw->current_ma,
                                    sys->bif.hw->pack_mv,
                                    dt);
         uiox_bms_algo_update_tte(&sys->algo, sys->bif.hw->current_ma);
 
         /* Detect charge state */
         if (sys->bif.hw->current_ma > 50 &&
             sys->state != UIOX_BMS_SUBSYS_CHARGING) {
             sys->state = UIOX_BMS_SUBSYS_CHARGING;
             fire(sys, UIOX_BMS_EV_CHG_START);
         } else if (sys->bif.hw->current_ma < -50 &&
                    sys->state != UIOX_BMS_SUBSYS_DISCHARGING) {
             sys->state = UIOX_BMS_SUBSYS_DISCHARGING;
             fire(sys, UIOX_BMS_EV_DSG_START);
         }
 
         /* Full charge detection */
         if (uiox_bms_algo_check_full(&sys->algo,
                                       sys->bif.hw->pack_mv,
                                       sys->bif.hw->current_ma))
             fire(sys, UIOX_BMS_EV_FULL);
 
         /* Low / critical SoC */
         if (sys->algo.soc_pct <= sys->soc_crit_pct)
             fire(sys, UIOX_BMS_EV_SOC_CRITICAL);
         else if (sys->algo.soc_pct <= sys->soc_low_pct)
             fire(sys, UIOX_BMS_EV_SOC_LOW);
     }
 
     /* Cell balancing tick */
     if (sys->state == UIOX_BMS_SUBSYS_CHARGING)
         uiox_bms_bal_tick(&sys->bal, now_ms);
 
     /* Dispatch events to callback */
     uiox_bms_event_t ev;
     while (!uiox_bms_event_empty())
         if (uiox_bms_event_pop(&ev)) fire(sys, ev.type);
 }
 
 void uiox_bms_subsys_set_cb(uiox_bms_subsys_t *sys,
                               uiox_bms_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 