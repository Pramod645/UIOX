/**
 * @file  uiox_chg_subsys.c
 * @brief UIOX Charger Subsystem — safety, thermal, event dispatch.
 * @date  2026-06-11
 */

 #include "uiox_chg_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_chg_subsys_t *sys, uiox_chg_ev_t ev,
                  const uiox_chg_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 /* Map IF event type → subsystem event */
 static uiox_chg_ev_t evt_map(uiox_chg_evt_type_t t)
 {
     switch (t) {
     case UIOX_CHG_EVT_PLUG_IN:          return UIOX_CHG_EV_PLUG_IN;
     case UIOX_CHG_EVT_PLUG_OUT:         return UIOX_CHG_EV_PLUG_OUT;
     case UIOX_CHG_EVT_CHRG_START:       return UIOX_CHG_EV_CHRG_START;
     case UIOX_CHG_EVT_CHRG_DONE:        return UIOX_CHG_EV_CHRG_DONE;
     case UIOX_CHG_EVT_FAULT:            return UIOX_CHG_EV_FAULT;
     case UIOX_CHG_EVT_FAULT_CLEAR:      return UIOX_CHG_EV_FAULT_CLEAR;
     case UIOX_CHG_EVT_PD_CONTRACT:      return UIOX_CHG_EV_PD_CONTRACT;
     case UIOX_CHG_EVT_PD_RESET:         return UIOX_CHG_EV_PD_RESET;
     case UIOX_CHG_EVT_OTG_ON:           return UIOX_CHG_EV_OTG_ON;
     case UIOX_CHG_EVT_OTG_OFF:          return UIOX_CHG_EV_OTG_OFF;
     case UIOX_CHG_EVT_THERMAL_THROTTLE: return UIOX_CHG_EV_THERMAL_THROTTLE;
     default:                             return UIOX_CHG_EV_ERROR;
     }
 }
 
 int uiox_chg_subsys_init(uiox_chg_subsys_t *sys,
                           uiox_chg_hw_t *hw,
                           const uiox_chg_profile_t *profile)
 {
     if (!sys || !hw || !profile) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_chg_if_config(&sys->cif, hw);
     if (rc < 0) return rc;
     rc = uiox_chg_policy_init(&sys->policy, &sys->cif, profile);
     if (rc < 0) return rc;
     sys->state = UIOX_CHG_STATE_OFF;
     return 0;
 }
 
 int uiox_chg_subsys_start(uiox_chg_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_CHG_STATE_INIT;
     int rc = uiox_chg_if_start(&sys->cif);
     if (rc < 0) { sys->state = UIOX_CHG_STATE_ERROR; return rc; }
     sys->state = UIOX_CHG_STATE_READY;
     return 0;
 }
 
 void uiox_chg_subsys_stop(uiox_chg_subsys_t *sys)
 {
     if (!sys) return;
     uiox_chg_if_stop(&sys->cif);
     sys->state = UIOX_CHG_STATE_OFF;
 }
 
 void uiox_chg_subsys_tick(uiox_chg_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_CHG_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* Run policy tick (watchdog + PD timeout) */
     uiox_chg_policy_tick(&sys->policy, now_ms);
 
     /* Poll interface for state changes */
     uiox_chg_evt_t *e = uiox_chg_if_irq_handle(&sys->cif, now_ms);
     if (!e)
         e = uiox_chg_if_poll(&sys->cif, now_ms);
     if (!e) goto thermal;
 
     /* React to event */
     switch (e->type) {
     case UIOX_CHG_EVT_PLUG_IN:
         uiox_chg_policy_on_plug(&sys->policy, e->src);
         fire(sys, UIOX_CHG_EV_PLUG_IN, e);
         break;
     case UIOX_CHG_EVT_PLUG_OUT:
         uiox_chg_policy_on_unplug(&sys->policy);
         fire(sys, UIOX_CHG_EV_PLUG_OUT, e);
         break;
     case UIOX_CHG_EVT_FAULT:
         sys->fault_count++;
         sys->state = UIOX_CHG_STATE_FAULT;
         fire(sys, UIOX_CHG_EV_FAULT, e);
         break;
     case UIOX_CHG_EVT_FAULT_CLEAR:
         sys->state = UIOX_CHG_STATE_READY;
         fire(sys, UIOX_CHG_EV_FAULT_CLEAR, e);
         break;
     case UIOX_CHG_EVT_CHRG_DONE:
         sys->charge_cycles++;
         fire(sys, UIOX_CHG_EV_CHRG_DONE, e);
         break;
     default:
         if (e->type != UIOX_CHG_EVT_NONE)
             fire(sys, evt_map(e->type), e);
         break;
     }
 
     /* Handle PD message if pending */
     if (sys->cif.hw->pending_irq & UIOX_CHG_IRQ_PD_MSG) {
         uint8_t buf[32];
         int n = uiox_chg_if_pd_rx(&sys->cif, buf, sizeof(buf));
         if (n > 0) {
             int rc = uiox_chg_policy_pd_rx(&sys->policy,
                                             buf, (uint8_t)n);
             if (rc == 0 &&
                 sys->policy.pd_state == UIOX_CHG_PD_CONTRACT_OK) {
                 sys->pd_contracts++;
                 e->type = UIOX_CHG_EVT_PD_CONTRACT;
                 fire(sys, UIOX_CHG_EV_PD_CONTRACT, e);
             }
         }
         sys->cif.hw->pending_irq &= ~UIOX_CHG_IRQ_PD_MSG;
     }
 
     uiox_chg_evt_free(e);
 
 thermal:
     /* Thermal throttle: read die temperature */
     {
         int32_t tdie_mc = 0;
         if (uiox_chg_hw_adc_read(sys->cif.hw,
                                    UIOX_CHG_ADC_TDIE,
                                    &tdie_mc) == 0) {
             if (!sys->throttled &&
                 tdie_mc >= (int32_t)UIOX_CHG_TDIE_THROTTLE_MC) {
                 sys->throttled = true;
                 /* Halve charge current */
                 uint32_t throttled_ma =
                     sys->policy.profile.fast_charge_ma / 2u;
                 uiox_chg_if_set_ichg(&sys->cif, throttled_ma);
                 fire(sys, UIOX_CHG_EV_THERMAL_THROTTLE, NULL);
             } else if (sys->throttled &&
                        tdie_mc < (int32_t)UIOX_CHG_TDIE_RESUME_MC) {
                 sys->throttled = false;
                 uiox_chg_if_set_ichg(&sys->cif,
                     sys->policy.profile.fast_charge_ma);
                 fire(sys, UIOX_CHG_EV_THERMAL_RESUME, NULL);
             }
         }
     }
 }
 
 void uiox_chg_subsys_set_cb(uiox_chg_subsys_t *sys,
                               uiox_chg_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_chg_subsys_enable(uiox_chg_subsys_t *sys, bool en)
 {
     if (!sys) return -EINVAL;
     return uiox_chg_if_charge_en(&sys->cif, en);
 }
 
 int uiox_chg_subsys_otg(uiox_chg_subsys_t *sys, bool en)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_chg_if_otg_en(&sys->cif, en);
     if (rc == 0) {
         sys->otg_active = en;
         fire(sys, en ? UIOX_CHG_EV_OTG_ON : UIOX_CHG_EV_OTG_OFF, NULL);
     }
     return rc;
 }
 
 int uiox_chg_subsys_set_profile(uiox_chg_subsys_t *sys,
                                   const uiox_chg_profile_t *p)
 {
     if (!sys || !p) return -EINVAL;
     return uiox_chg_policy_set_profile(&sys->policy, p);
 }
 