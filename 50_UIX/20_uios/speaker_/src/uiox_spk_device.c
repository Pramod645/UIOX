/**
 * @file    uiox_spk_device.c
 * @brief   UIOX Speaker device API implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_spk_open(uiox_spk_device_t *dev, const uiox_spk_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_spk_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_spk_subsys_init(&dev->subsys, p->hw,
                                p->codec_type, p->codec_i2c, &p->fmt);
     if (rc < 0) return rc;
 
     if (p->evt_cb)
         uiox_spk_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_spk_start(uiox_spk_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_spk_subsys_start(&dev->subsys);
 }
 
 void uiox_spk_stop(uiox_spk_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_spk_subsys_stop(&dev->subsys);
 }
 
 void uiox_spk_pause(uiox_spk_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_spk_subsys_pause(&dev->subsys);
 }
 
 void uiox_spk_resume(uiox_spk_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_spk_subsys_resume(&dev->subsys);
 }
 
 void uiox_spk_close(uiox_spk_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_spk_if_stop(&dev->subsys.sif);
     uiox_spk_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 void uiox_spk_tick(uiox_spk_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_spk_subsys_tick(&dev->subsys, now_ms);
 }
 
 int uiox_spk_play(uiox_spk_device_t *dev, const int16_t *data,
                    uint32_t n_stereo, float gain, bool loop)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_spk_subsys_add_stream(&dev->subsys, data, n_stereo, gain, loop);
 }
 
 void uiox_spk_stop_stream(uiox_spk_device_t *dev, uint8_t id)
 {
     if (!dev || !dev->open) return;
     uiox_spk_subsys_stop_stream(&dev->subsys, id);
 }
 
 uint32_t uiox_spk_write(uiox_spk_device_t *dev,
    const int16_t *samples, uint32_t n_stereo)
{
if (!dev || !dev->open) return 0;
return uiox_spk_subsys_write(&dev->subsys, samples, n_stereo);
}

int uiox_spk_set_volume(uiox_spk_device_t *dev, uint8_t vol_pct)
{
if (!dev || !dev->open) return -EINVAL;
return uiox_spk_subsys_set_vol(&dev->subsys, vol_pct);
}

int uiox_spk_set_mute(uiox_spk_device_t *dev, bool mute)
{
if (!dev || !dev->open) return -EINVAL;
return uiox_spk_subsys_mute(&dev->subsys, mute);
}

void uiox_spk_set_eq(uiox_spk_device_t *dev, uint8_t band,
 float hz, float gain_db, float q)
{
if (!dev || !dev->open) return;
uiox_spk_subsys_set_eq(&dev->subsys, band, hz, gain_db, q);
}

void uiox_spk_set_bass(uiox_spk_device_t *dev, int8_t db)
{
if (!dev || !dev->open) return;
uiox_spk_subsys_set_eq(&dev->subsys, 0,
       dev->subsys.dsp.bands[0].center_hz,
       (float)db, 1.0f);
uiox_spk_codec_set_bass(&dev->subsys.codec, db);
}

void uiox_spk_set_treble(uiox_spk_device_t *dev, int8_t db)
{
if (!dev || !dev->open) return;
uiox_spk_subsys_set_eq(&dev->subsys,
       UIOX_SPK_DSP_EQ_BANDS - 1u,
       dev->subsys.dsp.bands[UIOX_SPK_DSP_EQ_BANDS-1].center_hz,
       (float)db, 1.0f);
uiox_spk_codec_set_treble(&dev->subsys.codec, db);
}

uiox_spk_state_t uiox_spk_state(const uiox_spk_device_t *dev)
{
if (!dev || !dev->open) return UIOX_SPK_STATE_STOPPED;
return dev->subsys.state;
}

void uiox_spk_print_stats(const uiox_spk_device_t *dev)
{
if (!dev) return;
const uiox_spk_subsys_t *s = &dev->subsys;
printf("  State          : %s\n",  uiox_spk_state_name(s->state));
printf("  Master volume  : %u %%\n", s->master_vol);
printf("  Total frames   : %llu\n",
(unsigned long long)s->total_frames);
printf("  Total underruns: %llu\n",
(unsigned long long)s->total_underruns);
printf("  Ring avail     : %u / %u stereo pairs\n",
uiox_spk_ring_avail(&s->sif.ring),
UIOX_SPK_RING_SAMPLES);
printf("  PCM pool free  : %u / %u\n",
uiox_spk_buf_pcm_free(), UIOX_SPK_PCM_POOL_SIZE);
uiox_spk_if_stats_t is;
uiox_spk_if_stats_get(&dev->subsys.sif, &is);
printf("  Frames played  : %llu\n",
(unsigned long long)is.frames_played);
printf("  Bytes played   : %llu\n",
(unsigned long long)is.bytes_played);
printf("  IF underruns   : %llu\n",
(unsigned long long)is.underruns);
printf("  DMA callbacks  : %llu\n",
(unsigned long long)is.dma_callbacks);
printf("  Ring overflow  : %u\n",   s->sif.ring.overflow);
printf("  Ring underrun  : %u\n",   s->sif.ring.underrun);
printf("  EQ enabled     : %s\n",
s->dsp.eq_enabled ? "yes" : "no");
}

const char *uiox_spk_state_name(uiox_spk_state_t s)
{
switch (s) {
case UIOX_SPK_STATE_STOPPED:  return "STOPPED";
case UIOX_SPK_STATE_PLAYING:  return "PLAYING";
case UIOX_SPK_STATE_PAUSED:   return "PAUSED";
case UIOX_SPK_STATE_STOPPING: return "STOPPING";
default:                       return "UNKNOWN";
}
}

const char *uiox_spk_evt_name(uiox_spk_evt_t evt)
{
switch (evt) {
case UIOX_SPK_EVT_START:         return "START";
case UIOX_SPK_EVT_STOP:          return "STOP";
case UIOX_SPK_EVT_UNDERRUN:      return "UNDERRUN";
case UIOX_SPK_EVT_STREAM_END:    return "STREAM_END";
case UIOX_SPK_EVT_VOLUME_CHANGE: return "VOLUME_CHANGE";
case UIOX_SPK_EVT_FAULT:         return "FAULT";
default:                          return "UNKNOWN";
}
}
