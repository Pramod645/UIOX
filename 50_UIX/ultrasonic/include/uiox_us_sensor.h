/**
 * @file    uiox_us_sensor.h
 * @brief   UIOX Ultrasonic sensor abstraction.
 *
 * Abstracts sensor-specific parameters and register programming for:
 *   - HC-SR04  / JSN-SR04T  (GPIO, 40 kHz)
 *   - MB1013   / MB1040     (UART / analog, 42 kHz)
 *   - TDC1000  (SPI,  configurable frequency)
 *   - US-100   (UART / GPIO, 40 kHz with temperature)
 *
 * @date    2026-05-26
 */
//Layer 2b — Sensor Abstraction
 #ifndef UIOX_US_SENSOR_H
 #define UIOX_US_SENSOR_H
 
 #include "uiox_us_hw.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Sensor pulse configuration
  * ====================================================================== */
 
 typedef struct {
     uint32_t  freq_hz;          /**< Transducer frequency (e.g. 40000)      */
     uint32_t  num_pulses;       /**< Number of TX pulses per burst          */
     uint32_t  pulse_width_us;   /**< Trigger pulse width (µs)              */
     uint32_t  blanking_us;      /**< Post-trigger blanking window (µs)     */
     float     max_range_m;      /**< Maximum rated range (metres)           */
     float     min_range_m;      /**< Minimum rated range (metres)           */
     float     beam_angle_deg;   /**< Beam angle half-width (degrees)       */
     uint32_t  period_ms;        /**< Measurement period (ms)               */
     uint32_t  timeout_ms;       /**< Echo timeout (ms)                     */
 } uiox_us_pulse_cfg_t;
 
 /* =========================================================================
  * Temperature compensation
  * ====================================================================== */
 
 typedef struct {
     bool     enabled;           /**< Enable temperature compensation       */
     int32_t  temp_mc;           /**< Current temperature (milli-Celsius)   */
     float    speed_of_sound_mps;/**< Computed SoS at current temperature   */
 } uiox_us_temp_comp_t;
 
 /* =========================================================================
  * Sensor descriptor
  * ====================================================================== */
 
 typedef struct {
     const char           *name;        /**< "HC-SR04", "MB1013", etc.      */
     uiox_us_if_type_t     if_type;     /**< Interface type                 */
     uiox_us_pulse_cfg_t   pulse;       /**< Pulse configuration            */
     uiox_us_temp_comp_t   temp;        /**< Temperature compensation state */
     uint8_t               num_channels;/**< Channels on this sensor        */
     bool                  detected;    /**< Sensor presence confirmed       */
 } uiox_us_sensor_t;
 
 /* =========================================================================
  * Sensor API
  * ====================================================================== */
 
 /** Probe sensor: verify it responds within timeout. */
 int  uiox_us_sensor_detect(uiox_us_sensor_t *s, uiox_us_hw_t *hw);
 
 /** Configure pulse parameters. */
 int  uiox_us_sensor_config(uiox_us_sensor_t          *s,
                             const uiox_us_pulse_cfg_t *cfg);
 
 /**
  * @brief  Update temperature reading and recompute speed of sound.
  *
  * Uses Laplace approximation:
  *   SoS (m/s) = 331.3 × sqrt(1 + T_celsius / 273.15)
  */
 int  uiox_us_sensor_update_temp(uiox_us_sensor_t *s, uiox_us_hw_t *hw);
 
 /**
  * @brief  Convert echo ticks to distance (metres).
  *
  * @param  s       Sensor (for speed of sound + timer frequency).
  * @param  hw      HAL (for timer_freq_hz).
  * @param  ticks   Echo pulse width in timer ticks.
  * @return Distance in metres, or negative on error.
  */
 float uiox_us_sensor_ticks_to_m(const uiox_us_sensor_t *s,
                                   const uiox_us_hw_t     *hw,
                                   int64_t                 ticks);
 
 /** Return current speed of sound (m/s). */
 float uiox_us_sensor_sos(const uiox_us_sensor_t *s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_SENSOR_H */
 