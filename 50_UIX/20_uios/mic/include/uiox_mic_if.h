/**
 * @file    uiox_mic_if.h
 * @brief   UIOX Microphone interface driver (DMA capture path).
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_IF_H
 #define UIOX_MIC_IF_H
 
 #include "uiox_mic_hw.h"
 #include "uiox_mic_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  frames_captured;
     uint64_t  bytes_captured;
     uint64_t  overruns;
     uint64_t  dma_callbacks;
     uint32_t  sample_rate;
     uint8_t   channels;
     uint8_t   bit_depth;
 } uiox_mic_if_stats_t;
 
 typedef struct {
     uiox_mic_hw_t        *hw;
     uiox_mic_ring_t       ring;
     uiox_mic_frame_t     *active_frame;
     uiox_mic_frame_t     *next_frame;
     uiox_mic_if_stats_t   stats;
     bool                  primed;
 } uiox_mic_if_t;
 
 int      uiox_mic_if_config  (uiox_mic_if_t *mif, uiox_mic_hw_t *hw,
                                const uiox_mic_audio_fmt_t *fmt);
 int      uiox_mic_if_start   (uiox_mic_if_t *mif);
 void     uiox_mic_if_stop    (uiox_mic_if_t *mif);
 
 /**
  * @brief  DMA completion handler — rotate buffers and re-prime.
  *         Call from DMA half/done ISR bottom-half.
  */
 int      uiox_mic_if_refill  (uiox_mic_if_t *mif);
 
 /** Read captured mono int16 samples from ring buffer. */
 uint32_t uiox_mic_if_read    (uiox_mic_if_t *mif,
                                int16_t *samples, uint32_t n);
 
 void uiox_mic_if_stats_get   (const uiox_mic_if_t *mif,
                                uiox_mic_if_stats_t *out);
 void uiox_mic_if_stats_reset (uiox_mic_if_t *mif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_IF_H */
 