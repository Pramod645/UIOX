/**
 * @file    uiox_spk_if.h
 * @brief   UIOX Speaker interface driver (DMA audio path).
 *
 * Manages:
 *   - DMA double-buffer submission loop
 *   - Underrun detection and silence fill
 *   - Sample rate / format programming
 *   - Interface statistics (underruns, frames played)
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_IF_H
 #define UIOX_SPK_IF_H
 
 #include "uiox_spk_hw.h"
 #include "uiox_spk_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  frames_played;
     uint64_t  bytes_played;
     uint64_t  underruns;
     uint64_t  dma_callbacks;
     uint32_t  sample_rate;
     uint8_t   channels;
     uint8_t   bit_depth;
 } uiox_spk_if_stats_t;
 
 typedef struct {
     uiox_spk_hw_t        *hw;
     uiox_spk_ring_t       ring;
     uiox_spk_pcm_frame_t *active_frame;
     uiox_spk_pcm_frame_t *next_frame;
     uiox_spk_if_stats_t   stats;
     bool                  primed;
 } uiox_spk_if_t;
 
 int  uiox_spk_if_config   (uiox_spk_if_t *sif,
                             uiox_spk_hw_t *hw,
                             const uiox_spk_audio_fmt_t *fmt);
 int  uiox_spk_if_start    (uiox_spk_if_t *sif);
 void uiox_spk_if_stop     (uiox_spk_if_t *sif);
 
 /**
  * @brief  Refill DMA buffer from ring — call from DMA half/done ISR
  *         bottom-half or from the audio task.
  */
 int  uiox_spk_if_refill   (uiox_spk_if_t *sif);
 
 /** Write stereo int16 PCM samples into the ring buffer. */
 uint32_t uiox_spk_if_write(uiox_spk_if_t *sif,
                             const int16_t *samples, uint32_t n_stereo);
 
 void uiox_spk_if_stats_get  (const uiox_spk_if_t *sif,
                               uiox_spk_if_stats_t *out);
 void uiox_spk_if_stats_reset(uiox_spk_if_t *sif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_IF_H */
 