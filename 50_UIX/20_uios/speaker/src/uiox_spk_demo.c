/**
 * @file    uiox_spk_demo.c
 * @brief   UIOX Music Speaker stack end-to-end demonstration.
 *
 * Demonstrates: HAL init → codec detect → format config → start →
 *   tone generation → stream play → EQ → volume → pause/resume →
 *   multi-stream mix → fade → stats → stop.
 *
 * @date    2026-06-01
 */

 #include "uiox_spk_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <math.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Synthetic audio — generate sine wave tones
  * ====================================================================== */
 
 #define DEMO_SAMPLE_RATE  48000u
 #define DEMO_FRAME_SMP    UIOX_SPK_PCM_FRAME_SAMPLES  /* 480 stereo pairs */
 
 static int16_t s_sine_440[DEMO_FRAME_SMP * 2];  /* 440 Hz sine (stereo) */
 static int16_t s_sine_660[DEMO_FRAME_SMP * 2];  /* 660 Hz sine (stereo) */
 static int16_t s_noise   [DEMO_FRAME_SMP * 2];  /* White noise          */
 
 static void gen_sine(int16_t *buf, uint32_t n_stereo,
                      float freq_hz, float amp, uint32_t sample_rate)
 {
     for (uint32_t i = 0; i < n_stereo; i++) {
         float t   = (float)i / (float)sample_rate;
         int16_t s = (int16_t)(amp * 32767.0f *
                     sinf(2.0f * (float)M_PI * freq_hz * t));
         buf[i * 2]     = s;
         buf[i * 2 + 1] = s;
     }
 }
 
 static void gen_noise(int16_t *buf, uint32_t n_stereo, float amp)
 {
     static uint32_t lfsr = 0xACE1u;
     for (uint32_t i = 0; i < n_stereo; i++) {
         lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5;
         int16_t s = (int16_t)((float)(int32_t)lfsr * amp * 0.0001f);
         buf[i * 2]     = s;
         buf[i * 2 + 1] = s;
     }
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static uint32_t s_dma_count = 0;
 
 static int stub_init(uiox_spk_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  I2S  mclk=%u Hz\n", hw->mclk_hz);
     return 0;
 }
 
 static void stub_deinit  (uiox_spk_hw_t *hw) { (void)hw; }
 static int  stub_start   (uiox_spk_hw_t *hw)
 { (void)hw; printf("  [hal] I2S DMA start\n"); return 0; }
 static void stub_stop    (uiox_spk_hw_t *hw)
 { (void)hw; printf("  [hal] I2S DMA stop\n"); }
 
 static int stub_set_format(uiox_spk_hw_t *hw,
                             const uiox_spk_audio_fmt_t *fmt)
 {
     (void)hw;
     printf("  [hal] format  %u Hz  %uch  %u-bit\n",
            fmt->sample_rate, fmt->channels, fmt->bit_depth);
     return 0;
 }
 
 static int stub_set_volume(uiox_spk_hw_t *hw, uint8_t vol)
 {
     (void)hw;
     printf("  [hal] volume  %u %%\n", vol);
     return 0;
 }
 
 static int stub_set_mute(uiox_spk_hw_t *hw, bool mute)
 {
     (void)hw;
     printf("  [hal] mute %s\n", mute ? "ON" : "OFF");
     return 0;
 }
 
 static int stub_dma_submit(uiox_spk_hw_t *hw,
                             uintptr_t phys, uint32_t bytes, bool last)
 {
     (void)hw; (void)phys; (void)last;
     s_dma_count++;
     printf("  [hal] DMA submit  %u bytes  (#%u)\n", bytes, s_dma_count);
     /* Simulate DMA completion immediately in demo */
     hw->dma_done = true;
     return 0;
 }
 
 static int stub_dma_flush(uiox_spk_hw_t *hw)
 { (void)hw; return 0; }
 
 static int stub_i2c_read(uiox_spk_hw_t *hw, uint8_t addr,
                           uint8_t reg, uint8_t *buf, uint16_t len)
 {
     (void)hw; (void)addr; (void)reg;
     memset(buf, 0, len);
     buf[0] = 0x56u;  /* Simulated device ID: TAS5756 */
     return 0;
 }
 
 static int stub_i2c_write(uiox_spk_hw_t *hw, uint8_t addr,
                            uint8_t reg, const uint8_t *buf, uint16_t len)
 {
     (void)hw; (void)len; (void)buf;
     printf("  [hal] I2C W  addr=0x%02X  reg=0x%02X  val=0x%02X\n",
            addr, reg, buf[0]);
     return 0;
 }
 
 static void stub_gpio_set(uiox_spk_hw_t *hw, uint8_t pin, bool val)
 {
     (void)hw;
     printf("  [hal] GPIO pin=%u  val=%d\n", pin, (int)val);
 }
 
 static void stub_isr_dma_half(uiox_spk_hw_t *hw) { (void)hw; }
 static void stub_isr_dma_done(uiox_spk_hw_t *hw)
 { if (hw) hw->dma_done = true; }
 static void stub_isr_fault   (uiox_spk_hw_t *hw) { (void)hw; }
 
 static const uiox_spk_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .start         = stub_start,
     .stop          = stub_stop,
     .set_format    = stub_set_format,
     .set_volume    = stub_set_volume,
     .set_mute      = stub_set_mute,
     .dma_submit    = stub_dma_submit,
     .dma_flush     = stub_dma_flush,
     .i2c_read      = stub_i2c_read,
     .i2c_write     = stub_i2c_write,
     .gpio_set      = stub_gpio_set,
     .isr_dma_half  = stub_isr_dma_half,
     .isr_dma_done  = stub_isr_dma_done,
     .isr_fault     = stub_isr_fault,
 };
 
 /* =========================================================================
  * Hardware device instance (I2S + TAS5756 class-D amplifier)
  * ====================================================================== */
 
 static uiox_spk_hw_t s_hw = {
     .base_addr      = 0x40015000uL,  /* SAI / I2S MMIO (STM32-style)      */
     .irq_dma        = 55,
     .irq_fault      = 56,
     .caps           = UIOX_SPK_CAP_I2S        |
                       UIOX_SPK_CAP_DMA        |
                       UIOX_SPK_CAP_HW_VOLUME  |
                       UIOX_SPK_CAP_HW_MUTE    |
                       UIOX_SPK_CAP_HW_EQ      |
                       UIOX_SPK_CAP_STEREO     |
                       UIOX_SPK_CAP_BASS_BOOST |
                       UIOX_SPK_CAP_FAULT_IRQ,
     .if_type        = UIOX_SPK_IF_I2S,
     .mclk_hz        = 12288000u,     /* 256 × 48 kHz                      */
     .mute_pin       = 4u,
     .pwr_pin        = 5u,
     .codec_i2c_addr = 0x4Cu,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_spk_event(uiox_spk_evt_t evt, uint8_t stream_id, void *ctx)
 {
     (void)ctx;
     printf("  [event] %-16s  stream=%u\n",
            uiox_spk_evt_name(evt), stream_id);
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Music Speaker Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Generate synthetic audio content                                 */
     /* ------------------------------------------------------------------ */
 
     printf("--- Generate test audio ---\n");
     gen_sine(s_sine_440, DEMO_FRAME_SMP, 440.0f, 0.6f, DEMO_SAMPLE_RATE);
     gen_sine(s_sine_660, DEMO_FRAME_SMP, 660.0f, 0.4f, DEMO_SAMPLE_RATE);
     gen_noise(s_noise,   DEMO_FRAME_SMP, 0.1f);
     printf("  440 Hz sine  %u stereo pairs\n", DEMO_FRAME_SMP);
     printf("  660 Hz sine  %u stereo pairs\n", DEMO_FRAME_SMP);
     printf("  White noise  %u stereo pairs\n", DEMO_FRAME_SMP);
 
     /* ------------------------------------------------------------------ */
     /* 2. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Open ---\n");
     uiox_spk_device_t dev;
     uiox_spk_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw              = &s_hw;
     p.hw_ops          = &stub_ops;
     p.codec_type      = UIOX_SPK_CODEC_TAS5756;
     p.codec_i2c       = s_hw.codec_i2c_addr;
     p.fmt.sample_rate = DEMO_SAMPLE_RATE;
     p.fmt.channels    = 2u;
     p.fmt.bit_depth   = 16u;
     p.fmt.is_signed   = true;
     p.fmt.big_endian  = false;
     p.evt_cb          = on_spk_event;
 
     int rc = uiox_spk_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_spk_open failed: %d\n", rc);
         return 1;
     }
     printf("  Format: %u Hz  %uch  %u-bit\n",
            p.fmt.sample_rate, p.fmt.channels, p.fmt.bit_depth);
     printf("  Codec : TAS5756  I2C=0x%02X\n", p.codec_i2c);
 
     /* ------------------------------------------------------------------ */
     /* 3. Set initial volume and EQ                                        */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Initial settings ---\n");
     uiox_spk_set_volume(&dev, 75u);
     printf("  Volume: 75 %%\n");
 
     uiox_spk_set_bass  (&dev,  6);
     printf("  Bass  : +6 dB\n");
 
     uiox_spk_set_treble(&dev,  3);
     printf("  Treble: +3 dB\n");
 
     /* Custom mid-range EQ band */
     uiox_spk_set_eq(&dev, 2, 1000.0f, -2.0f, 2.0f);
     printf("  EQ[2] : 1kHz  -2 dB  Q=2.0\n");
 
     /* ------------------------------------------------------------------ */
     /* 4. Start playback                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_spk_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_spk_state_name(uiox_spk_state(&dev)), rc);
 
     /* ------------------------------------------------------------------ */
     /* 5. Play 440 Hz tone stream                                          */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Play 440 Hz tone (loop) ---\n");
     int sid_440 = uiox_spk_play(&dev, s_sine_440, DEMO_FRAME_SMP,
                                   0.8f, true);
     printf("  Stream id=%d  440 Hz loop\n", sid_440);
 
     /* ------------------------------------------------------------------ */
     /* 6. Tick loop (5 frames)                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Tick loop (5 frames) ---\n");
     for (uint32_t t = 10; t <= 50; t += 10) {
         printf("\n  [tick t=%u ms]\n", t);
         uiox_spk_tick(&dev, t);
     }
 
     /* ------------------------------------------------------------------ */
     /* 7. Add 660 Hz harmony stream                                        */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Add 660 Hz harmony ---\n");
     int sid_660 = uiox_spk_play(&dev, s_sine_660, DEMO_FRAME_SMP,
                                   0.4f, true);
     printf("  Stream id=%d  660 Hz loop\n", sid_660);
 
     for (uint32_t t = 60; t <= 100; t += 10) {
         printf("\n  [tick t=%u ms]\n", t);
         uiox_spk_tick(&dev, t);
     }
 
     /* ------------------------------------------------------------------ */
     /* 8. Write raw PCM directly                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Write raw noise PCM ---\n");
     uint32_t written = uiox_spk_write(&dev, s_noise, DEMO_FRAME_SMP);
     printf("  Written: %u stereo pairs\n", written);
     uiox_spk_tick(&dev, 110u);
 
     /* ------------------------------------------------------------------ */
     /* 9. Volume fade-down                                                 */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Volume: 75 %% → 30 %% ---\n");
     uiox_spk_set_volume(&dev, 30u);
     for (uint32_t t = 120; t <= 160; t += 10) {
         uiox_spk_tick(&dev, t);
     }
     printf("  Volume adjusted\n");
 
     /* ------------------------------------------------------------------ */
     /* 10. Pause / resume                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Pause ---\n");
     uiox_spk_pause(&dev);
     printf("  State: %s\n", uiox_spk_state_name(uiox_spk_state(&dev)));
 
     printf("\n--- Resume ---\n");
     uiox_spk_resume(&dev);
     printf("  State: %s\n", uiox_spk_state_name(uiox_spk_state(&dev)));
     uiox_spk_tick(&dev, 170u);
 
     /* ------------------------------------------------------------------ */
     /* 11. Stop individual streams                                         */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop 440 Hz stream ---\n");
     uiox_spk_stop_stream(&dev, (uint8_t)sid_440);
     printf("  Stream %d stopped\n", sid_440);
     uiox_spk_tick(&dev, 180u);
 
     printf("\n--- Stop 660 Hz stream ---\n");
     uiox_spk_stop_stream(&dev, (uint8_t)sid_660);
     printf("  Stream %d stopped\n", sid_660);
     uiox_spk_tick(&dev, 190u);
 
     /* ------------------------------------------------------------------ */
     /* 12. Mute / unmute                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Mute ---\n");
     uiox_spk_set_mute(&dev, true);
     uiox_spk_tick(&dev, 200u);
 
     printf("\n--- Unmute ---\n");
     uiox_spk_set_mute(&dev, false);
     uiox_spk_tick(&dev, 210u);
 
     /* ------------------------------------------------------------------ */
     /* 13. Statistics                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_spk_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 14. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop ---\n");
     uiox_spk_stop(&dev);
     /* Run ticks to drain fade-out */
     for (uint32_t t = 220; t <= 250; t += 10)
         uiox_spk_tick(&dev, t);
     printf("  State: %s\n", uiox_spk_state_name(uiox_spk_state(&dev)));
 
     printf("\n--- Close ---\n");
     uiox_spk_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX Speaker Demo complete ===\n");
     return 0;
 }
 