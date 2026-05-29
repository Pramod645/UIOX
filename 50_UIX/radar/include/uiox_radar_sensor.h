/**
 * @file    uiox_radar_sensor.h
 * @brief   UIOX Radar sensor abstraction (chirp config, SPI register map).
 *
 * Abstracts FMCW chirp parameter programming for sensors such as:
 *   - TI AWR1843 / IWR6843 (mmWave)
 *   - Infineon BGT60TR13C
 *   - NXP TEF810x
 *
 * Chirp parameters define the radar waveform:
 *   Start frequency, bandwidth, ramp time, idle time, ADC samples.
 *
 * @date    2026-05-26
 */
//Layer 2b — Sensor Abstraction
 #ifndef UIOX_RADAR_SENSOR_H
 #define UIOX_RADAR_SENSOR_H
 
 #include "uiox_radar_hw.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * FMCW chirp configuration
  * ====================================================================== */
 
 typedef struct {
     /* Waveform */
     uint64_t  start_freq_hz;     /**< Chirp start frequency (e.g. 77e9)     */
     uint64_t  bandwidth_hz;      /**< Sweep bandwidth (e.g. 1e9 = 1 GHz)    */
     uint32_t  ramp_time_us;      /**< Chirp ramp duration (µs)              */
     uint32_t  idle_time_us;      /**< Inter-chirp idle time (µs)            */
     uint32_t  adc_start_time_us; /**< ADC valid start offset into ramp (µs) */
 
     /* ADC */
     uint16_t  num_adc_samples;   /**< Samples per chirp per RX              */
     uint32_t  adc_sample_rate_hz;/**< ADC sampling rate (Hz)                */
 
     /* Frame */
     uint16_t  num_chirps;        /**< Chirps per frame                      */
     uint32_t  frame_period_ms;   /**< Frame repetition period (ms)          */
 
     /* Antenna */
     uint8_t   tx_mask;           /**< TX enable bitmask (bit0=TX0, etc.)    */
     uint8_t   rx_mask;           /**< RX enable bitmask                     */
     int8_t    tx_power_dbm;      /**< TX power (dBm); -1 = max             */
 } uiox_radar_chirp_cfg_t;
 
 /* =========================================================================
  * Derived radar performance metrics (computed from chirp config)
  * ====================================================================== */
 
 typedef struct {
     float     range_res_m;       /**< Range resolution (metres)             */
     float     max_range_m;       /**< Maximum unambiguous range (metres)    */
     float     velocity_res_mps;  /**< Velocity resolution (m/s)            */
     float     max_velocity_mps;  /**< Maximum unambiguous velocity (m/s)   */
     float     range_fft_bin_m;   /**< Metres per range FFT bin              */
     float     doppler_fft_bin_mps; /**< m/s per Doppler FFT bin            */
 } uiox_radar_perf_t;
 
 /* =========================================================================
  * Sensor bus operations (SPI)
  * ====================================================================== */
 
 typedef struct {
     int (*spi_write)(uint16_t addr, uint32_t val, void *ctx);
     int (*spi_read) (uint16_t addr, uint32_t *val, void *ctx);
     int (*delay_ms) (uint32_t ms, void *ctx);
     void *ctx;
 } uiox_radar_bus_ops_t;
 
 /* =========================================================================
  * Sensor descriptor
  * ====================================================================== */
 
 typedef struct {
     const char              *name;       /**< e.g. "AWR1843"               */
     uint16_t                 device_id;
     uiox_radar_chirp_cfg_t   chirp;      /**< Current chirp configuration  */
     uiox_radar_perf_t        perf;       /**< Derived metrics              */
     const uiox_radar_bus_ops_t *bus;
     bool                     streaming;
 } uiox_radar_sensor_t;
 
 /* =========================================================================
  * Sensor API
  * ====================================================================== */
 
 /** Detect sensor: read device ID register, return 0 on match. */
 int  uiox_radar_sensor_detect (uiox_radar_sensor_t *s);
 
 /** Soft-reset the sensor and wait for ready. */
 int  uiox_radar_sensor_reset  (uiox_radar_sensor_t *s);
 
 /** Programme chirp parameters into sensor registers. */
 int  uiox_radar_sensor_config (uiox_radar_sensor_t *s,
                                 const uiox_radar_chirp_cfg_t *cfg);
 
 /** Compute and cache derived performance metrics from chirp config. */
 void uiox_radar_sensor_compute_perf(uiox_radar_sensor_t *s);
 
 /** Enable or disable radar frame transmission. */
 int  uiox_radar_sensor_stream (uiox_radar_sensor_t *s, bool enable);
 
 /** Return pointer to cached performance metrics. */
 const uiox_radar_perf_t *uiox_radar_sensor_perf(const uiox_radar_sensor_t *s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_SENSOR_H */
 