/**
 * @file    uiox_spk_subsys.c
 * @brief   UIOX Speaker subsystem implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_spk_subsys_t *sys, uiox_spk_evt_t evt, uint8_t sid)
 {
     if (sys->evt_cb) sys->evt_cb(evt, sid, sys->evt_ctx);
 }
 
 int uiox_spk_subsys_init(uiox_spk_subsys_t          *sys,
                           uiox_spk_hw_t              *hw,
                           uiox_spk_codec_type_t       codec_type,
                           uint8_t                     codec_i2c,
                           const uiox_spk_audio_fmt_t *fmt)
 {
     if (!sys || !hw || !fmt) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     int rc = uiox_spk_if_config(&sys->sif, hw, fmt);
     if (rc < 0) return rc;
 
     rc = uiox_spk_codec_init(&sys->codec, hw, codec_type, codec_i2c);
     if (rc < 0) return rc;
 
     rc = uiox_spk_dsp_init(&sys->dsp, fmt->sample_rate, fmt->channels);
     if (rc < 0) return rc;
 
     uiox_spk_codec_set_fmt(&sys->codec, fmt);
     sys->master_vol = 80u;
     sys->state      = UIOX_SPK_STATE_STOPPED;
     return 0;
 }
 
 int uiox_spk_subsys_start(uiox_spk_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     /* Fade in to avoid pop */
     sys->dsp.fade_gain = 0.0f;
     uiox_spk_dsp_fade(&sys->dsp, 1.0f, 20u);
     int rc = uiox_spk_if_start(&sys->sif);
     if (rc == 0) {
         sys->state = UIOX_SPK_STATE_PLAYING;
         fire(sys, UIOX_SPK_EVT_START, 0u);
     }
     return rc;
 }
 
 void uiox_spk_subsys_stop(uiox_spk_subsys_t *sys)
 {
     if (!sys) return;
     /* Fade out to avoid pop */
     uiox_spk_dsp_fade(&sys->dsp, 0.0f, 20u);
     sys->state = UIOX_SPK_STATE_STOPPING;
 }
 
 void uiox_spk_subsys_pause(uiox_spk_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_SPK_STATE_PLAYING) return;
     uiox_spk_hw_set_mute(sys->sif.hw, true);
     sys->state = UIOX_SPK_STATE_PAUSED;
 }
 
 void uiox_spk_subsys_resume(uiox_spk_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_SPK_STATE_PAUSED) return;
     uiox_spk_hw_set_mute(sys->sif.hw, false);
     sys->state = UIOX_SPK_STATE_PLAYING;
 }
 
 int uiox_spk_subsys_add_stream(uiox_spk_subsys_t *sys,
                                 const int16_t *data,
                                 uint32_t n_stereo,
                                 float gain,
                                 bool loop)
 {
     if (!sys || !data) return -EINVAL;
     for (uint8_t i = 0; i < UIOX_SPK_MAX_STREAMS; i++) {
         if (!sys->streams[i].active) {
             sys->streams[i].data   = data;
             sys->streams[i].total  = n_stereo;
             sys->streams[i].pos    = 0;
             sys->streams[i].gain   = gain;
             sys->streams[i].loop   = loop;
             sys->streams[i].active = true;
             sys->streams[i].id     = i;
             return (int)i;
         }
     }
     return -ENOSPC;
 }
 
 void uiox_spk_subsys_stop_stream(uiox_spk_subsys_t *sys, uint8_t id)
 {
     if (!sys || id >= UIOX_SPK_MAX_STREAMS) return;
     sys->streams[id].active = false;
 }
 
 uint32_t uiox_spk_subsys_write(uiox_spk_subsys_t *sys,
                                 const int16_t *samples,
                                 uint32_t n_stereo)
 {
     if (!sys || !samples) return 0;
     return uiox_spk_if_write(&sys->sif, samples, n_stereo);
 }
 
 int uiox_spk_subsys_set_vol(uiox_spk_subsys_t *sys, uint8_t vol_pct)
 {
     if (!sys) return -EINVAL;
     sys->master_vol = vol_pct;
     float gain = (float)vol_pct / 100.0f;
     uiox_spk_dsp_set_volume(&sys->dsp, gain);
     uiox_spk_codec_set_vol(&sys->codec, vol_pct);
     uiox_spk_hw_set_volume(sys->sif.hw, vol_pct);
     fire(sys, UIOX_SPK_EVT_VOLUME_CHANGE, 0u);
     return 0;
 }
 
 int uiox_spk_subsys_mute(uiox_spk_subsys_t *sys, bool mute)
 {
     if (!sys) return -EINVAL;
     if (mute) {
         uiox_spk_dsp_fade(&sys->dsp, 0.0f, 10u);
     } else {
         float gain = (float)sys->master_vol / 100.0f;
         uiox_spk_dsp_fade(&sys->dsp, gain, 10u);
     }
     return uiox_spk_hw_set_mute(sys->sif.hw, mute);
 }
 
 void uiox_spk_subsys_set_eq(uiox_spk_subsys_t *sys, uint8_t band,
                               float hz, float gain_db, float q)
 {
     if (!sys) return;
     uiox_spk_dsp_set_eq_band(&sys->dsp, band, hz, gain_db, q);
     sys->dsp.eq_enabled = true;
     /* Also push to hardware codec if supported */
     if (band == 0) uiox_spk_codec_set_bass(&sys->codec, (int8_t)gain_db);
     if (band == UIOX_SPK_DSP_EQ_BANDS - 1)
         uiox_spk_codec_set_treble(&sys->codec, (int8_t)gain_db);
 }
 
 void uiox_spk_subsys_tick(uiox_spk_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_SPK_STATE_STOPPED) return;
     (void)now_ms;
 
     /* Mix all active streams into mix_buf */
     uint32_t n = UIOX_SPK_PCM_FRAME_SAMPLES;
     memset(sys->mix_buf, 0, n * 2 * sizeof(int16_t));
 
     bool any_active = false;
     for (uint8_t s = 0; s < UIOX_SPK_MAX_STREAMS; s++) {
         uiox_spk_stream_t *st = &sys->streams[s];
         if (!st->active) continue;
         any_active = true;
 
         for (uint32_t i = 0; i < n; i++) {
             if (st->pos >= st->total) {
                 if (st->loop) st->pos = 0;
                 else { st->active = false; fire(sys, UIOX_SPK_EVT_STREAM_END, s); break; }
             }
             int32_t l = sys->mix_buf[i*2]   + (int32_t)((float)st->data[st->pos*2]   * st->gain);
             int32_t r = sys->mix_buf[i*2+1] + (int32_t)((float)st->data[st->pos*2+1] * st->gain);
             if (l >  32767) l =  32767;
             if (l < -32768) l = -32768;
             if (r >  32767) r =  32767;
             if (r < -32768) r = -32768;
             sys->mix_buf[i*2]   = (int16_t)l;
             sys->mix_buf[i*2+1] = (int16_t)r;
             st->pos++;
         }
     }
 
     /* Run DSP on mix buffer */
     uiox_spk_dsp_process(&sys->dsp, sys->mix_buf, n);
 
     /* Write to ring buffer */
     uiox_spk_if_write(&sys->sif, sys->mix_buf, n);
 
     /* DMA refill if needed */
     if (sys->sif.hw->dma_done || sys->sif.hw->dma_half) {
         sys->sif.hw->dma_done = false;
         sys->sif.hw->dma_half = false;
         int rc = uiox_spk_if_refill(&sys->sif);
         if (rc < 0) {
             sys->total_underruns++;
             fire(sys, UIOX_SPK_EVT_UNDERRUN, 0u);
         }
     }
     sys->total_frames++;
 
     /* Handle fade-out → stop */
     if (sys->state == UIOX_SPK_STATE_STOPPING && !sys->dsp.fading) {
         uiox_spk_if_stop(&sys->sif);
         sys->state = UIOX_SPK_STATE_STOPPED;
         fire(sys, UIOX_SPK_EVT_STOP, 0u);
     }
 }
 
 void uiox_spk_subsys_set_cb(uiox_spk_subsys_t *sys,
                               uiox_spk_evt_cb_t cb, void *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 