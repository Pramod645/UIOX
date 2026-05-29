/**
 * @file    uiox_us_dsp.h
 * @brief   UIOX Ultrasonic DSP layer.
 *
 * Processes raw ADC echo samples through:
 *   1. DC offset removal
 *   2. Bandpass filtering (centred at transducer frequency)
 *   3. Envelope detection (rectify + low-pass)
 *   4. Threshold detection → Time-of-Flight (ToF)
 *   5. Sub-sample peak interpolation for improved resolution
 *   6. Temperature-compensated distance conversion
 *   7. Median filtering across measurements (noise rejection)
 *
 * For GPIO pulse-width mode (no ADC), steps 1-5 are skipped and
 * the pulse width is converted directly to distance.
 *
 * @date    2026-05-26
 */
//Layer 3 — Signal Processing
 #ifndef UIOX_US_DSP_H
 #define UIOX_US_DSP_H
 
 #include "uiox_us_buf.h"
 #include "uiox_us_sensor.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * DSP configuration
  * ====================================================================== */
 
 typedef struct {
     /* Bandpass filter */
     float     bp_center_hz;     /**< Centre frequency (= transducer freq)  */
     float     bp_bandwidth_hz;  /**< Bandpass bandwidth (Hz)               */
 
     /* Envelope */
     float     env_cutoff_hz;    /**< Low-pass cutoff for envelope (Hz)     */
 
     /* Threshold */
     float     threshold_pct;    /**< Detection threshold (% of peak)       */
     uint32_t  blanking_samples; /**< Samples to skip after trigger         */
 
     /* Median filter */
     uint8_t   median_window;    /**< Median filter length (odd, 1..9)      */
 
     /* Peak interpolation */
     bool      do_interpolate;   /**< Enable sub-sample peak interpolation  */
 } uiox_us_dsp_cfg_t;
 
 /* =========================================================================
  * DSP measurement result
  * ====================================================================== */
 
 typedef struct {
     float     distance_m;       /**< Measured distance (metres)            */
     float     tof_us;           /**< Time of flight (microseconds)         */
     float     peak_amplitude;   /**< Peak envelope amplitude (normalised)  */
     float     snr_db;           /**< Estimated signal-to-noise ratio (dB) */
     float     temp_celsius;     /**< Temperature at measurement time       */
     float     sos_mps;          /**< Speed of sound used (m/s)            */
     bool      valid;            /**< true if detection is valid            */
     uint8_t   channel;          /**< Sensor channel                        */
     uint32_t  meas_id;          /**< Measurement ID                        */
     uint64_t  ts_ns;            /**< Timestamp (ns)                        */
 } uiox_us_result_t;
 
 /* =========================================================================
  * DSP context
  * ====================================================================== */
 
 #define UIOX_US_MEDIAN_MAX  9
 
 typedef struct {
     uiox_us_dsp_cfg_t cfg;
 
     /* IIR bandpass filter state (2nd order biquad) */
     float bp_b[3];      /**< Numerator coefficients                        */
     float bp_a[3];      /**< Denominator coefficients                      */
     float bp_x[2];      /**< Input delay line                              */
     float bp_y[2];      /**< Output delay line                             */
 
     /* IIR low-pass envelope filter state */
     float env_alpha;    /**< Single-pole IIR coefficient                   */
     float env_state;    /**< Filter state                                  */
 
     /* Median filter history */
     float     median_buf[UIOX_US_MEDIAN_MAX];
     uint8_t   median_idx;
     uint8_t   median_count;
 } uiox_us_dsp_t;
 
 /* =========================================================================
  * DSP API
  * ====================================================================== */
 
 /**
  * @brief  Initialise DSP context and compute filter coefficients.
  * @param  sample_rate_hz  ADC sampling rate.
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_us_dsp_init  (uiox_us_dsp_t *dsp,
                           const uiox_us_dsp_cfg_t *cfg,
                           uint32_t sample_rate_hz);
 
 /** Reset all filter states (call between measurements). */
 void uiox_us_dsp_reset (uiox_us_dsp_t *dsp);
 
 /**
  * @brief  Process a raw ADC echo frame → result.
  * @param  dsp     Initialised DSP context.
  * @param  raw     Raw ADC frame (int16 samples).
  * @param  sensor  Sensor descriptor (for SoS + range limits).
  * @param  out     Result to fill.
  */
 int  uiox_us_dsp_process_raw(uiox_us_dsp_t          *dsp,
                                const uiox_us_frame_t  *raw,
                                const uiox_us_sensor_t *sensor,
                                uiox_us_result_t       *out);
 
 /**
  * @brief  Convert a GPIO pulse-width measurement directly to a result.
  * @param  dsp     DSP context (for median filter).
  * @param  ticks   Echo pulse width in timer ticks.
  * @param  sensor  Sensor descriptor.
  * @param  hw      HAL (for timer_freq_hz).
  * @param  out     Result to fill.
  */
 int  uiox_us_dsp_process_ticks(uiox_us_dsp_t          *dsp,
                                  int64_t                 ticks,
                                  const uiox_us_sensor_t *sensor,
                                  const uiox_us_hw_t     *hw,
                                  uiox_us_result_t       *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_DSP_H */
 