/**
 * @file    uiox_mic_device.c
 * @brief   UIOX Microphone device API implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_mic_open(uiox_mic_device_t *dev, const uiox_mic_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc = uiox_mic_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_mic_subsys_init(&dev->subsys, p->hw,
                                p->codec_type, p->codec_i2c,
                                &p->fmt, &p->dsp);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_mic_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int uiox_mic_start(uiox_mic_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mic_subsys_start(&dev->subsys);
 }
 
 void uiox_mic_stop(uiox_mic_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mic_subsys_stop(&dev->subsys);
 }
 
 void uiox_mic_close(uiox_mic_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_mic_stop(dev);
     uiox_mic_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_mic_tick(uiox_mic_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_mic_subsys_tick(&dev->subsys, now_ms);
 }
 
 uint32_t uiox_mic_read(uiox_mic_device_t *dev,
                         int16_t *buf, uint32_t n_samples)
 {
     if (!dev || !dev->open) return 0;
     return uiox_mic_subsys_read(&dev->subsys, buf, n_samples);
 }
 
 int uiox_mic_set_gain(uiox_mic_device_t *dev, uint8_t gain_db)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mic_subsys_set_gain(&dev->subsys, gain_db);
 }
 
 int uiox_mic_set_mute(uiox_mic_device_t *dev, bool mute)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_mic_subsys_set_mute(&dev->subsys, mute);
 }
 
 bool uiox_mic_voice_active(const uiox_mic_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     const uiox_mic_vad_t *v = uiox_mic_dsp_vad(&dev->subsys.dsp);
     return v ? v->voice_active : false;
 }
 
 float uiox_mic_energy_dbfs(const uiox_mic_device_t *dev)
 {
     if (!dev || !dev->open) return -120.0f;
     const uiox_mic_vad_t *v = uiox_mic_dsp_vad(&dev->subsys.dsp);
     return v ? v->energy_dbfs : -120.0f;
 }
 
 void uiox_mic_print_stats(const uiox_mic_device_t *dev)
 {
     if (!dev) return;
     const uiox_mic_subsys_t *s = &dev->subsys;
     printf("  State          : %s\n",  uiox_mic_state_name(s->state));
     printf("  Tick count     : %u\n",  s->tick_count);
     printf("  Total frames   : %llu\n",(unsigned long long)s->total_frames);
     printf("  Total overruns : %llu\n",(unsigned long long)s->total_overruns);
     printf("  Ring avail     : %u\n",
            uiox_mic_ring_avail(&s->mif.ring));
     printf("  CAP pool free  : %u / %u\n",
            uiox_mic_buf_cap_free(), UIOX_MIC_CAP_POOL_SIZE);
     printf("  AGC gain       : %.2f (%.1f dB)\n",
            s->dsp.agc_gain,
            20.0f * log10f(s->dsp.agc_gain > 0.0f ? s->dsp.agc_gain : 0.001f));
     const uiox_mic_vad_t *v = uiox_mic_dsp_vad(&s->dsp);
     if (v) {
         printf("  VAD active     : %s  energy=%.1f dBFS\n",
                v->voice_active ? "YES" : "NO", v->energy_dbfs);
         printf("  VAD frames     : active=%u  silent=%u\n",
                v->active_frames, v->silent_frames);
     }
     uiox_mic_if_stats_t is;
     uiox_mic_if_stats_get(&s->mif, &is);
     printf("  Frames captured: %llu\n",(unsigned long long)is.frames_captured);
     printf("  Bytes captured : %llu\n",(unsigned long long)is.bytes_captured);
     printf("  DMA callbacks  : %llu\n",(unsigned long long)is.dma_callbacks);
     printf("  Ring overflow  : %u\n",   s->mif.ring.overflow);
 }
 
 const char *uiox_mic_state_name(uiox_mic_state_t s)
 {
     switch (s) {
     case UIOX_MIC_STATE_STOPPED: return "STOPPED";
     case UIOX_MIC_STATE_RUNNING: return "RUNNING";
     case UIOX_MIC_STATE_MUTED:   return "MUTED";
     case UIOX_MIC_STATE_ERROR:   return "ERROR";
     default:                      return "UNKNOWN";
     }
 }
 
 const char *uiox_mic_evt_name(uiox_mic_evt_t evt)
 {
     switch (evt) {
     case UIOX_MIC_EVT_STARTED:     return "STARTED";
     case UIOX_MIC_EVT_STOPPED:     return "STOPPED";
     case UIOX_MIC_EVT_VOICE_START: return "VOICE_START";
     case UIOX_MIC_EVT_VOICE_END:   return "VOICE_END";
     case UIOX_MIC_EVT_OVERRUN:     return "OVERRUN";
     case UIOX_MIC_EVT_GAIN_CHANGE: return "GAIN_CHANGE";
     default:                        return "UNKNOWN";
     }
 }
 