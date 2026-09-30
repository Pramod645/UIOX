/**
 * @file    uiox_mic_subsys.c
 * @brief   UIOX Microphone subsystem implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_mic_subsys_t *sys, uiox_mic_evt_t evt)
 { if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx); }
 
 int uiox_mic_subsys_init(uiox_mic_subsys_t          *sys,
                           uiox_mic_hw_t              *hw,
                           uiox_mic_codec_type_t       codec_type,
                           uint8_t                     codec_i2c,
                           const uiox_mic_audio_fmt_t *fmt,
                           const uiox_mic_dsp_cfg_t   *dsp_cfg)
 {
     if (!sys || !hw || !fmt || !dsp_cfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_mic_if_config(&sys->mif, hw, fmt);
     if (rc < 0) return rc;
 
     rc = uiox_mic_codec_init(&sys->codec, hw, codec_type, codec_i2c);
     if (rc < 0) return rc;
 
     uiox_mic_codec_set_fmt(&sys->codec, fmt);
 
     rc = uiox_mic_dsp_init(&sys->dsp, dsp_cfg);
     if (rc < 0) return rc;
 
     sys->state    = UIOX_MIC_STATE_STOPPED;
     sys->last_vad = false;
     return 0;
 }
 
 int uiox_mic_subsys_start(uiox_mic_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_mic_if_start(&sys->mif);
     if (rc == 0) {
         sys->state = UIOX_MIC_STATE_RUNNING;
         fire(sys, UIOX_MIC_EVT_STARTED);
     }
     return rc;
 }
 
 void uiox_mic_subsys_stop(uiox_mic_subsys_t *sys)
 {
     if (!sys) return;
     uiox_mic_if_stop(&sys->mif);
     sys->state = UIOX_MIC_STATE_STOPPED;
     fire(sys, UIOX_MIC_EVT_STOPPED);
 }
 
 void uiox_mic_subsys_tick(uiox_mic_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_MIC_STATE_RUNNING) return;
     (void)now_ms;
     sys->tick_count++;
 
     /* Trigger DMA refill if needed */
     if (sys->mif.hw->dma_done || sys->mif.hw->dma_half) {
         sys->mif.hw->dma_done = false;
         sys->mif.hw->dma_half = false;
         int rc = uiox_mic_if_refill(&sys->mif);
         if (rc < 0) {
             sys->total_overruns++;
             fire(sys, UIOX_MIC_EVT_OVERRUN);
         }
     }
     sys->total_frames++;
 
     /* VAD transition events */
     const uiox_mic_vad_t *vad = uiox_mic_dsp_vad(&sys->dsp);
     if (vad) {
         if (vad->voice_active && !sys->last_vad)
             fire(sys, UIOX_MIC_EVT_VOICE_START);
         else if (!vad->voice_active && sys->last_vad)
             fire(sys, UIOX_MIC_EVT_VOICE_END);
         sys->last_vad = vad->voice_active;
     }
 }
 
 int uiox_mic_subsys_set_gain(uiox_mic_subsys_t *sys, uint8_t gain_db)
 {
     if (!sys) return -EINVAL;
     uiox_mic_codec_set_gain(&sys->codec, gain_db);
     int rc = uiox_mic_hw_set_gain(sys->mif.hw, gain_db);
     if (rc == 0) fire(sys, UIOX_MIC_EVT_GAIN_CHANGE);
     return rc;
 }
 
 int uiox_mic_subsys_set_mute(uiox_mic_subsys_t *sys, bool mute)
 {
     if (!sys) return -EINVAL;
     uiox_mic_codec_set_mute(&sys->codec, mute);
     int rc = uiox_mic_hw_set_mute(sys->mif.hw, mute);
     if (rc == 0) sys->state = mute ? UIOX_MIC_STATE_MUTED
                                    : UIOX_MIC_STATE_RUNNING;
     return rc;
 }
 
 uint32_t uiox_mic_subsys_read(uiox_mic_subsys_t *sys,
                                int16_t *buf, uint32_t n)
 {
     if (!sys || !buf || n == 0) return 0;
     uint32_t got = uiox_mic_if_read(&sys->mif, buf, n);
     if (got > 0)
         uiox_mic_dsp_process(&sys->dsp, buf, got);
     return got;
 }
 
 void uiox_mic_subsys_set_cb(uiox_mic_subsys_t *sys,
                               uiox_mic_evt_cb_t cb, void *ctx)
 { if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
 