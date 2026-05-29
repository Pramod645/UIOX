/**
 * @file    uiox_us_device.c
 * @brief   UIOX Ultrasonic device API implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_device.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_us_open(uiox_us_device_t           *dev,
                   const uiox_us_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     /* 1. Init HAL */
     int rc = uiox_us_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     /* 2. Build pipeline */
     rc = uiox_us_pipeline_build(&dev->pipeline,
                                  p->hw,
                                  p->sensor_name,
                                  p->if_type,
                                  p->num_channels);
     if (rc < 0) return rc;
 
     /* 3. Configure pipeline */
     rc = uiox_us_pipeline_config(&dev->pipeline,
                                   &p->pulse,
                                   &p->dsp,
                                   &p->zones,
                                   p->sample_rate_hz,
                                   p->echo_window_us,
                                   p->temp_update_interval);
     if (rc < 0) return rc;
 
     dev->open = true;
     return 0;
 }
 
 int uiox_us_start(uiox_us_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_us_pipeline_start(&dev->pipeline);
 }
 
 void uiox_us_stop(uiox_us_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_us_pipeline_stop(&dev->pipeline);
 }
 void uiox_us_close(uiox_us_device_t *dev)
 {
     if (!dev) return;
     uiox_us_stop(dev);
     uiox_us_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 const uiox_us_chan_state_t *uiox_us_measure(uiox_us_device_t *dev,
                                              uint8_t           ch)
 {
     if (!dev || !dev->open) return NULL;
     return uiox_us_pipeline_measure(&dev->pipeline, ch);
 }
 
 int uiox_us_measure_all(uiox_us_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_us_pipeline_measure_all(&dev->pipeline);
 }
 
 const uiox_us_result_t *uiox_us_last_result(const uiox_us_device_t *dev,
                                               uint8_t ch)
 {
     if (!dev || !dev->open) return NULL;
     if (ch >= dev->pipeline.active_channels) return NULL;
     return &dev->pipeline.chan[ch].last;
 }
 
 uiox_us_zone_t uiox_us_zone(const uiox_us_device_t *dev, uint8_t ch)
 {
     if (!dev || !dev->open) return UIOX_US_ZONE_UNKNOWN;
     if (ch >= dev->pipeline.active_channels) return UIOX_US_ZONE_UNKNOWN;
     return dev->pipeline.chan[ch].zone;
 }
 
 const char *uiox_us_zone_name(uiox_us_zone_t zone)
 {
     switch (zone) {
     case UIOX_US_ZONE_CLEAR:   return "CLEAR";
     case UIOX_US_ZONE_CAUTION: return "CAUTION";
     case UIOX_US_ZONE_NEAR:    return "NEAR";
     default:                   return "UNKNOWN";
     }
 }
 
 int uiox_us_set_pulse(uiox_us_device_t          *dev,
                        const uiox_us_pulse_cfg_t *pulse)
 {
     if (!dev || !pulse) return -EINVAL;
     return uiox_us_sensor_config(&dev->pipeline.sensor, pulse);
 }
 
 int uiox_us_update_temp(uiox_us_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_us_sensor_update_temp(&dev->pipeline.sensor, dev->hw);
 }
 
 void uiox_us_get_stats(const uiox_us_device_t *dev, uint8_t ch,
                         float    *min_m,        float    *max_m,
                         float    *mean_m,       uint32_t *valid_count,
                         uint32_t *invalid_count)
 {
     if (!dev || !dev->open || ch >= dev->pipeline.active_channels) return;
     const uiox_us_chan_state_t *cs = &dev->pipeline.chan[ch];
     if (min_m)         *min_m         = cs->stat_min_m;
     if (max_m)         *max_m         = cs->stat_max_m;
     if (mean_m)        *mean_m        = cs->stat_mean_m;
     if (valid_count)   *valid_count   = cs->stat_valid_count;
     if (invalid_count) *invalid_count = cs->stat_invalid_count;
 }
  