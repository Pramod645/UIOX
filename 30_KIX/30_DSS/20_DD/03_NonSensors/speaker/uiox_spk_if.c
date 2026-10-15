/**
 * @file    uiox_spk_if.c
 * @brief   UIOX Speaker interface driver implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_if.h"
 
 int uiox_spk_if_config(uiox_spk_if_t *sif,
                         uiox_spk_hw_t *hw,
                         const uiox_spk_audio_fmt_t *fmt)
 {
     if (!sif || !hw || !fmt) return -EINVAL;
     memset(sif, 0, sizeof(*sif));
     sif->hw = hw;
 
     uiox_spk_buf_init();
     uiox_spk_ring_init(&sif->ring);
 
     int rc = uiox_spk_hw_set_fmt(hw, fmt);
     if (rc < 0) return rc;
 
     sif->stats.sample_rate = fmt->sample_rate;
     sif->stats.channels    = fmt->channels;
     sif->stats.bit_depth   = fmt->bit_depth;
     sif->primed = true;
     return 0;
 }
 
 int uiox_spk_if_start(uiox_spk_if_t *sif)
 {
     if (!sif || !sif->primed) return -EINVAL;
 
     /* Pre-fill two frames with silence */
     for (int i = 0; i < 2; i++) {
         uiox_spk_pcm_frame_t *f = uiox_spk_buf_alloc_pcm();
         if (!f) return -ENOMEM;
         memset(f->data, 0, f->capacity);
         f->valid_bytes = f->capacity;
         f->state       = UIOX_SPK_FRAME_READY;
         bool last = (i == 1);
         uiox_spk_hw_dma_submit(sif->hw, f->paddr, f->valid_bytes, last);
         if (i == 0) sif->active_frame = f;
         else        sif->next_frame   = f;
     }
 
     return uiox_spk_hw_start(sif->hw);
 }
 
 void uiox_spk_if_stop(uiox_spk_if_t *sif)
 {
     if (!sif) return;
     uiox_spk_hw_stop(sif->hw);
     if (sif->active_frame) {
         uiox_spk_buf_free(sif->active_frame);
         sif->active_frame = NULL;
     }
     if (sif->next_frame) {
         uiox_spk_buf_free(sif->next_frame);
         sif->next_frame = NULL;
     }
     uiox_spk_ring_flush(&sif->ring);
 }
 
 int uiox_spk_if_refill(uiox_spk_if_t *sif)
 {
     if (!sif) return -EINVAL;
     sif->stats.dma_callbacks++;
 
     /* Rotate: active → free, next → active */
     if (sif->active_frame) {
         sif->stats.frames_played++;
         sif->stats.bytes_played += sif->active_frame->valid_bytes;
         uiox_spk_buf_free(sif->active_frame);
     }
     sif->active_frame = sif->next_frame;
     sif->next_frame   = NULL;
 
     /* Allocate and fill next frame */
     uiox_spk_pcm_frame_t *f = uiox_spk_buf_alloc_pcm();
     if (!f) {
         sif->stats.underruns++;
         return -ENOMEM;
     }
 
     uint32_t bps        = sif->stats.bit_depth / 8u;
     uint32_t frame_smp  = UIOX_SPK_PCM_FRAME_SAMPLES;
     uint32_t frame_bytes= frame_smp * sif->stats.channels * bps;
     uint32_t n_stereo   = frame_smp;
 
     uint32_t got = uiox_spk_ring_read(&sif->ring,
                                        (int16_t *)f->data, n_stereo);
 
     if (got < n_stereo) {
         /* Underrun — fill remainder with silence */
         uint32_t done  = got * sif->stats.channels * bps;
         uint32_t remain= frame_bytes - done;
         memset(f->data + done, 0, remain);
         sif->stats.underruns++;
     }
 
     f->valid_bytes = frame_bytes;
     f->state       = UIOX_SPK_FRAME_READY;
     sif->next_frame = f;
 
     uiox_spk_hw_dma_submit(sif->hw, f->paddr, f->valid_bytes, false);
     return 0;
 }
 
 uint32_t uiox_spk_if_write(uiox_spk_if_t *sif,
                              const int16_t *samples, uint32_t n_stereo)
 {
     if (!sif || !samples) return 0;
     return uiox_spk_ring_write(&sif->ring, samples, n_stereo);
 }
 
 void uiox_spk_if_stats_get(const uiox_spk_if_t *sif,
                              uiox_spk_if_stats_t *out)
 {
     if (!sif || !out) return;
     memcpy(out, &sif->stats, sizeof(*out));
 }
 
 void uiox_spk_if_stats_reset(uiox_spk_if_t *sif)
 {
     if (!sif) return;
     memset(&sif->stats, 0, sizeof(sif->stats));
 }
 