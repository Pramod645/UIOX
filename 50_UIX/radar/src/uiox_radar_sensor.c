/**
 * @file    uiox_radar_sensor.c
 * @brief   UIOX Radar sensor abstraction implementation.
 * @date    2026-05-26
 */

 #include "uiox_radar_sensor.h"
 #include <string.h>
 #include <errno.h>
 #include <math.h>
 
 /* Speed of light */
 #define UIOX_RADAR_C_MPS    299792458.0f
 
 /* -------------------------------------------------------------------------
  * Register address stubs (AWR1843-style)
  * ---------------------------------------------------------------------- */
 
 #define REG_DEVICE_ID           0x0000
 #define REG_SOFT_RESET          0x0001
 #define REG_START_FREQ_LO       0x0010
 #define REG_START_FREQ_HI       0x0011
 #define REG_BANDWIDTH_LO        0x0012
 #define REG_BANDWIDTH_HI        0x0013
 #define REG_RAMP_TIME           0x0014
 #define REG_IDLE_TIME           0x0015
 #define REG_ADC_START_TIME      0x0016
 #define REG_NUM_ADC_SAMPLES     0x0017
 #define REG_ADC_SAMPLE_RATE     0x0018
 #define REG_NUM_CHIRPS          0x0019
 #define REG_FRAME_PERIOD        0x001A
 #define REG_TX_MASK             0x001B
 #define REG_RX_MASK             0x001C
 #define REG_TX_POWER            0x001D
 #define REG_STREAM_CTRL         0x001E
 
 static inline int spi_wr(const uiox_radar_sensor_t *s,
                           uint16_t addr, uint32_t val)
 {
     if (!s->bus || !s->bus->spi_write) return -ENOSYS;
     return s->bus->spi_write(addr, val, s->bus->ctx);
 }
 
 static inline int spi_rd(const uiox_radar_sensor_t *s,
                           uint16_t addr, uint32_t *val)
 {
     if (!s->bus || !s->bus->spi_read) return -ENOSYS;
     return s->bus->spi_read(addr, val, s->bus->ctx);
 }
 
 /* -------------------------------------------------------------------------
  * Public API
  * ---------------------------------------------------------------------- */
 
 int uiox_radar_sensor_detect(uiox_radar_sensor_t *s)
 {
     if (!s || !s->bus) return -EINVAL;
     uint32_t id = 0;
     int rc = spi_rd(s, REG_DEVICE_ID, &id);
     if (rc < 0) return rc;
     return ((uint16_t)id == s->device_id) ? 0 : -ENODEV;
 }
 
 int uiox_radar_sensor_reset(uiox_radar_sensor_t *s)
 {
     if (!s || !s->bus) return -EINVAL;
     int rc = spi_wr(s, REG_SOFT_RESET, 0x1);
     if (rc < 0) return rc;
     if (s->bus->delay_ms) s->bus->delay_ms(10, s->bus->ctx);
     /* Wait for reset complete */
     uint32_t status = 1;
     for (int i = 0; i < 100 && (status & 0x1); i++) {
         spi_rd(s, REG_SOFT_RESET, &status);
         if (s->bus->delay_ms) s->bus->delay_ms(1, s->bus->ctx);
     }
     return (status & 0x1) ? -ETIMEDOUT : 0;
 }
 
 int uiox_radar_sensor_config(uiox_radar_sensor_t *s,
                               const uiox_radar_chirp_cfg_t *cfg)
 {
     if (!s || !cfg) return -EINVAL;
     memcpy(&s->chirp, cfg, sizeof(*cfg));
 
     /* Programme waveform registers */
     spi_wr(s, REG_START_FREQ_LO, (uint32_t)(cfg->start_freq_hz & 0xFFFFFFFF));
     spi_wr(s, REG_START_FREQ_HI, (uint32_t)(cfg->start_freq_hz >> 32));
     spi_wr(s, REG_BANDWIDTH_LO,  (uint32_t)(cfg->bandwidth_hz  & 0xFFFFFFFF));
     spi_wr(s, REG_BANDWIDTH_HI,  (uint32_t)(cfg->bandwidth_hz  >> 32));
     spi_wr(s, REG_RAMP_TIME,     cfg->ramp_time_us);
     spi_wr(s, REG_IDLE_TIME,     cfg->idle_time_us);
     spi_wr(s, REG_ADC_START_TIME,cfg->adc_start_time_us);
 
     /* Programme ADC */
     spi_wr(s, REG_NUM_ADC_SAMPLES, cfg->num_adc_samples);
     spi_wr(s, REG_ADC_SAMPLE_RATE, cfg->adc_sample_rate_hz);
 
     /* Programme frame */
     spi_wr(s, REG_NUM_CHIRPS,   cfg->num_chirps);
     spi_wr(s, REG_FRAME_PERIOD, cfg->frame_period_ms);
 
     /* Programme antenna */
     spi_wr(s, REG_TX_MASK,  cfg->tx_mask);
     spi_wr(s, REG_RX_MASK,  cfg->rx_mask);
     spi_wr(s, REG_TX_POWER, (uint32_t)(int32_t)cfg->tx_power_dbm);
 
     /* Compute derived metrics */
     uiox_radar_sensor_compute_perf(s);
 
     return 0;
 }
 
 void uiox_radar_sensor_compute_perf(uiox_radar_sensor_t *s)
 {
     if (!s) return;
     const uiox_radar_chirp_cfg_t *c = &s->chirp;
     uiox_radar_perf_t *p = &s->perf;
 
     float bw  = (float)c->bandwidth_hz;
     float fs  = (float)c->adc_sample_rate_hz;
     float tc  = (float)c->ramp_time_us * 1e-6f;
     float N   = (float)c->num_adc_samples;
     float M   = (float)c->num_chirps;
     float fc  = (float)(c->start_freq_hz + c->bandwidth_hz / 2);
     float lambda = UIOX_RADAR_C_MPS / fc;
 
     /* Range resolution: c / (2 × BW) */
     p->range_res_m = UIOX_RADAR_C_MPS / (2.0f * bw);
 
     /* Max unambiguous range: (fs × c) / (2 × slope) */
     float slope = bw / tc;
     p->max_range_m = (fs * UIOX_RADAR_C_MPS) / (2.0f * slope);
 
     /* Range FFT bin size */
     p->range_fft_bin_m = p->max_range_m / N;
 
     /* Velocity resolution: lambda / (2 × M × Tc) */
     p->velocity_res_mps = lambda / (2.0f * M * tc);
 
     /* Max unambiguous velocity: lambda / (4 × Tc) */
     p->max_velocity_mps = lambda / (4.0f * tc);
 
     /* Doppler FFT bin size */
     p->doppler_fft_bin_mps = p->max_velocity_mps / (M / 2.0f);
 }
 
 int uiox_radar_sensor_stream(uiox_radar_sensor_t *s, bool enable)
 {
     if (!s) return -EINVAL;
     int rc = spi_wr(s, REG_STREAM_CTRL, enable ? 0x1u : 0x0u);
     if (rc == 0) s->streaming = enable;
     return rc;
 }
 
 const uiox_radar_perf_t *uiox_radar_sensor_perf(const uiox_radar_sensor_t *s)
 {
     return s ? &s->perf : NULL;
 }
 