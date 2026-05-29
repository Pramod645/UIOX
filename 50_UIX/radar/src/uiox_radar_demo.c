/**
 * @file    uiox_radar_demo.c
 * @brief   UIOX Radar stack end-to-end demonstration.
 *
 * Demonstrates: HAL init → sensor detect → chirp config →
 *   DSP init → streaming → frame capture → point cloud → tracking.
 *
 * Build: see Makefile.
 * @date    2026-05-26
 */
//Demo Application
 #include "uiox_radar_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
 /* =========================================================================
  * Stub SPI bus ops
  * ====================================================================== */
 
 static int stub_spi_write(uint16_t addr, uint32_t val, void *ctx)
 {
     (void)ctx;
     printf("  [spi] W  addr=0x%04X  val=0x%08X\n", addr, val);
     return 0;
 }
 
 static int stub_spi_read(uint16_t addr, uint32_t *val, void *ctx)
 {
     (void)ctx;
     /* Simulate device ID register = 0x1843 at addr 0x0000 */
     *val = (addr == 0x0000) ? 0x1843u : 0x0000u;
     printf("  [spi] R  addr=0x%04X  val=0x%08X\n", addr, *val);
     return 0;
 }
 
 static int stub_delay_ms(uint32_t ms, void *ctx)
 {
     (void)ctx; (void)ms;
     return 0;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int  stub_hw_init   (uiox_radar_hw_t *hw)
 { (void)hw; printf("  [hal] init\n");   return 0; }
 
 static void stub_hw_deinit (uiox_radar_hw_t *hw)
 { (void)hw; printf("  [hal] deinit\n"); }
 
 static int  stub_hw_start  (uiox_radar_hw_t *hw)
 { (void)hw; printf("  [hal] start\n");  return 0; }
 
 static void stub_hw_stop   (uiox_radar_hw_t *hw)
 { (void)hw; printf("  [hal] stop\n");   }
 
 static int stub_set_adc(uiox_radar_hw_t *hw,
                          uint16_t num_samples,
                          uint16_t num_chirps,
                          uiox_radar_adc_fmt_t fmt)
 {
     (void)hw;
     printf("  [hal] ADC  samples=%u  chirps=%u  fmt=%d\n",
            num_samples, num_chirps, (int)fmt);
     return 0;
 }
 
 static int stub_dma_queue(uiox_radar_hw_t *hw,
                            uintptr_t phys, uint32_t length)
 {
     (void)hw;
     printf("  [hal] DMA queue  phys=0x%08lX  len=%u\n",
            (unsigned long)phys, length);
     return 0;
 }
 
 static uint32_t s_frame_id = 0;
 
 static int stub_dma_complete(uiox_radar_hw_t *hw,
                               uintptr_t *phys_out,
                               uint32_t  *bytes_out)
 {
     (void)hw;
     /* Simulate a completed frame using first raw descriptor's paddr */
     extern uiox_radar_frame_t s_raw_desc[];
     *phys_out  = s_raw_desc[0].paddr;
     *bytes_out = s_raw_desc[0].capacity;
     s_raw_desc[0].frame_id = ++s_frame_id;
     s_raw_desc[0].ts_ns    = (uint64_t)s_frame_id * 50000000ull; /* 50ms/frame */
     s_raw_desc[0].in_use   = 1;
     /* Fill with synthetic ADC data (all zeros = no real targets) */
     return (int)*bytes_out;
 }
 
 static void stub_hw_isr   (uiox_radar_hw_t *hw) { (void)hw; }
 
 static int stub_spi_r_hw  (uiox_radar_hw_t *hw,
                              uint16_t addr, uint32_t *val)
 { (void)hw; *val = (addr == 0) ? 0x1843u : 0; return 0; }
 
 static int stub_spi_w_hw  (uiox_radar_hw_t *hw,
                              uint16_t addr, uint32_t val)
 { (void)hw; (void)addr; (void)val; return 0; }
 
 static const uiox_radar_hw_ops_t stub_hw_ops = {
     .init         = stub_hw_init,
     .deinit       = stub_hw_deinit,
     .start        = stub_hw_start,
     .stop         = stub_hw_stop,
     .set_adc      = stub_set_adc,
     .dma_queue    = stub_dma_queue,
     .dma_complete = stub_dma_complete,
     .isr          = stub_hw_isr,
     .spi_read     = stub_spi_r_hw,
     .spi_write    = stub_spi_w_hw,
 };
 
 /* =========================================================================
  * Hardware device instance (AWR1843 style)
  * ====================================================================== */
 
 static uiox_radar_hw_t s_hw = {
     .base_addr  = 0x60000000uL,
     .irq        = 80,
     .caps       = UIOX_RADAR_CAP_LVDS     |
                   UIOX_RADAR_CAP_SPI_CFG  |
                   UIOX_RADAR_CAP_DMA_CONTIG |
                   UIOX_RADAR_CAP_MULTI_RX |
                   UIOX_RADAR_CAP_MULTI_TX,
     .num_rx     = 4,
     .num_tx     = 3,
     .adc_bits   = 16,
     .adc_fmt    = UIOX_RADAR_ADC_COMPLEX_1X,
 };
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Radar Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Build open parameters                                            */
     /* ------------------------------------------------------------------ */
 
     uiox_radar_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw      = &s_hw;
     p.hw_ops  = &stub_hw_ops;
 
     /* SPI bus */
     p.bus.spi_write = stub_spi_write;
     p.bus.spi_read  = stub_spi_read;
     p.bus.delay_ms  = stub_delay_ms;
     p.bus.ctx       = NULL;
 
     /* Sensor */
     p.sensor_name = "AWR1843";
     p.device_id   = 0x1843;
 
     /* Interface */
     p.if_type     = UIOX_RADAR_IF_LVDS;
     p.num_rx      = 4;
     p.num_tx      = 3;
     p.queue_count = 3;
 
     /* Chirp — 77 GHz, 1 GHz BW, 256 samples, 128 chirps */
     p.chirp.start_freq_hz      = 77000000000ULL;
     p.chirp.bandwidth_hz       = 1000000000ULL;
     p.chirp.ramp_time_us       = 60;
     p.chirp.idle_time_us       = 5;
     p.chirp.adc_start_time_us  = 6;
     p.chirp.num_adc_samples    = 256;
     p.chirp.adc_sample_rate_hz = 5000000;
     p.chirp.num_chirps         = 128;
     p.chirp.frame_period_ms    = 50;
     p.chirp.tx_mask            = 0x07;  /* TX0,TX1,TX2 */
     p.chirp.rx_mask            = 0x0F;  /* RX0..RX3   */
     p.chirp.tx_power_dbm       = -1;    /* max         */
 
     /* DSP */
     p.dsp.range_fft_size        = 256;
     p.dsp.doppler_fft_size      = 128;
     p.dsp.angle_fft_size        = 64;
     p.dsp.range_win             = UIOX_RADAR_WIN_HANN;
     p.dsp.doppler_win           = UIOX_RADAR_WIN_HANN;
     p.dsp.angle_win             = UIOX_RADAR_WIN_HANN;
     p.dsp.cfar_type             = UIOX_RADAR_CFAR_CA;
     p.dsp.cfar_guard_cells      = 4;
     p.dsp.cfar_train_cells      = 16;
     p.dsp.cfar_threshold_db     = 15.0f;
     p.dsp.do_angle_est          = true;
 
     /* Tracker */
     p.tracker.max_tracks        = UIOX_RADAR_MAX_TRACKS;
     p.tracker.gate_range_m      = 2.0f;
     p.tracker.gate_velocity_mps = 1.5f;
     p.tracker.gate_azimuth_deg  = 10.0f;
     p.tracker.max_misses        = 5;
     p.tracker.confirm_hits      = 3;
 
     /* ------------------------------------------------------------------ */
     /* 2. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     uiox_radar_device_t dev;
     printf("--- Open ---\n");
     int rc = uiox_radar_open(&dev, &p);
     if (rc < 0) {
         printf("[error] u
     /* ------------------------------------------------------------------ */
    /* 2. Open device                                                      */
    /* ------------------------------------------------------------------ */

    uiox_radar_device_t dev;
    printf("--- Open ---\n");
    int rc = uiox_radar_open(&dev, &p);
    if (rc < 0) {
        printf("[error] uiox_radar_open failed: %d\n", rc);
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Print sensor performance metrics                                 */
    /* ------------------------------------------------------------------ */

    printf("\n--- Sensor Performance Metrics ---\n");
    const uiox_radar_perf_t *perf = uiox_radar_get_perf(&dev);
    if (perf) {
        printf("  Range resolution       : %.3f m\n",  perf->range_res_m);
        printf("  Max range              : %.2f m\n",  perf->max_range_m);
        printf("  Range FFT bin size     : %.4f m\n",  perf->range_fft_bin_m);
        printf("  Velocity resolution    : %.4f m/s\n",perf->velocity_res_mps);
        printf("  Max velocity           : %.2f m/s\n",perf->max_velocity_mps);
        printf("  Doppler FFT bin size   : %.4f m/s\n",perf->doppler_fft_bin_mps);
    }

    /* ------------------------------------------------------------------ */
    /* 4. Start streaming                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- Start streaming ---\n");
    rc = uiox_radar_start(&dev);
    if (rc < 0) {
        printf("[error] uiox_radar_start failed: %d\n", rc);
        uiox_radar_close(&dev);
        return 1;
    }
    printf("  Radar streaming: ACTIVE\n");
    printf("  Sensor         : %s (ID=0x%04X)\n",
           p.sensor_name, p.device_id);
    printf("  Interface      : LVDS  RX=%u  TX=%u\n",
           p.num_rx, p.num_tx);
    printf("  Chirp          : %llu Hz  BW=%llu Hz"
           "  %u chirps  %u samples  %u fps\n",
           (unsigned long long)p.chirp.start_freq_hz,
           (unsigned long long)p.chirp.bandwidth_hz,
           p.chirp.num_chirps,
           p.chirp.num_adc_samples,
           (unsigned)(1000u / p.chirp.frame_period_ms));

    /* ------------------------------------------------------------------ */
    /* 5. Process frames                                                   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Frame processing (%d frames) ---\n", 6);

    const int NUM_FRAMES = 6;
    int processed = 0;

    while (processed < NUM_FRAMES) {

        uiox_radar_point_cloud_t *cloud = uiox_radar_get_point_cloud(&dev);
        if (!cloud) {
            /* No frame ready yet — in real system sleep and retry */
            /* uiox_os_sleep_ms(1); */
            continue;
        }

        printf("\n  Frame %u  (ts=%.3f s)  detections=%u\n",
               cloud->frame_id,
               (double)cloud->ts_ns / 1e9,
               cloud->num_points);

        for (uint16_t pi = 0; pi < cloud->num_points; pi++) {
            const uiox_radar_point_t *pt = &cloud->points[pi];
            printf("    [%3u] x=%6.2f m  y=%6.2f m  z=%6.2f m"
                   "  v=%+6.2f m/s  SNR=%5.1f dB  track=%u\n",
                   pi,
                   pt->x_m, pt->y_m, pt->z_m,
                   pt->velocity_mps,
                   pt->snr_db,
                   pt->track_id);
        }

        if (cloud->num_points == 0)
            printf("    (no detections this frame)\n");

        processed++;
    }

    /* ------------------------------------------------------------------ */
    /* 6. Query active tracks                                              */
    /* ------------------------------------------------------------------ */

    printf("\n--- Active tracks ---\n");
    uiox_radar_track_t tracks[UIOX_RADAR_MAX_TRACKS];
    uint16_t num_tracks = uiox_radar_get_tracks(&dev, tracks,
                                                 UIOX_RADAR_MAX_TRACKS);
    if (num_tracks == 0) {
        printf("  No confirmed tracks (need %u hits to confirm)\n",
               p.tracker.confirm_hits);
    } else {
        for (uint16_t ti = 0; ti < num_tracks; ti++) {
            const uiox_radar_track_t *t = &tracks[ti];
            printf("  Track %3u : range=%6.2f m  vel=%+6.2f m/s"
                   "  az=%+6.1f deg  SNR=%5.1f dB"
                   "  age=%u  hits=%u\n",
                   t->id,
                   t->range_m,
                   t->velocity_mps,
                   t->azimuth_deg,
                   t->snr_db,
                   t->age,
                   t->hits);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 7. Mid-stream chirp reconfiguration                                 */
    /* ------------------------------------------------------------------ */

    printf("\n--- Chirp reconfiguration (long-range mode) ---\n");
    uiox_radar_chirp_cfg_t new_chirp = p.chirp;
    new_chirp.bandwidth_hz    = 500000000ULL;  /* 500 MHz — longer range */
    new_chirp.num_adc_samples = 512;
    new_chirp.ramp_time_us    = 120;
    rc = uiox_radar_set_chirp(&dev, &new_chirp);
    printf("  New chirp BW=%llu Hz  samples=%u  ramp=%u us  rc=%d\n",
           (unsigned long long)new_chirp.bandwidth_hz,
           new_chirp.num_adc_samples,
           new_chirp.ramp_time_us, rc);

    const uiox_radar_perf_t *perf2 = uiox_radar_get_perf(&dev);
    if (perf2) {
        printf("  Updated range res      : %.3f m\n", perf2->range_res_m);
        printf("  Updated max range      : %.2f m\n", perf2->max_range_m);
    }

    /* ------------------------------------------------------------------ */
    /* 8. Buffer pool status                                               */
    /* ------------------------------------------------------------------ */

    printf("\n--- Buffer pool status ---\n");
    printf("  Raw frames free : %u / %u\n",
           uiox_radar_buf_raw_free_count(), UIOX_RADAR_RAW_POOL_SIZE);
    printf("  DSP frames free : %u / %u\n",
           uiox_radar_buf_dsp_free_count(), UIOX_RADAR_DSP_POOL_SIZE);

    /* ------------------------------------------------------------------ */
    /* 9. Stop and close                                                   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Stop and close ---\n");
    uiox_radar_stop(&dev);
    printf("  Streaming : STOPPED\n");
    uiox_radar_close(&dev);
    printf("  Device    : CLOSED\n");

    printf("\n=== UIOX Radar Demo complete ===\n");
    return 0;
}
