/**
 * @file    uiox_pmic_subsys.c
 * @brief   UIOX PMIC subsystem implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_pmic_subsys_t *sys,
                  uiox_pmic_ev_t evt, uint8_t rail_id)
 {
     if (sys->evt_cb) sys->evt_cb(evt, rail_id, sys->evt_ctx);
 }
 
 int uiox_pmic_subsys_init(uiox_pmic_subsys_t *sys, uiox_pmic_hw_t *hw)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_pmic_if_config(&sys->pif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_pmic_rail_init(&sys->mgr, &sys->pif);
     if (rc < 0) return rc;
 
     static const uiox_pmic_thermal_cfg_t thermal_cfg = {
         .throttle_temp_c = 85,
         .critical_temp_c = 105,
         .resume_temp_c   = 75,
         .throttle_mv     = 900u,
     };
     rc = uiox_pmic_policy_init(&sys->policy, &sys->mgr,
                                 &thermal_cfg,
                                 "VCORE", "VMEM", "VIO");
     if (rc < 0) return rc;
 
     sys->wdt_kick_interval_ms = 5000u;
     sys->telem_interval_ms    = 1000u;
     sys->state                = UIOX_PMIC_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_pmic_subsys_start(uiox_pmic_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_pmic_if_start(&sys->pif);
     if (rc < 0) return rc;
     sys->state = UIOX_PMIC_SUBSYS_RUNNING;
     uiox_pmic_event_t ev = { .type=UIOX_PMIC_EV_POWER_ON, .valid=true };
     uiox_pmic_event_push(&ev);
     fire(sys, UIOX_PMIC_EV_POWER_ON, 0xFFu);
     return 0;
 }
 
 void uiox_pmic_subsys_stop(uiox_pmic_subsys_t *sys)
 {
     if (!sys) return;
     uiox_pmic_if_stop(&sys->pif);
     sys->state = UIOX_PMIC_SUBSYS_STOPPED;
     uiox_pmic_event_t ev = { .type=UIOX_PMIC_EV_POWER_OFF, .valid=true };
     uiox_pmic_event_push(&ev);
 }
 
 void uiox_pmic_subsys_tick(uiox_pmic_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_PMIC_SUBSYS_STOPPED) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* IRQ handling */
     int faults = uiox_pmic_if_irq_handle(&sys->pif, now_ms);
     if (faults > 0) {
         sys->state = UIOX_PMIC_SUBSYS_FAULT;
         fire(sys, UIOX_PMIC_EV_FAULT, 0xFFu);
     }
 
     /* Telemetry */
     if ((now_ms - sys->last_telem_ms) >= sys->telem_interval_ms) {
         uiox_pmic_telem_t snap;
         uiox_pmic_if_telemetry(&sys->pif, &snap, now_ms);
         sys->last_telem_ms = now_ms;
         uiox_pmic_policy_thermal_tick(&sys->policy, snap.die_temp_c);
         if (sys->policy.throttled)
             fire(sys, UIOX_PMIC_EV_OTP, 0xFFu);
     }
 
     /* Watchdog kick */
     if ((now_ms - sys->last_wdt_kick_ms) >= sys->wdt_kick_interval_ms) {
         uiox_pmic_hw_wdt_kick(sys->pif.hw);
         sys->last_wdt_kick_ms = now_ms;
     }
 
     /* Dispatch buffered events to callback */
     uiox_pmic_event_t ev;
     while (!uiox_pmic_event_empty()) {
         if (!uiox_pmic_event_pop(&ev)) break;
         fire(sys, ev.type, ev.rail_id);
     }
 }
 
 void uiox_pmic_subsys_set_cb(uiox_pmic_subsys_t *sys,
                                uiox_pmic_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 