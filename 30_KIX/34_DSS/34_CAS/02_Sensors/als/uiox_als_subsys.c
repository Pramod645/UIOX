/**
 * @file  uiox_als_subsys.c
 * @brief UIOX ALS Subsystem — threshold, auto-gain, scene events.
 * @date  2026-06-11
 */

 #include "uiox_als_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_als_subsys_t *sys, uiox_als_ev_t ev,
                  const uiox_als_sample_t *s)
 { if (sys->evt_cb) sys->evt_cb(ev, s, sys->evt_ctx); }
 
 int uiox_als_subsys_init(uiox_als_subsys_t *sys,
                           uiox_als_hw_t *hw,
                           const uiox_als_coeff_t *coeff)
 {
     if (!sys || !hw || !coeff) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     int rc = uiox_als_if_config(&sys->aif, hw, true);
     if (rc < 0) return rc;
     rc = uiox_als_cal_init(&sys->cal, &sys->aif, coeff);
     if (rc < 0) return rc;
     sys->thresh_high_milli = UIOX_ALS_BRIGHT_THRESH_MILLI;
     sys->thresh_low_milli  = UIOX_ALS_DARK_THRESH_MILLI;
     sys->auto_gain_en      = true;
     sys->scene_dark        = true;   /* assume dark until first sample    */
     sys->state             = UIOX_ALS_STATE_OFF;
     return 0;
 }
 
 int uiox_als_subsys_start(uiox_als_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_ALS_STATE_INIT;
     int rc = uiox_als_if_start(&sys->aif);
     if (rc < 0) { sys->state = UIOX_ALS_STATE_ERROR; return rc; }
     sys->state = UIOX_ALS_STATE_READY;
     return 0;
 }
 
 void uiox_als_subsys_stop(uiox_als_subsys_t *sys)
 {
     if (!sys) return;
     uiox_als_if_stop(&sys->aif);
     sys->state = UIOX_ALS_STATE_OFF;
 }
 
 void uiox_als_subsys_tick(uiox_als_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_ALS_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* Handle pending IRQ events first */
     uiox_als_evt_t *irq_evt = uiox_als_if_irq_handle(&sys->aif, now_ms);
     if (irq_evt) {
         irq_evt->lux_milli = sys->last_sample.lux_milli;
         if (irq_evt->type == UIOX_ALS_EVT_THRESH_HIGH) {
             sys->thresh_high_count++;
             fire(sys, UIOX_ALS_EV_THRESH_HIGH, &sys->last_sample);
         } else if (irq_evt->type == UIOX_ALS_EVT_THRESH_LOW) {
             sys->thresh_low_count++;
             fire(sys, UIOX_ALS_EV_THRESH_LOW, &sys->last_sample);
         }
         uiox_als_evt_free(irq_evt);
     }
 
     /* Fetch new measurement */
     uint16_t raw_als = 0u, raw_white = 0u, raw_ir = 0u;
     int ready = uiox_als_if_fetch(&sys->aif, now_ms,
                                    &raw_als, &raw_white, &raw_ir);
     if (ready <= 0) return;
 
     /* Compute lux and CCT */
     uint32_t lux_milli = uiox_als_cal_to_lux(&sys->cal, raw_als,
                                                sys->aif.hw->gain,
                                                sys->aif.hw->itime);
     uint32_t cct_k     = uiox_als_cal_to_cct(&sys->cal,
                                                raw_als, raw_ir);
 
     /* Build sample record */
     uiox_als_sample_t *s = uiox_als_sample_alloc();
     if (!s) return;
 
     s->timestamp_ms = now_ms;
     s->raw_als      = raw_als;
     s->raw_white    = raw_white;
     s->raw_ir       = raw_ir;
     s->lux_milli    = lux_milli;
     s->cct_k        = cct_k;
     s->gain         = sys->aif.hw->gain;
     s->itime        = sys->aif.hw->itime;
     s->saturated    = (raw_als >= sys->cal.coeff->ag_saturate) ? 1u : 0u;
 
     /* Copy to last_sample (subsys holds a persistent copy) */
     sys->last_sample = *s;
     sys->sample_count++;
 
     /* Fire DATA_READY */
     fire(sys, UIOX_ALS_EV_DATA_READY, s);
 
     /* Saturation warning */
     if (s->saturated) {
         sys->aif.stats.saturations++;
         fire(sys, UIOX_ALS_EV_SATURATED, s);
     }
 
     /* Scene transition */
     if (sys->scene_dark && lux_milli >= sys->thresh_high_milli) {
         sys->scene_dark = false;
         fire(sys, UIOX_ALS_EV_BRIGHT, s);
     } else if (!sys->scene_dark && lux_milli <= sys->thresh_low_milli) {
         sys->scene_dark = true;
         fire(sys, UIOX_ALS_EV_DARK, s);
     }
 
     /* Lux threshold comparison for HW interrupt programming */
     if ((sys->aif.hw->caps & UIOX_ALS_CAP_THRESHOLD_INT) && lux_milli > 0u) {
         uint32_t scale = sys->cal.coeff->scale_num;
         uint32_t den   = sys->cal.coeff->scale_den;
         if (den > 0u && scale > 0u) {
             uint16_t hw_hi = (uint16_t)(sys->thresh_high_milli
                              / (scale * 1000u / den));
             uint16_t hw_lo = (uint16_t)(sys->thresh_low_milli
                              / (scale * 1000u / den));
             uiox_als_if_set_threshold(&sys->aif, hw_lo, hw_hi);
         }
     }
 
     /* Auto-gain */
     if (sys->auto_gain_en && !s->saturated) {
         uiox_als_gain_t  new_gain  = sys->aif.hw->gain;
         uiox_als_itime_t new_itime = sys->aif.hw->itime;
         bool changed = uiox_als_cal_auto_gain(&sys->cal, raw_als,
                                                &new_gain, &new_itime);
         if (changed) {
             sys->gain_change_count++;
             uiox_als_if_set_gain(&sys->aif, new_gain);
             uiox_als_if_set_itime(&sys->aif, new_itime);
             /* Populate event */
             uiox_als_evt_t *ge = uiox_als_evt_alloc();
             if (ge) {
                 ge->type         = UIOX_ALS_EVT_GAIN_CHANGED;
                 ge->timestamp_ms = now_ms;
                 ge->gain         = new_gain;
                 ge->itime        = new_itime;
                 ge->lux_milli    = lux_milli;
                 fire(sys, UIOX_ALS_EV_GAIN_CHANGED, s);
                 uiox_als_evt_free(ge);
             }
         }
     }
 
     uiox_als_sample_free(s);
 }
 
 void uiox_als_subsys_set_cb(uiox_als_subsys_t *sys,
                               uiox_als_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 
 int uiox_als_subsys_set_thresh(uiox_als_subsys_t *sys,
                                 uint32_t low_milli, uint32_t high_milli)
 {
     if (!sys) return -EINVAL;
     sys->thresh_low_milli  = low_milli;
     sys->thresh_high_milli = high_milli;
     return 0;
 }
 
 void uiox_als_subsys_auto_gain(uiox_als_subsys_t *sys, bool en)
 { if (sys) sys->auto_gain_en = en; }
 
 int uiox_als_subsys_set_gain(uiox_als_subsys_t *sys, uiox_als_gain_t g)
 { if (!sys) return -EINVAL; return uiox_als_if_set_gain(&sys->aif, g); }
 
 int uiox_als_subsys_set_itime(uiox_als_subsys_t *sys, uiox_als_itime_t t)
 { if (!sys) return -EINVAL; return uiox_als_if_set_itime(&sys->aif, t); }
 