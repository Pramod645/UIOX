/**
 * @file    uiox_us_sensor.c
 * @brief   UIOX Ultrasonic sensor abstraction implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_sensor.h"
 #include "uiox_us_if.h"
 #include <math.h>
 #include <string.h>
 #include <errno.h>
 
 /* Default speed of sound at 20 °C */
 #define UIOX_US_SOS_DEFAULT_MPS   343.2f
 #define UIOX_US_SOS_BASE_MPS      331.3f
 
 /* -------------------------------------------------------------------------
  * Temperature → SoS
  * SoS = 331.3 × √(1 + T_C / 273.15)
  * ---------------------------------------------------------------------- */
 
 static float sos_from_temp(int32_t temp_mc)
 {
     float t_c = (float)temp_mc / 1000.0f;
     return UIOX_US_SOS_BASE_MPS * sqrtf(1.0f + t_c / 273.15f);
 }
 
 /* -------------------------------------------------------------------------
  * Public API
  * ---------------------------------------------------------------------- */
 
 int uiox_us_sensor_detect(uiox_us_sensor_t *s, uiox_us_hw_t *hw)
 {
     if (!s || !hw) return -EINVAL;
 
     /* For GPIO sensors, detection is implicit — just probe a single
      * trigger/echo cycle and check it completes without timeout.      */
     uiox_us_trig_cfg_t probe = {
         .pulse_width_us = s->pulse.pulse_width_us,
         .period_ms      = s->pulse.period_ms,
         .timeout_ms     = 100   /* short probe timeout */
     };
 
     int rc = uiox_us_hw_trigger(hw, 0, &probe);
     if (rc < 0) { s->detected = false; return rc; }
 
     int64_t ticks = uiox_us_hw_echo_wait(hw, 0, 100u);
     s->detected = (ticks > 0);
     return s->detected ? 0 : -ENODEV;
 }
 
 int uiox_us_sensor_config(uiox_us_sensor_t          *s,
                             const uiox_us_pulse_cfg_t *cfg)
 {
     if (!s || !cfg) return -EINVAL;
     memcpy(&s->pulse, cfg, sizeof(*cfg));
 
     /* Set default SoS */
     if (!s->temp.enabled)
         s->temp.speed_of_sound_mps = UIOX_US_SOS_DEFAULT_MPS;
 
     return 0;
 }
 
 int uiox_us_sensor_update_temp(uiox_us_sensor_t *s, uiox_us_hw_t *hw)
 {
     if (!s || !hw) return -EINVAL;
 
     int32_t temp_mc = 20000; /* default 20 °C */
     int rc = uiox_us_hw_read_temp(hw, &temp_mc);
     if (rc < 0) {
         /* No temperature sensor — keep current value */
         return 0;
     }
 
     s->temp.temp_mc           = temp_mc;
     s->temp.speed_of_sound_mps = sos_from_temp(temp_mc);
     s->temp.enabled           = true;
     return 0;
 }
 
 float uiox_us_sensor_ticks_to_m(const uiox_us_sensor_t *s,
                                   const uiox_us_hw_t     *hw,
                                   int64_t                 ticks)
 {
     if (!s || !hw || ticks <= 0) return -1.0f;
 
     float sos    = s->temp.speed_of_sound_mps;
     if (sos < 200.0f || sos > 400.0f)
         sos = UIOX_US_SOS_DEFAULT_MPS;
 
     /* distance = (ticks / timer_freq) × SoS / 2 */
     float time_s = (float)ticks / (float)hw->timer_freq_hz;
     float dist_m = (time_s * sos) / 2.0f;
 
     /* Clamp to sensor rated range */
     if (dist_m < s->pulse.min_range_m) return -1.0f; /* blanking zone */
     if (dist_m > s->pulse.max_range_m) return -1.0f; /* out of range  */
 
     return dist_m;
 }
 
 float uiox_us_sensor_sos(const uiox_us_sensor_t *s)
 {
     return s ? s->temp.speed_of_sound_mps : UIOX_US_SOS_DEFAULT_MPS;
 }
 