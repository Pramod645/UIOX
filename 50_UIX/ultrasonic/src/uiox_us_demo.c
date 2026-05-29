/**
 * @file    uiox_us_demo.c
 * @brief   UIOX Ultrasonic sensor stack end-to-end demonstration.
 *
 * Demonstrates the complete flow:
 *   HAL init → sensor detect → pulse config → DSP init →
 *   streaming → multi-channel measurement → zone detection →
 *   statistics → teardown.
 *
 * Uses stub HAL ops — replace with real timer/GPIO/DMA driver
 * for production targets (HC-SR04, MB1013, TDC1000, US-100).
 *
 * Build: see Makefile.
 * @date    2026-05-26
 */
//Demo Application
 #include "uiox_us_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <math.h>
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_us_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  (timer_freq=%u Hz)\n", hw->timer_freq_hz);
     return 0;
 }
 
 static void stub_deinit(uiox_us_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] deinit\n");
 }
 
 static int stub_trigger(uiox_us_hw_t *hw, uint8_t ch,
                          const uiox_us_trig_cfg_t *cfg)
 {
     (void)hw;
     printf("  [hal] trigger  ch=%u  pulse=%u us  period=%u ms\n",
            ch, cfg->pulse_width_us, cfg->period_ms);
     return 0;
 }
 
 /* Simulate echo returns for 4 channels at different distances */
 static const float s_sim_dist_m[4] = { 0.35f, 1.20f, 2.85f, 0.08f };
 
 static int64_t stub_echo_wait(uiox_us_hw_t *hw, uint8_t ch,
                                uint32_t timeout_ms)
 {
     (void)timeout_ms;
     /* Compute ticks from simulated distance:
      *   ticks = (2 × dist / SoS) × timer_freq */
     float sos    = 343.2f;
     float dist_m = s_sim_dist_m[ch % 4];
     float time_s = (2.0f * dist_m) / sos;
     int64_t ticks = (int64_t)(time_s * (float)hw->timer_freq_hz);
     printf("  [hal] echo_wait  ch=%u  ticks=%lld  (~%.3f m)\n",
            ch, (long long)ticks, dist_m);
     return ticks;
 }
 
 static uint64_t stub_timer_now(uiox_us_hw_t *hw)
 {
     (void)hw;
     static uint64_t t = 0;
     return t += 1000000;
 }
 
 static int stub_read_temp_mc(uiox_us_hw_t *hw, int32_t *mc)
 {
     (void)hw;
     *mc = 23500;  /* 23.5 °C */
     printf("  [hal] temp = %d m°C  (%.1f °C)\n", *mc, (float)*mc / 1000.0f);
     return 0;
 }
 
 static int stub_spi_read (uiox_us_hw_t *hw, uint8_t addr, uint8_t *val)
 { (void)hw; *val = (addr == 0) ? 0xA5u : 0x00u; return 0; }
 
 static int stub_spi_write(uiox_us_hw_t *hw, uint8_t addr, uint8_t val)
 { (void)hw; (void)addr; (void)val; return 0; }
 
 static int stub_uart_read(uiox_us_hw_t *hw, uint8_t *buf, uint16_t len)
 { (void)hw; memset(buf, 0, len); return 0; }
 
 static int stub_uart_write(uiox_us_hw_t *hw,
                             const uint8_t *buf, uint16_t len)
 { (void)hw; (void)buf; (void)len; return 0; }
 
 static int stub_dma_queue(uiox_us_hw_t *hw,
                            uintptr_t phys, uint32_t len)
 { (void)hw; (void)phys; (void)len; return 0; }
 
 static int stub_dma_complete(uiox_us_hw_t *hw,
                               uintptr_t *phys_out, uint32_t *bytes_out)
 {
     (void)hw;
     *phys_out  = 0;
     *bytes_out = 0;
     return 0;
 }
 
 static void stub_isr(uiox_us_hw_t *hw) { (void)hw; }
 
 static const uiox_us_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .trigger       = stub_trigger,
     .echo_wait     = stub_echo_wait,
     .timer_now     = stub_timer_now,
     .read_temp_mc  = stub_read_temp_mc,
     .spi_read      = stub_spi_read,
     .spi_write     = stub_spi_write,
     .uart_read     = stub_uart_read,
     .uart_write    = stub_uart_write,
     .dma_queue     = stub_dma_queue,
     .dma_complete  = stub_dma_complete,
     .isr           = stub_isr,
 };
 
 /* =========================================================================
  * Hardware device instance (4-channel HC-SR04 on GPIO + timer)
  * ====================================================================== */
 
 static uiox_us_hw_t s_hw = {
     .base_addr    = 0x40020000uL,  /* Timer MMIO base               */
     .irq_echo     = 28,
     .irq_trig     = 29,
     .caps         = UIOX_US_CAP_GPIO_TRIG  |
                     UIOX_US_CAP_GPIO_ECHO  |
                     UIOX_US_CAP_MULTI_SENSOR |
                     UIOX_US_CAP_TEMP_SENSOR,
     .if_type      = UIOX_US_IF_GPIO,
     .echo_mode    = UIOX_US_ECHO_PULSE_WIDTH,
     .timer_freq_hz= 1000000u,   /* 1 MHz → 1 µs resolution         */
     .num_channels = 4,
     /* GPIO pins: trig = PA0..PA3, echo = PB0..PB3 (platform IDs)  */
     .trig_pin     = { 0, 1, 2, 3 },
     .echo_pin     = { 16, 17, 18, 19 },
 };
 
 /* =========================================================================
  * Zone colour helper
  * ====================================================================== */
 
 static const char *zone_colour(uiox_us_zone_t z)
 {
     switch (z) {
     case UIOX_US_ZONE_NEAR:    return "🔴 NEAR";
     case UIOX_US_ZONE_CAUTION: return "🟡 CAUTION";
     case UIOX_US_ZONE_CLEAR:   return "🟢 CLEAR";
     default:                   return "⚪ UNKNOWN";
     }
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Ultrasonic Sensor Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Build open parameters                                            */
     /* ------------------------------------------------------------------ */
 
     uiox_us_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw         = &s_hw;
     p.hw_ops     = &stub_ops;
     p.sensor_name= "HC-SR04";
     p.if_type    = UIOX_US_IF_GPIO;
     p.num_channels = 4;
 
     /* Pulse config — HC-SR04: 10 µs trigger, 40 kHz, up to 4 m */
     p.pulse.freq_hz         = 40000u;
     p.pulse.num_pulses      = 8u;
     p.pulse.pulse_width_us  = 10u;
     p.pulse.blanking_us     = 300u;
     p.pulse.max_range_m     = 4.0f;
     p.pulse.min_range_m     = 0.02f;
     p.pulse.beam_angle_deg  = 15.0f;
     p.pulse.period_ms       = 60u;
     p.pulse.timeout_ms      = 30u;
 
     /* DSP config — GPIO mode (no ADC; bandpass/envelope not used) */
     p.dsp.bp_center_hz      = 40000.0f;
     p.dsp.bp_bandwidth_hz   = 5000.0f;
     p.dsp.env_cutoff_hz     = 1000.0f;
     p.dsp.threshold_pct     = 0.30f;
     p.dsp.blanking_samples  = 30u;
     p.dsp.median_window     = 5u;
     p.dsp.do_interpolate    = true;
 
     p.sample_rate_hz         = 1000000u;  /* 1 Msps (ADC mode) */
     p.echo_window_us         = 25000u;    /* 25 ms window → ~4 m max */
 
     /* Zone thresholds */
     p.zones.near_m           = 0.30f;
     p.zones.caution_m        = 1.00f;
     p.zones.hysteresis_m     = 0.05f;
 
     p.temp_update_interval   = 10u;  /* update SoS every 10 measurements */
 
     /* ------------------------------------------------------------------ */
     /* 2. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_us_device_t dev;
     int rc = uiox_us_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_us_open failed: %d\n", rc);
         return 1;
     }
     printf("  Sensor     : %s\n",  p.sensor_name);
     printf("  Interface  : GPIO  channels=%u\n", p.num_channels);
     printf("  Range      : %.2f m … %.2f m\n",
            p.pulse.min_range_m, p.pulse.max_range_m);
     printf("  Frequency  : %u Hz\n", p.pulse.freq_hz);
     printf("  Zones      : NEAR < %.2f m  CAUTION < %.2f m\n",
            p.zones.near_m, p.zones.caution_m);
 
     /* ------------------------------------------------------------------ */
     /* 3. Start pipeline                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_us_start(&dev);
     if (rc < 0) {
         printf("[error] uiox_us_start failed: %d\n", rc);
         uiox_us_close(&dev);
         return 1;
     }
 
     float sos = uiox_us_sensor_sos(&dev.pipeline.sensor);
     printf("  Speed of sound : %.1f m/s  (%.1f °C)\n",
            sos,
            (float)dev.pipeline.sensor.temp.temp_mc / 1000.0f);
 
     /* ------------------------------------------------------------------ */
     /* 4. Measurement loop — 8 rounds, all channels                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Measurement loop (8 rounds × 4 channels) ---\n");
 
     for (int round = 0; round < 8; round++) {
         printf("\n  Round %d:\n", round + 1);
 
         int n = uiox_us_measure_all(&dev);
         printf("  Channels measured: %d\n", n);
 
         for (uint8_t ch = 0; ch < p.num_channels; ch++) {
             const uiox_us_result_t *r = uiox_us_last_result(&dev, ch);
             uiox_us_zone_t zone       = uiox_us_zone(&dev, ch);
 
             if (!r) continue;
 
             if (r->valid) {
                 printf("    ch%u : %6.3f m  ToF=%7.1f µs"
                        "  SoS=%.1f m/s  %s\n",
                        ch,
                        r->distance_m,
                        r->tof_us,
                        r->sos_mps,
                        zone_colour(zone));
             } else {
                 printf("    ch%u : --- (no echo)  %s\n",
                        ch, zone_colour(zone));
             }
         }
     }
 
     /* ------------------------------------------------------------------ */
     /* 5. Mid-stream pulse reconfiguration (long-range mode)              */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Pulse reconfiguration (long-range mode) ---\n");
     uiox_us_pulse_cfg_t new_pulse = p.pulse;
     new_pulse.num_pulses    = 16u;
     new_pulse.pulse_width_us= 20u;
     new_pulse.max_range_m   = 8.0f;
     new_pulse.period_ms     = 120u;
 
     rc = uiox_us_set_pulse(&dev, &new_pulse);
     printf("  New pulse: %u pulses  %u µs  max=%.1f m  rc=%d\n",
            new_pulse.num_pulses, new_pulse.pulse_width_us,
            new_pulse.max_range_m, rc);
 
     /* One more round with updated config */
     printf("\n  Post-reconfig measurement:\n");
     uiox_us_measure_all(&dev);
     for (uint8_t ch = 0; ch < p.num_channels; ch++) {
         const uiox_us_result_t *r = uiox_us_last_result(&dev, ch);
         if (r && r->valid)
             printf("    ch%u : %.3f m  %s\n",
                    ch, r->distance_m,
                    zone_colour(uiox_us_zone(&dev, ch)));
     }
 
     /* ------------------------------------------------------------------ */
     /* 6. Force temperature update                                         */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Temperature update ---\n");
     uiox_us_update_temp(&dev);
     printf("  SoS updated: %.2f m/s\n",
            uiox_us_sensor_sos(&dev.pipeline.sensor));
 
     /* ------------------------------------------------------------------ */
     /* 7. Per-channel statistics                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Channel statistics ---\n");
     for (uint8_t ch = 0; ch < p.num_channels; ch++) {
         float min_m, max_m, mean_m;
         uint32_t valid_cnt, invalid_cnt;
         uiox_us_get_stats(&dev, ch,
                            &min_m, &max_m, &mean_m,
                            &valid_cnt, &invalid_cnt);
         printf("  ch%u : min=%.3f m  max=%.3f m  mean=%.3f m"
                "  valid=%u  invalid=%u\n",
                ch, min_m, max_m, mean_m,
                valid_cnt, invalid_cnt);
     }
 
     /* ------------------------------------------------------------------ */
     /* 8. Buffer pool status                                               */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Buffer pool status ---\n");
     printf("  Raw buffers free : %u / %u\n",
            uiox_us_buf_raw_free(), UIOX_US_RAW_POOL_SIZE);
     printf("  DSP buffers free : %u / %u\n",
            uiox_us_buf_dsp_free(), UIOX_US_DSP_POOL_SIZE);
 
     /* ------------------------------------------------------------------ */
     /* 9. Stop and close                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_us_stop(&dev);
     printf("  Pipeline : STOPPED\n");
     uiox_us_close(&dev);
     printf("  Device   : CLOSED\n");
 
     printf("\n=== UIOX Ultrasonic Demo complete ===\n");
     return 0;
 }
 