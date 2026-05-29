/**
 * @file    uiox_us_device.h
 * @brief   UIOX Ultrasonic top-level application-facing device API.
 *
 * Single include for application code. Wraps the entire ultrasonic
 * sensor stack from HAL through to zone-classified distance results.
 *
 * @date    2026-05-26
 */
//Layer 5 — Device API
 #ifndef UIOX_US_DEVICE_H
 #define UIOX_US_DEVICE_H
 
 #include "uiox_us_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Open parameters
  * ====================================================================== */
 
 typedef struct {
     /* HAL */
     uiox_us_hw_t             *hw;
     const uiox_us_hw_ops_t   *hw_ops;
 
     /* Sensor identity */
     const char               *sensor_name;
     uiox_us_if_type_t         if_type;
     uint8_t                   num_channels;
 
     /* Pulse config */
     uiox_us_pulse_cfg_t       pulse;
 
     /* DSP config */
     uiox_us_dsp_cfg_t         dsp;
     uint32_t                  sample_rate_hz;
     uint32_t                  echo_window_us;
 
     /* Zone config */
     uiox_us_zone_cfg_t        zones;
 
     /* Periodic temperature update interval (measurements) */
     uint32_t                  temp_update_interval;
 } uiox_us_open_params_t;
 
 /* =========================================================================
  * Device handle
  * ====================================================================== */
 
 typedef struct {
     uiox_us_pipeline_t  pipeline;
     uiox_us_hw_t       *hw;
     bool                open;
 } uiox_us_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 /** Open and fully initialise an ultrasonic device. */
 int  uiox_us_open    (uiox_us_device_t          *dev,
                        const uiox_us_open_params_t *p);
 
 /** Start measurement pipeline. */
 int  uiox_us_start   (uiox_us_device_t *dev);
 
 /** Stop measurement pipeline. */
 void uiox_us_stop    (uiox_us_device_t *dev);
 
 /** Close device and release all resources. */
 void uiox_us_close   (uiox_us_device_t *dev);
 
 /**
  * @brief  Perform one measurement on channel ch.
  * @return Channel state (distance, zone, stats), or NULL on error.
  */
 const uiox_us_chan_state_t *uiox_us_measure(uiox_us_device_t *dev,
                                              uint8_t           ch);
 
 /** Measure all channels in round-robin order. */
 int  uiox_us_measure_all(uiox_us_device_t *dev);
 
 /**
  * @brief  Get last result for channel ch without triggering a new measurement.
  */
 const uiox_us_result_t *uiox_us_last_result(const uiox_us_device_t *dev,
                                               uint8_t ch);
 
 /**
  * @brief  Get current proximity zone for channel ch.
  */
 uiox_us_zone_t uiox_us_zone(const uiox_us_device_t *dev, uint8_t ch);
 
 /** Return zone as a human-readable string. */
 const char *uiox_us_zone_name(uiox_us_zone_t zone);
 
 /** Update pulse configuration mid-operation. */
 int  uiox_us_set_pulse(uiox_us_device_t          *dev,
                         const uiox_us_pulse_cfg_t *pulse);
 
 /** Force an immediate temperature read and SoS update. */
 int  uiox_us_update_temp(uiox_us_device_t *dev);
 
 /** Get channel statistics snapshot. */
 void uiox_us_get_stats(const uiox_us_device_t *dev, uint8_t ch,
                         float *min_m, float *max_m, float *mean_m,
                         uint32_t *valid_count, uint32_t *invalid_count);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_DEVICE_H */
 