/**
 * @file  uiox_als_if.c
 * @brief UIOX ALS interface driver — I²C, IRQ dispatch, integration guard.
 * @date  2026-06-11
 */

 #include "uiox_als_if.h"
 #include "uiox_klibc.h"

 
 int uiox_als_if_config(uiox_als_if_t *aif, uiox_als_hw_t *hw,
                         bool continuous)
 {
     if (!aif || !hw) return -EINVAL;
     memset(aif, 0, sizeof(*aif));
     aif->hw          = hw;
     aif->primed      = true;
     aif->continuous  = continuous;
     uiox_als_buf_init();
     return 0;
 }
 
 int uiox_als_if_start(uiox_als_if_t *aif)
 {
     if (!aif || !aif->primed) return -EINVAL;
     int rc = uiox_als_hw_power_on(aif->hw);
     if (rc < 0) return rc;
     /* Enable threshold interrupt */
     rc = uiox_als_hw_int_enable(aif->hw, true);
     if (rc < 0) return rc;
     /* Set default integration-time window */
     aif->next_sample_ms = 0u;
     return 0;
 }
 
 void uiox_als_if_stop(uiox_als_if_t *aif)
 {
     if (!aif) return;
     uiox_als_hw_int_enable(aif->hw, false);
     uiox_als_hw_power_off(aif->hw);
 }
 
 int uiox_als_if_trigger(uiox_als_if_t *aif)
 {
     if (!aif || aif->continuous) return -EINVAL;
     return uiox_als_hw_trigger(aif->hw);
 }
 
 int uiox_als_if_fetch(uiox_als_if_t *aif, uint32_t now_ms,
                        uint16_t *als, uint16_t *white, uint16_t *ir)
 {
     if (!aif || !als || !white || !ir) return -EINVAL;
 
     /* Guard: do not read before integration completes */
     if (now_ms < aif->next_sample_ms) return 0;
 
     int rc = uiox_als_hw_read_als(aif->hw, als, white);
     if (rc < 0) { aif->stats.errors++; return rc; }
 
     /* IR channel optional */
     if (aif->hw->caps & UIOX_ALS_CAP_IR_CH) {
         rc = uiox_als_hw_read_ir(aif->hw, ir);
         if (rc < 0) *ir = 0u;
     } else {
         *ir = 0u;
     }
 
     /* Schedule next sample window */
     aif->next_sample_ms = now_ms +
         UIOX_ALS_ITIME_MS[aif->hw->itime] + 5u; /* 5 ms margin */
 
     aif->stats.samples_read++;
     return 1;  /* data available */
 }
 
 int uiox_als_if_set_gain(uiox_als_if_t *aif, uiox_als_gain_t g)
 {
     if (!aif) return -EINVAL;
     int rc = uiox_als_hw_set_gain(aif->hw, g);
     if (rc == 0) aif->stats.gain_changes++;
     return rc;
 }
 
 int uiox_als_if_set_itime(uiox_als_if_t *aif, uiox_als_itime_t t)
 {
     if (!aif) return -EINVAL;
     return uiox_als_hw_set_itime(aif->hw, t);
 }
 
 int uiox_als_if_set_threshold(uiox_als_if_t *aif,
                                uint16_t low, uint16_t high)
 {
     if (!aif) return -EINVAL;
     return uiox_als_hw_set_threshold(aif->hw, low, high);
 }
 
 uiox_als_evt_t *uiox_als_if_irq_handle(uiox_als_if_t *aif,
                                          uint32_t now_ms)
 {
     if (!aif) return NULL;
     uint32_t irq = aif->hw->pending_irq;
     if (!irq) return NULL;
 
     aif->hw->pending_irq = 0u;
     aif->stats.irq_count++;
 
     uiox_als_evt_t *e = uiox_als_evt_alloc();
     if (!e) { aif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->gain         = aif->hw->gain;
     e->itime        = aif->hw->itime;
 
     if (irq & UIOX_ALS_IRQ_THRESH_HIGH) {
         e->type = UIOX_ALS_EVT_THRESH_HIGH;
         aif->stats.irq_thresh_high++;
         uiox_als_hw_int_clear(aif->hw);
     } else if (irq & UIOX_ALS_IRQ_THRESH_LOW) {
         e->type = UIOX_ALS_EVT_THRESH_LOW;
         aif->stats.irq_thresh_low++;
         uiox_als_hw_int_clear(aif->hw);
     } else if (irq & UIOX_ALS_IRQ_DATA_READY) {
         e->type = UIOX_ALS_EVT_DATA_READY;
         aif->stats.irq_data_ready++;
     } else {
         e->type = UIOX_ALS_EVT_NONE;
     }
     return e;
 }
 
 void uiox_als_if_stats_get(const uiox_als_if_t *aif,
                              uiox_als_if_stats_t *out)
 { if (!aif || !out) return; memcpy(out, &aif->stats, sizeof(*out)); }
 