/**
 * @file    uiox_therm_sensor.c
 * @brief   UIOX Thermal Sensor abstraction implementation.
 * @date    2026-06-05
 */

 #include "uiox_therm_sensor.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 #include <stdint.h>
 
 /* NTC: Simplified Beta equation */
 int16_t uiox_therm_ntc_convert(uint16_t raw,
                                  const uiox_therm_ntc_cfg_t *cfg)
 {
     if (!cfg || raw == 0) return INT16_MIN;
     uint32_t adc_max = (1u << cfg->adc_bits) - 1u;
     if (raw >= adc_max) return INT16_MIN;
 
     /* Voltage divider: R_ntc = R_series × raw / (adc_max - raw) */
     float r_ntc = cfg->r_series * (float)raw / (float)(adc_max - raw);
     if (r_ntc <= 0.0f) return INT16_MIN;
 
     /* Beta equation: 1/T = 1/T_nom + (1/Beta) × ln(R/R_nom) */
     float t_nom_k = cfg->t_nominal + 273.15f;
     float inv_t   = (1.0f / t_nom_k) +
                     (1.0f / cfg->beta) * logf(r_ntc / cfg->r_nominal);
     if (inv_t <= 0.0f) return INT16_MIN;
 
     float t_celsius = (1.0f / inv_t) - 273.15f;
     return (int16_t)(t_celsius * 10.0f);
 }
 
 int uiox_therm_sensor_init(uiox_therm_sensor_mgr_t *mgr,
                              uiox_therm_if_t *tif)
 {
     if (!mgr || !tif) return -EINVAL;
     memset(mgr, 0, sizeof(*mgr));
     mgr->tif = tif;
     return 0;
 }
 
 int uiox_therm_sensor_register(uiox_therm_sensor_mgr_t *mgr,
                                 const uiox_therm_sensor_t *s)
 {
     if (!mgr || !s) return -EINVAL;
     if (mgr->num_sensors >= UIOX_THERM_MAX_SENSORS) return -ENOSPC;
     memcpy(&mgr->sensors[mgr->num_sensors++], s, sizeof(*s));
     return 0;
 }
 
 uiox_therm_sensor_t *uiox_therm_sensor_find(
     uiox_therm_sensor_mgr_t *mgr, const char *name)
 {
     if (!mgr || !name) return NULL;
     for (uint8_t i = 0; i < mgr->num_sensors; i++)
         if (strncmp(mgr->sensors[i].name, name,
                     UIOX_THERM_SENSOR_NAME_MAX) == 0)
             return &mgr->sensors[i];
     return NULL;
 }
 
 int uiox_therm_sensor_update(uiox_therm_sensor_mgr_t *mgr,
                                uint32_t now_ms)
 {
     if (!mgr) return -EINVAL;
     (void)now_ms;
     int rc = 0;
 
     for (uint8_t i = 0; i < mgr->num_sensors; i++) {
         uiox_therm_sensor_t *s = &mgr->sensors[i];
         if (!s->enabled) continue;
 
         int16_t raw_temp = 0;
 
         if (s->type == UIOX_THERM_TYPE_NTC) {
             /* NTC: read ADC then convert */
             const uiox_therm_hw_ops_t *ops =
                 (const uiox_therm_hw_ops_t *)mgr->tif->hw->priv;
             uint16_t adc_raw = 0;
             if (ops && ops->adc_read) {
                 int r = ops->adc_read(mgr->tif->hw, s->hw_channel, &adc_raw);
                 if (r < 0) { s->error = true; rc = r; continue; }
                 raw_temp = uiox_therm_ntc_convert(adc_raw,
                                                    &mgr->tif->hw->ntc);
                 if (raw_temp == INT16_MIN) { s->error = true; continue; }
             }
         } else {
             /* Digital sensor */
             int r = uiox_therm_hw_read_temp(mgr->tif->hw,
                                              s->hw_channel, &raw_temp);
             if (r < 0) { s->error = true; rc = r; continue; }
         }
 
         s->error = false;
 
         /* Apply calibration offset */
         raw_temp = (int16_t)(raw_temp + s->offset_dc);
 
         /* Clamp to valid range */
         if (raw_temp < s->min_dc) raw_temp = s->min_dc;
         if (raw_temp > s->max_dc) raw_temp = s->max_dc;
 
         s->cur_dc = raw_temp;
 
         /* Running average */
         if (s->avg_samples > 1) {
             s->avg_acc += raw_temp;
             s->avg_count++;
             if (s->avg_count >= s->avg_samples) {
                 s->avg_dc    = (int16_t)(s->avg_acc / s->avg_samples);
                 s->avg_acc   = 0;
                 s->avg_count = 0;
             }
         } else {
             s->avg_dc = raw_temp;
         }
     }
     return rc;
 }
 
 int16_t uiox_therm_sensor_get(const uiox_therm_sensor_mgr_t *mgr,
                                const char *name)
 {
     if (!mgr || !name) return INT16_MIN;
     for (uint8_t i = 0; i < mgr->num_sensors; i++) {
         if (strncmp(mgr->sensors[i].name, name,
                     UIOX_THERM_SENSOR_NAME_MAX) == 0)
             return mgr->sensors[i].avg_dc;
     }
     return INT16_MIN;
 }
 