/**
 * @file    uiox_us_subsys.h
 * @brief   UIOX Ultrasonic subsystem — pipeline, zones, multi-sensor.
 *
 * Assembles sensor + interface + DSP into a complete measurement
 * pipeline. Adds:
 *   - Multi-channel round-robin scheduling
 *   - Proximity zone detection (NEAR / CAUTION / CLEAR)
 *   - Hysteresis to prevent zone flicker
 *   - Moving-average distance smoothing
 *   - Per-channel statistics (min, max, mean)
 *
 * @date    2026-05-26
 */
//Layer 4 — Ultrasonic Subsystem
 #ifndef UIOX_US_SUBSYS_H
 #define UIOX_US_SUBSYS_H
 
 #include "uiox_us_if.h"
 #include "uiox_us_sensor.h"
 #include "uiox_us_dsp.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Proximity zones
  * ====================================================================== */
 
 typedef enum {
     UIOX_US_ZONE_UNKNOWN = 0,
     UIOX_US_ZONE_CLEAR,      /**< Object beyond caution range              */
     UIOX_US_ZONE_CAUTION,    /**< Object within caution range              */
     UIOX_US_ZONE_NEAR,       /**< Object within near (danger) range        */
 } uiox_us_zone_t;
 
 /* =========================================================================
  * Zone configuration
  * ====================================================================== */
 
 typedef struct {
     float near_m;        /**< Near zone threshold (metres)                 */
     float caution_m;     /**< Caution zone threshold (metres)              */
     float hysteresis_m;  /**< Hysteresis band (metres)                     */
 } uiox_us_zone_cfg_t;
 
 /* =========================================================================
  * Per-channel pipeline state
  * ====================================================================== */
 
 #define UIOX_US_MAX_CHANNELS    4
 #define UIOX_US_SMOOTH_WIN      8   /**< Moving-average window length       */
 
 typedef struct {
     uiox_us_result_t  last;              /**< Last measurement result       */
     uiox_us_zone_t    zone;              /**< Current proximity zone        */
     uiox_us_zone_t    zone_prev;         /**< Previous zone (hysteresis)    */
 
     /* Moving average */
     float             smooth_buf[UIOX_US_SMOOTH_WIN];
     uint8_t           smooth_idx;
     uint8_t           smooth_count;
     float             smooth_dist_m;
 
     /* Statistics */
     float             stat_min_m;
     float             stat_max_m;
     float             stat_mean_m;
     uint32_t          stat_valid_count;
     uint32_t          stat_invalid_count;
 } uiox_us_chan_state_t;
 
 /* =========================================================================
  * Pipeline state
  * ====================================================================== */
 
 typedef enum {
     UIOX_US_PIPE_STOPPED = 0,
     UIOX_US_PIPE_RUNNING,
 } uiox_us_pipe_state_t;
 
 typedef struct {
     uiox_us_if_t         uif;
     uiox_us_sensor_t     sensor;
     uiox_us_dsp_t        dsp;
     uiox_us_zone_cfg_t   zone_cfg;
     uiox_us_pipe_state_t state;
     uiox_us_chan_state_t chan[UIOX_US_MAX_CHANNELS];
     uint8_t              active_channels;
     uint32_t             meas_id;
     uint32_t             temp_update_interval; /**< Temp update every N measurements */
 } uiox_us_pipeline_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_us_pipeline_build (uiox_us_pipeline_t   *pl,
                                uiox_us_hw_t         *hw,
                                const char           *sensor_name,
                                uiox_us_if_type_t     if_type,
                                uint8_t               num_channels);
 
 int  uiox_us_pipeline_config(uiox_us_pipeline_t         *pl,
                                const uiox_us_pulse_cfg_t  *pulse,
                                const uiox_us_dsp_cfg_t    *dsp_cfg,
                                const uiox_us_zone_cfg_t   *zone_cfg,
                                uint32_t sample_rate_hz,
                                uint32_t echo_window_us,
                                uint32_t temp_update_interval);
 
 int  uiox_us_pipeline_start (uiox_us_pipeline_t *pl);
 void uiox_us_pipeline_stop  (uiox_us_pipeline_t *pl);
 
 /**
  * @brief  Perform one measurement on channel ch.
  * @return Pointer to updated channel state, NULL on error.
  */
 const uiox_us_chan_state_t *uiox_us_pipeline_measure(
     uiox_us_pipeline_t *pl, uint8_t ch);
 
 /** Measure all channels in round-robin order. */
 int uiox_us_pipeline_measure_all(uiox_us_pipeline_t *pl);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_SUBSYS_H */
 