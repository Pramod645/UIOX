/**
 * @file    uiox_radar_dsp.h
 * @brief   UIOX Radar DSP layer — FFT, CFAR, angle estimation.
 *
 * Transforms raw ADC frames through the radar signal chain:
 *
 *   Raw ADC (range × chirp × RX)
 *     → 1D Range FFT      → range-compressed cube
 *     → 2D Doppler FFT    → range-Doppler map (per RX)
 *     → CFAR detection    → detected cell list
 *     → Angle estimation  → azimuth / elevation per detection
 *
 * Uses fixed-point arithmetic on 16-bit I+Q samples for embedded targets.
 * Floating-point output for detection results.
 *
 * @date    2026-05-26
 */
//Layer 3 — Signal Processing
 #ifndef UIOX_RADAR_DSP_H
 #define UIOX_RADAR_DSP_H
 
 #include "uiox_radar_buf.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Complex sample type (I+Q, 16-bit each)
  * ====================================================================== */
 
 typedef struct {
     int16_t i;
     int16_t q;
 } uiox_cplx16_t;
 
 typedef struct {
     float i;
     float q;
 } uiox_cplx32f_t;
 
 /* =========================================================================
  * DSP configuration
  * ====================================================================== */
 
 typedef enum {
     UIOX_RADAR_WIN_NONE = 0,    /**< Rectangular (no windowing)            */
     UIOX_RADAR_WIN_HANN,        /**< Hann window                           */
     UIOX_RADAR_WIN_HAMMING,     /**< Hamming window                        */
     UIOX_RADAR_WIN_BLACKMAN,    /**< Blackman window                       */
 } uiox_radar_window_t;
 
 typedef enum {
     UIOX_RADAR_CFAR_CA = 0,     /**< Cell-averaging CFAR                   */
     UIOX_RADAR_CFAR_CASO,       /**< Smallest-of CFAR                      */
     UIOX_RADAR_CFAR_CAGO,       /**< Greatest-of CFAR                      */
     UIOX_RADAR_CFAR_OS,         /**< Ordered statistic CFAR                */
 } uiox_radar_cfar_type_t;
 
 typedef struct {
     /* FFT */
     uint16_t            range_fft_size;    /**< Range FFT size (power of 2) */
     uint16_t            doppler_fft_size;  /**< Doppler FFT size (pow of 2) */
     uint16_t            angle_fft_size;    /**< Angle FFT size (pow of 2)   */
     uiox_radar_window_t range_win;
     uiox_radar_window_t doppler_win;
     uiox_radar_window_t angle_win;
 
     /* CFAR */
     uiox_radar_cfar_type_t cfar_type;
     uint8_t             cfar_guard_cells;  /**< Guard cells each side       */
     uint8_t             cfar_train_cells;  /**< Training cells each side    */
     float               cfar_threshold_db; /**< Detection threshold (dB)   */
 
     /* Angle */
     bool                do_angle_est;      /**< Compute azimuth/elevation   */
 } uiox_radar_dsp_cfg_t;
 
 /* =========================================================================
  * Detection result (one per detected target)
  * ====================================================================== */
 
 typedef struct {
     float   range_m;            /**< Detected range (metres)               */
     float   velocity_mps;       /**< Detected velocity (m/s, +ve=approach) */
     float   azimuth_deg;        /**< Azimuth angle (degrees)               */
     float   elevation_deg;      /**< Elevation angle (degrees)             */
     float   snr_db;             /**< Signal-to-noise ratio (dB)            */
     float   rcs_dbsm;           /**< Estimated RCS (dBsm)                  */
     uint16_t range_bin;         /**< Range FFT bin index                   */
     uint16_t doppler_bin;       /**< Doppler FFT bin index                 */
 } uiox_radar_detection_t;
 
 /* =========================================================================
  * DSP pipeline output frame
  * ====================================================================== */
 
 #define UIOX_RADAR_MAX_DETECTIONS   128
 
 typedef struct {
     uiox_radar_detection_t detections[UIOX_RADAR_MAX_DETECTIONS];
     uint16_t   num_detections;
     uint32_t   frame_id;
     uint64_t   ts_ns;
 } uiox_radar_det_frame_t;
 
 /* =========================================================================
  * DSP context
  * ====================================================================== */
 
 typedef struct {
     uiox_radar_dsp_cfg_t cfg;
     /* Twiddle factor tables (pre-computed at init) */
     uiox_cplx32f_t      *range_twiddle;
     uiox_cplx32f_t      *doppler_twiddle;
     uiox_cplx32f_t      *angle_twiddle;
     /* Window coefficient tables */
     float               *range_win_coeff;
     float               *doppler_win_coeff;
 } uiox_radar_dsp_t;
 
 /* =========================================================================
  * DSP API
  * ====================================================================== */
 
 /**
  * @brief  Initialise DSP context: precompute twiddle factors + windows.
  * @return 0 on success, negative errno on failure.
  */
 int uiox_radar_dsp_init(uiox_radar_dsp_t *dsp,
                          const uiox_radar_dsp_cfg_t *cfg);
 
 /** Release all resources held by dsp context. */
 void uiox_radar_dsp_deinit(uiox_radar_dsp_t *dsp);
 
 /**
  * @brief  Run full DSP chain on a raw ADC frame.
  *
  * Chain: window → range FFT → doppler FFT → CFAR → angle estimation
  *
  * @param  dsp     Initialised DSP context.
  * @param  raw     Raw ADC frame (must be type UIOX_RADAR_FRAME_RAW).
  * @param  out     Pre-allocated detection frame to fill.
  * @return 0 on success, negative errno on failure.
  */
 int uiox_radar_dsp_process(uiox_radar_dsp_t   *dsp,
                             const uiox_radar_frame_t *raw,
                             uiox_radar_det_frame_t   *out);
 
 /* Individual stages (exposed for testing / partial pipelines) */
 int uiox_radar_dsp_range_fft  (uiox_radar_dsp_t *dsp,
                                 const uiox_radar_frame_t *raw,
                                 uiox_cplx32f_t *range_cube);
 
 int uiox_radar_dsp_doppler_fft(uiox_radar_dsp_t *dsp,
                                 uiox_cplx32f_t *range_cube,
                                 uint16_t num_rx,
                                 uint16_t num_chirps,
                                 uint16_t num_range_bins,
                                 float *rdmap);
 
 int uiox_radar_dsp_cfar       (uiox_radar_dsp_t *dsp,
                                 const float *rdmap,
                                 uint16_t num_range_bins,
                                 uint16_t num_doppler_bins,
                                 uiox_radar_det_frame_t *out);
 
 int uiox_radar_dsp_angle      (uiox_radar_dsp_t *dsp,
                                 const uiox_cplx32f_t *range_cube,
                                 uiox_radar_det_frame_t *dets,
                                 uint16_t num_rx,
                                 uint16_t num_range_bins);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_DSP_H */
 