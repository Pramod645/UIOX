/**
 * @file    uiox_us_subsys.c
 * @brief   UIOX Ultrasonic subsystem implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Moving average update
  * ====================================================================== */
 
 static void smooth_update(uiox_us_chan_state_t *cs, float dist_m)
 {
     cs->smooth_buf[cs->smooth_idx % UIOX_US_SMOOTH_WIN] = dist_m;
     cs->smooth_idx++;
     if (cs->smooth_count < UIOX_US_SMOOTH_WIN) cs->smooth_count++;
 
     float sum = 0.0f;
     for (uint8_t i = 0; i < cs->smooth_count; i++)
         sum += cs->smooth_buf[i];
     cs->smooth_dist_m = sum / (float)cs->smooth_count;
 }
 
 /* =========================================================================
  * Statistics update
  * ====================================================================== */
 
 static void stats_update(uiox_us_chan_state_t *cs, float dist_m, bool valid)
 {
     if (!valid) { cs->stat_invalid_count++; return; }
     cs->stat_valid_count++;
     if (dist_m < cs->stat_min_m || cs->stat_valid_count == 1)
         cs->stat_min_m = dist_m;
     if (dist_m > cs->stat_max_m)
         cs->stat_max_m = dist_m;
     /* Running mean */
     cs->stat_mean_m +=
         (dist_m - cs->stat_mean_m) / (float)cs->stat_valid_count;
 }
 
 /* =========================================================================
  * Zone classification with hysteresis
  * ====================================================================== */
 
 static uiox_us_zone_t classify_zone(const uiox_us_zone_cfg_t *cfg,
                                      uiox_us_zone_t            prev,
                                      float                     dist_m)
 {
     float hys = cfg->hysteresis_m;
 
     switch (prev) {
     case UIOX_US_ZONE_NEAR:
         if (dist_m > cfg->near_m    + hys) return UIOX_US_ZONE_CAUTION;
         return UIOX_US_ZONE_NEAR;
     case UIOX_US_ZONE_CAUTION:
         if (dist_m < cfg->near_m    - hys) return UIOX_US_ZONE_NEAR;
         if (dist_m > cfg->caution_m + hys) return UIOX_US_ZONE_CLEAR;
         return UIOX_US_ZONE_CAUTION;
     case UIOX_US_ZONE_CLEAR:
         if (dist_m < cfg->caution_m - hys) return UIOX_US_ZONE_CAUTION;
         return UIOX_US_ZONE_CLEAR;
     default:
         /* First classification — no hysteresis */
         if (dist_m <= cfg->near_m)    return UIOX_US_ZONE_NEAR;
         if (dist_m <= cfg->caution_m) return UIOX_US_ZONE_CAUTION;
         return UIOX_US_ZONE_CLEAR;
     }
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 int uiox_us_pipeline_build(uiox_us_pipeline_t   *pl,
                             uiox_us_hw_t         *hw,
                             const char           *sensor_name,
                             uiox_us_if_type_t     if_type,
                             uint8_t               num_channels)
 {
     if (!pl || !hw || num_channels == 0 ||
         num_channels > UIOX_US_MAX_CHANNELS) return -EINVAL;
 
     memset(pl, 0, sizeof(*pl));
     pl->uif.hw              = hw;
     pl->sensor.name         = sensor_name;
     pl->sensor.if_type      = if_type;
     pl->sensor.num_channels = num_channels;
     pl->active_channels     = num_channels;
 
     /* Default SoS until temperature is read */
     pl->sensor.temp.speed_of_sound_mps = 343.2f;
 
     /* Try to detect sensor */
     uiox_us_sensor_detect(&pl->sensor, hw);
 
     return 0;
 }
 
 int uiox_us_pipeline_config(uiox_us_pipeline_t        *pl,
                               const uiox_us_pulse_cfg_t *pulse,
                               const uiox_us_dsp_cfg_t   *dsp_cfg,
                               const uiox_us_zone_cfg_t  *zone_cfg,
                               uint32_t sample_rate_hz,
                               uint32_t echo_window_us,
                               uint32_t temp_update_interval)
 {
     if (!pl || !pulse || !dsp_cfg || !zone_cfg) return -EINVAL;
 
     /* Sensor pulse config */
     int rc = uiox_us_sensor_config(&pl->sensor, pulse);
     if (rc < 0) return rc;
 
     /* Interface config */
     rc = uiox_us_if_config(&pl->uif,
                             pl->uif.hw,
                             pl->sensor.if_type,
                             pl->active_channels,
                             pulse->pulse_width_us,
                             pulse->period_ms,
                             pulse->timeout_ms,
                             sample_rate_hz,
                             echo_window_us);
     if (rc < 0) return rc;
 
     /* DSP context init */
     rc = uiox_us_dsp_init(&pl->dsp, dsp_cfg, sample_rate_hz);
     if (rc < 0) return rc;
 
     /* Zone config */
     memcpy(&pl->zone_cfg, zone_cfg, sizeof(*zone_cfg));
     pl->temp_update_interval = temp_update_interval;
 
     /* Reset channel states */
     memset(pl->chan, 0, sizeof(pl->chan));
     for (int i = 0; i < UIOX_US_MAX_CHANNELS; i++)
         pl->chan[i].zone = UIOX_US_ZONE_UNKNOWN;
 
     return 0;
 }
 
 int uiox_us_pipeline_start(uiox_us_pipeline_t *pl)
 {
     if (!pl) return -EINVAL;
     /* Initial temperature read */
     uiox_us_sensor_update_temp(&pl->sensor, pl->uif.hw);
     pl->state = UIOX_US_PIPE_RUNNING;
     return 0;
 }
 
 void uiox_us_pipeline_stop(uiox_us_pipeline_t *pl)
 {
     if (!pl) return;
     pl->state = UIOX_US_PIPE_STOPPED;
 }
 
 const uiox_us_chan_state_t *uiox_us_pipeline_measure(
     uiox_us_pipeline_t *pl, uint8_t ch)
 {
     if (!pl || ch >= pl->active_channels) return NULL;
     if (pl->state != UIOX_US_PIPE_RUNNING) return NULL;
 
     /* Periodic temperature update */
     if (pl->temp_update_interval > 0 &&
         (pl->meas_id % pl->temp_update_interval) == 0)
         uiox_us_sensor_update_temp(&pl->sensor, pl->uif.hw);
 
     uiox_us_chan_state_t *cs = &pl->chan[ch];
     uiox_us_result_t      result;
     uiox_us_frame_t      *raw_frame = NULL;
 
     int64_t ticks = uiox_us_if_measure(&pl->uif, ch, &raw_frame);
 
     int rc;
     if (raw_frame) {
         /* ADC mode */
         raw_frame->meas_id = pl->meas_id;
         rc = uiox_us_dsp_process_raw(&pl->dsp, raw_frame,
                                       &pl->sensor, &result);
         uiox_us_buf_free(raw_frame);
     } else {
         /* GPIO / UART mode */
         rc = uiox_us_dsp_process_ticks(&pl->dsp, ticks,
                                         &pl->sensor, pl->uif.hw,
                                         &result);
     }
 
     result.channel = ch;
     result.meas_id = pl->meas_id++;
 
     if (rc == 0 && result.valid) {
         smooth_update(cs, result.distance_m);
         result.distance_m = cs->smooth_dist_m;
         cs->zone_prev = cs->zone;
         cs->zone = classify_zone(&pl->zone_cfg, cs->zone_prev,
                                   cs->smooth_dist_m);
     }
 
     stats_update(cs, result.distance_m, result.valid);
     memcpy(&cs->last, &result, sizeof(result));
     return cs;
 }
 
 int uiox_us_pipeline_measure_all(uiox_us_pipeline_t *pl)
 {
     if (!pl) return -EINVAL;
     int ok = 0;
     for (uint8_t ch = 0; ch < pl->active_channels; ch++)
         if (uiox_us_pipeline_measure(pl, ch)) ok++;
     return ok;
 }
 