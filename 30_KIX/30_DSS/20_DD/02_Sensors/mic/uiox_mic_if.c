/**
 * @file    uiox_mic_if.c
 * @brief   UIOX Microphone interface driver implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_if.h"
 
 int uiox_mic_if_config(uiox_mic_if_t *mif, uiox_mic_hw_t *hw,
                         const uiox_mic_audio_fmt_t *fmt)
 {
     if (!mif || !hw || !fmt) return -EINVAL;
     memset(mif, 0, sizeof(*mif));
     mif->hw = hw;
     uiox_mic_buf_init();
     uiox_mic_ring_init(&mif->ring);
     int rc = uiox_mic_hw_set_fmt(hw, fmt);
     if (rc < 0) return rc;
     mif->stats.sample_rate = fmt->sample_rate;
     mif->stats.channels    = fmt->channels;
     mif->stats.bit_depth   = fmt->bit_depth;
     mif->primed = true;
     return 0;
 }
 
 int uiox_mic_if_start(uiox_mic_if_t *mif)
 {
     if (!mif || !mif->primed) return -EINVAL;
     /* Prime two capture frames */
     for (int i = 0; i < 2; i++) {
         uiox_mic_frame_t *f = uiox_mic_buf_alloc_cap();
         if (!f) return -ENOMEM;
         f->channels    = mif->stats.channels;
         f->bit_depth   = mif->stats.bit_depth;
         f->num_samples = UIOX_MIC_FRAME_SAMPLES;
         f->valid_bytes = f->num_samples * f->channels * (f->bit_depth / 8u);
         uiox_mic_hw_dma_submit(mif->hw, f->paddr, f->capacity, i == 1);
         if (i == 0) mif->active_frame = f;
         else        mif->next_frame   = f;
     }
     return uiox_mic_hw_start(mif->hw);
 }
 
 void uiox_mic_if_stop(uiox_mic_if_t *mif)
 {
     if (!mif) return;
     uiox_mic_hw_stop(mif->hw);
     if (mif->active_frame) { uiox_mic_buf_free(mif->active_frame); mif->active_frame = NULL; }
     if (mif->next_frame)   { uiox_mic_buf_free(mif->next_frame);   mif->next_frame   = NULL; }
     uiox_mic_ring_flush(&mif->ring);
 }
 
 int uiox_mic_if_refill(uiox_mic_if_t *mif)
 {
     if (!mif) return -EINVAL;
     mif->stats.dma_callbacks++;
 
     /* Completed frame → push to ring buffer */
     if (mif->active_frame) {
         mif->stats.frames_captured++;
         mif->stats.bytes_captured += mif->active_frame->valid_bytes;
         /* Convert captured bytes to mono int16 samples for ring */
         uint32_t n_smp = mif->active_frame->valid_bytes /
                          (mif->stats.channels * (mif->stats.bit_depth / 8u));
         const int16_t *src = (const int16_t *)mif->active_frame->data;
         uint32_t ch = mif->stats.channels;
         /* Down-mix to mono if multi-channel */
         for (uint32_t i = 0; i < n_smp; i++) {
             int32_t acc = 0;
             for (uint32_t c = 0; c < ch; c++) acc += src[i * ch + c];
             int16_t mono = (int16_t)(acc / (int32_t)ch);
             uiox_mic_ring_write(&mif->ring, &mono, 1u);
         }
         mif->active_frame->state = UIOX_MIC_FRAME_READY;
         uiox_mic_buf_free(mif->active_frame);
     }
 
     mif->active_frame = mif->next_frame;
     mif->next_frame   = NULL;
 
     /* Allocate and re-prime next buffer */
     uiox_mic_frame_t *f = uiox_mic_buf_alloc_cap();
     if (!f) { mif->stats.overruns++; return -ENOMEM; }
     f->channels    = mif->stats.channels;
     f->bit_depth   = mif->stats.bit_depth;
     f->num_samples = UIOX_MIC_FRAME_SAMPLES;
     f->valid_bytes = f->num_samples * f->channels * (f->bit_depth / 8u);
     mif->next_frame = f;
     uiox_mic_hw_dma_submit(mif->hw, f->paddr, f->capacity, false);
     return 0;
 }
 
 uint32_t uiox_mic_if_read(uiox_mic_if_t *mif,
                            int16_t *samples, uint32_t n)
 {
     if (!mif || !samples) return 0;
     return uiox_mic_ring_read(&mif->ring, samples, n);
 }
 
 void uiox_mic_if_stats_get(const uiox_mic_if_t *mif,
                             uiox_mic_if_stats_t *out)
 { if (!mif || !out) return; memcpy(out, &mif->stats, sizeof(*out)); }
 
 void uiox_mic_if_stats_reset(uiox_mic_if_t *mif)
 { if (!mif) return; memset(&mif->stats, 0, sizeof(mif->stats)); }
 