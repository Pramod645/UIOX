/**
 * @file    uiox_therm_if.c
 * @brief   UIOX Thermal Sensor interface driver implementation.
 * @date    2026-06-05
 */

 #include "uiox_therm_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_therm_if_config(uiox_therm_if_t *tif, uiox_therm_hw_t *hw)
 {
     if (!tif || !hw) return -EINVAL;
     memset(tif, 0, sizeof(*tif));
     tif->hw     = hw;
     tif->primed = true;
     uiox_therm_buf_init();
     return 0;
 }
 
 int uiox_therm_if_start(uiox_therm_if_t *tif)
 {
     if (!tif || !tif->primed) return -EINVAL;
 
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->priv;
 
     /* Set alert thresholds in hardware */
     if (tif->hw->t_high_dc)
         uiox_therm_hw_set_t_high(tif->hw, tif->hw->t_high_dc);
     if (tif->hw->t_hyst_dc)
         uiox_therm_hw_set_t_hyst(tif->hw, tif->hw->t_hyst_dc);
     if (tif->hw->t_crit_dc)
         uiox_therm_hw_set_t_crit(tif->hw, tif->hw->t_crit_dc);
 
     /* Configure alert as interrupt mode, fault queue = 2 */
     if (ops && ops->reg_write) {
         uint8_t conf = (uint8_t)(UIOX_THERM_CONF_CMP_INT | (1u << 3));
         ops->reg_write(tif->hw, UIOX_REG_THERM_CONF, conf);
     }
     return 0;
 }
 
 void uiox_therm_if_stop(uiox_therm_if_t *tif)
 {
     if (!tif) return;
     /* Put sensor into shutdown mode to save power */
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->priv;
     if (ops && ops->set_mode) ops->set_mode(tif->hw, true);
 }
 
 int uiox_therm_if_measure(uiox_therm_if_t *tif, uint32_t now_ms)
 {
     if (!tif || !tif->primed) return -EINVAL;
     tif->stats.measurements++;
     int rc = 0;
 
     for (uint8_t ch = 0; ch < tif->hw->num_channels; ch++) {
         int16_t temp = 0;
         int r = uiox_therm_hw_read_temp(tif->hw, ch, &temp);
         if (r < 0) {
             tif->stats.comm_errors++;
             tif->stats.error_count++;
             uiox_therm_event_t ev = {
                 .type      = UIOX_THERM_EV_SENSOR_ERROR,
                 .sensor_id = ch,
                 .ts_ms     = now_ms,
                 .valid     = true,
             };
             uiox_therm_event_push(&ev);
             rc = r;
             continue;
         }
 
         /* Check alert threshold */
         if (tif->hw->t_high_dc != 0 && temp >= tif->hw->t_high_dc) {
             if (!tif->hw->meas[ch].alert_active) {
                 tif->hw->meas[ch].alert_active = true;
                 tif->stats.alert_count++;
                 uiox_therm_event_t ev = {
                     .type         = UIOX_THERM_EV_ALERT_HIGH,
                     .sensor_id    = ch,
                     .temp_dc      = temp,
                     .threshold_dc = tif->hw->t_high_dc,
                     .ts_ms        = now_ms,
                     .valid        = true,
                 };
                 uiox_therm_event_push(&ev);
             }
         } else if (tif->hw->meas[ch].alert_active &&
                    temp < tif->hw->t_hyst_dc) {
             tif->hw->meas[ch].alert_active = false;
             uiox_therm_event_t ev = {
                 .type      = UIOX_THERM_EV_ALERT_CLEAR,
                 .sensor_id = ch,
                 .temp_dc   = temp,
                 .ts_ms     = now_ms,
                 .valid     = true,
             };
             uiox_therm_event_push(&ev);
         }
 
         /* Critical threshold */
         if (tif->hw->t_crit_dc != 0 && temp >= tif->hw->t_crit_dc) {
             uiox_therm_event_t ev = {
                 .type         = UIOX_THERM_EV_CRITICAL,
                 .sensor_id    = ch,
                 .temp_dc      = temp,
                 .threshold_dc = tif->hw->t_crit_dc,
                 .ts_ms        = now_ms,
                 .valid        = true,
             };
             uiox_therm_event_push(&ev);
         }
     }
     return rc;
 }
 
 int uiox_therm_if_irq_handle(uiox_therm_if_t *tif, uint32_t now_ms)
 {
     if (!tif) return -EINVAL;
     tif->stats.irq_count++;
     tif->stats.alert_count++;
 
     /* Read alert status */
     const uiox_therm_hw_ops_t *ops =
         (const uiox_therm_hw_ops_t *)tif->hw->priv;
     bool alert = false;
     if (ops && ops->alert_status) ops->alert_status(tif->hw, &alert);
 
     if (alert) {
         /* Measure all channels to find which is above threshold */
         uiox_therm_if_measure(tif, now_ms);
     }
     uiox_therm_hw_alert_clear(tif->hw);
     return alert ? 1 : 0;
 }
 
 int uiox_therm_if_telemetry(uiox_therm_if_t *tif,
                              uiox_therm_telem_t *out, uint32_t now_ms)
 {
     if (!tif || !out) return -EINVAL;
     out->ts_ms       = now_ms;
     out->num_channels= tif->hw->num_channels;
     for (uint8_t i = 0; i < tif->hw->num_channels; i++) {
         out->temp_dc[i] = tif->hw->meas[i].temp_dc;
         out->alert[i]   = tif->hw->meas[i].alert_active;
     }
     return 0;
 }
 
 int uiox_therm_if_set_alert(uiox_therm_if_t *tif,
                              int16_t t_high_dc, int16_t t_hyst_dc)
 {
     if (!tif) return -EINVAL;
     int rc = uiox_therm_hw_set_t_high(tif->hw, t_high_dc);
     if (rc == 0) rc = uiox_therm_hw_set_t_hyst(tif->hw, t_hyst_dc);
     return rc;
 }
 
 void uiox_therm_if_stats_get(const uiox_therm_if_t *tif,
                               uiox_therm_if_stats_t *out)
 { if (!tif || !out) return; memcpy(out, &tif->stats, sizeof(*out)); }
 
 void uiox_therm_if_stats_reset(uiox_therm_if_t *tif)
 { if (!tif) return; memset(&tif->stats, 0, sizeof(tif->stats)); }
 