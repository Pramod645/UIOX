/**
 * @file    uiox_mic_demo.c
 * @brief   UIOX Microphone stack end-to-end demonstration.
 * @date    2026-06-03
 */

 #include "uiox_mic_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <math.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Simulated microphone data generator (sine + noise)
  * ====================================================================== */
 
 static int16_t s_sim_audio[UIOX_MIC_FRAME_SAMPLES * 10];
 
 static void gen_sim_audio(float freq_hz, float amp, float noise_amp,
                            uint32_t sample_rate)
 {
     uint32_t n = UIOX_MIC_FRAME_SAMPLES * 10u;
     static uint32_t lfsr = 0xACE1u;
     for (uint32_t i = 0; i < n; i++) {
         float t   = (float)i / (float)sample_rate;
         float sig = amp * sinf(2.0f * (float)M_PI * freq_hz * t);
         lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5;
         float ns  = noise_amp * ((float)(int32_t)lfsr * 0.00000001f);
         s_sim_audio[i] = (int16_t)((sig + ns) * 32767.0f);
     }
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int  stub_init   (uiox_mic_hw_t *hw)
 { (void)hw; printf("  [hal] init  I2S  mclk=%u Hz\n", hw->mclk_hz); return 0; }
 static void stub_deinit (uiox_mic_hw_t *hw) { (void)hw; }
 static int  stub_start  (uiox_mic_hw_t *hw)
 { (void)hw; printf("  [hal] I2S capture start\n"); return 0; }
 static void stub_stop   (uiox_mic_hw_t *hw)
 { (void)hw; printf("  [hal] I2S capture stop\n"); }
 
 static int stub_set_format(uiox_mic_hw_t *hw,
                             const uiox_mic_audio_fmt_t *fmt)
 {
     (void)hw;
     printf("  [hal] format  %u Hz  %uch  %u-bit\n",
            fmt->sample_rate, fmt->channels, fmt->bit_depth);
     return 0;
 }
 
 static int stub_set_gain(uiox_mic_hw_t *hw, uint8_t gain_db)
 { (void)hw; printf("  [hal] gain → %u dB\n", gain_db); return 0; }
 
 static int stub_set_mute(uiox_mic_hw_t *hw, bool mute)
 { (void)hw; printf("  [hal] mute %s\n", mute ? "ON":"OFF"); return 0; }
 
 static uint32_t s_sim_pos = 0;
 static uint32_t s_dma_count = 0;
 
 static int stub_dma_submit(uiox_mic_hw_t *hw,
                             uintptr_t phys, uint32_t bytes, bool last)
 {
     (void)last;
     s_dma_count++;
     printf("  [hal] DMA prime  %u bytes  (#%u)\n", bytes, s_dma_count);
     /* Copy simulated audio into the DMA buffer */
     uint32_t n_smp = bytes / 2u;
     int16_t *dst   = (int16_t *)phys;
     for (uint32_t i = 0; i < n_smp; i++) {
         dst[i] = s_sim_audio[(s_sim_pos++) %
                               (UIOX_MIC_FRAME_SAMPLES * 10u)];
     }
     hw->dma_done = true;   /* immediate completion in simulation */
     return 0;
 }
 
 static int stub_i2c_read(uiox_mic_hw_t *hw, uint8_t addr,
                           uint8_t reg, uint8_t *buf, uint16_t len)
 { (void)hw; (void)addr; (void)reg; memset(buf, 0, len); buf[0]=0x1Cu; return 0; }
 
 static int stub_i2c_write(uiox_mic_hw_t *hw, uint8_t addr,
                            uint8_t reg, const uint8_t *buf, uint16_t len)
 {
     (void)hw; (void)len;
     printf("  [hal] I2C W  0x%02X  reg=0x%02X  val=0x%02X\n",
            addr, reg, buf[0]);
     return 0;
 }
 
 static void stub_gpio_set(uiox_mic_hw_t *hw, uint8_t pin, bool val)
 { (void)hw; printf("  [hal] GPIO pin=%u %d\n", pin, (int)val); }
 
 static void stub_isr_dma_half(uiox_mic_hw_t *hw)
 { if (hw) hw->dma_half = true; }
 static void stub_isr_dma_done(uiox_mic_hw_t *hw)
 { if (hw) hw->dma_done = true; }
 static void stub_isr_overrun (uiox_mic_hw_t *hw)
 { if (hw) hw->overrun_count++; }
 
 static const uiox_mic_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .start         = stub_start,
     .stop          = stub_stop,
     .set_format    = stub_set_format,
     .set_gain      = stub_set_gain,
     .set_mute      = stub_set_mute,
     .dma_submit    = stub_dma_submit,
     .i2c_read      = stub_i2c_read,
     .i2c_write     = stub_i2c_write,
     .gpio_set      = stub_gpio_set,
     .isr_dma_half  = stub_isr_dma_half,
     .isr_dma_done  = stub_isr_dma_done,
     .isr_overrun   = stub_isr_overrun,
 };
 
 static uiox_mic_hw_t s_hw = {
     .base_addr      = 0x40015000uL,
     .irq_dma        = 57, .irq_overrun = 58,
     .caps           = UIOX_MIC_CAP_I2S | UIOX_MIC_CAP_DMA |
                       UIOX_MIC_CAP_STEREO | UIOX_MIC_CAP_HW_GAIN |
                       UIOX_MIC_CAP_HW_MUTE | UIOX_MIC_CAP_HIGH_SNR,
     .if_type        = UIOX_MIC_IF_I2S,
     .mclk_hz        = 12288000u,
     .enable_pin     = 3u, .mute_pin = 4u, .pwr_pin = 5u,
     .codec_i2c_addr = 0x1Au,
 };
 
 static void on_mic_event(uiox_mic_evt_t evt, void *ctx)
 {
     (void)ctx;
     printf("  [event] %s\n", uiox_mic_evt_name(evt));
 }
 
 int main(void)
 {
     printf("=== UIOX Microphone Stack Demo ===\n\n");
 
     printf("--- Generate test audio (1 kHz sine + noise) ---\n");
     gen_sim_audio(1000.0f, 0.3f, 0.05f, 16000u);
     printf("  %u samples generated\n\n", UIOX_MIC_FRAME_SAMPLES * 10u);
 
     printf("--- Open ---\n");
     uiox_mic_device_t dev;
     uiox_mic_open_params_t p;
     memset(&p, 0, sizeof(p));
     p.hw = &s_hw; p.hw_ops = &stub_ops;
     p.codec_type = UIOX_MIC_CODEC_ICS43434;
     p.codec_i2c  = s_hw.codec_i2c_addr;
     p.fmt.sample_rate = 16000u; p.fmt.channels = 1u;
     p.fmt.bit_depth = 16u; p.fmt.is_signed = true;
     p.dsp.agc_enabled    = true;  p.dsp.vad_enabled = true;
     p.dsp.noise_cancel   = true;  p.dsp.highpass    = true;
     p.dsp.agc_attack_ms  = 5.0f;  p.dsp.agc_release_ms = 100.0f;
     p.dsp.vad_threshold_db = -40.0f;
     p.dsp.hp_cutoff_hz   = 80.0f; p.dsp.sample_rate = 16000u;
     p.evt_cb = on_mic_event;
 
     int rc = uiox_mic_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
     printf("  Format: 16kHz mono 16-bit  codec=ICS43434\n");
 
     printf("\n--- Set gain 25 dB ---\n");
     uiox_mic_set_gain(&dev, 25u);
 
     printf("\n--- Start ---\n");
     rc = uiox_mic_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_mic_state_name(dev.subsys.state), rc);
 
     printf("\n--- Capture loop (8 ticks) ---\n");
     int16_t pcm[UIOX_MIC_FRAME_SAMPLES];
     for (uint32_t t = 10; t <= 80; t += 10) {
         uiox_mic_tick(&dev, t);
         uint32_t got = uiox_mic_read(&dev, pcm, UIOX_MIC_FRAME_SAMPLES);
         printf("  [t=%2u ms]  read=%u smp  voice=%s  energy=%.1f dBFS  agc=%.2f\n",
                t, got,
                uiox_mic_voice_active(&dev) ? "YES" : "NO",
                uiox_mic_energy_dbfs(&dev),
                uiox_mic_dsp_agc_gain(&dev.subsys.dsp));
     }
 
     printf("\n--- Mute/unmute ---\n");
     uiox_mic_set_mute(&dev, true);
     printf("  State: %s\n", uiox_mic_state_name(dev.subsys.state));
     uiox_mic_set_mute(&dev, false);
     printf("  State: %s\n", uiox_mic_state_name(dev.subsys.state));
 
     printf("\n--- Statistics ---\n");
     uiox_mic_print_stats(&dev);
 
     printf("\n--- Stop and close ---\n");
     uiox_mic_stop(&dev);
     uiox_mic_close(&dev);
     printf("  Device: CLOSED\n");
     printf("\n=== UIOX Mic Demo complete ===\n");
     return 0;
 }
 