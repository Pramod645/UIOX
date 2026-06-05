/**
 * @file    uiox_mic_buf.h
 * @brief   UIOX Microphone PCM capture buffer pool and ring buffer.
 *
 * Two pools:
 *   CAPTURE pool — DMA-ready PCM frames (double/triple buffered)
 *   PROC pool    — processing scratch buffers for DSP pipeline
 *
 * SPSC lock-free ring for streaming captured samples to the application.
 *
 * @date    2026-06-03
 */

 #ifndef UIOX_MIC_BUF_H
 #define UIOX_MIC_BUF_H
 
 #include "uiox_mic_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PCM capture frame pool
  * 16kHz mono 16-bit: 16000 × 2B = 32 KB/s
  * 10ms frame = 320 bytes
  * ====================================================================== */
 
 #define UIOX_MIC_CAP_POOL_SIZE      4
 #define UIOX_MIC_PROC_POOL_SIZE     2
 #define UIOX_MIC_FRAME_SAMPLES      160     /**< 10ms @ 16kHz            */
 #define UIOX_MIC_MAX_CHANNELS       8
 #define UIOX_MIC_FRAME_MAX_BYTES    (UIOX_MIC_FRAME_SAMPLES * \
                                      UIOX_MIC_MAX_CHANNELS * 4)
 #define UIOX_MIC_BUF_ALIGN          64
 
 /* =========================================================================
  * Capture frame descriptor
  * ====================================================================== */
 
 typedef enum {
     UIOX_MIC_FRAME_FREE = 0,
     UIOX_MIC_FRAME_CAPTURING,
     UIOX_MIC_FRAME_READY,
     UIOX_MIC_FRAME_PROCESSING,
 } uiox_mic_frame_state_t;
 
 typedef struct uiox_mic_frame {
     uint8_t    *data;
     uintptr_t   paddr;
     uint32_t    capacity;
     uint32_t    valid_bytes;
     uint16_t    num_samples;
     uint8_t     channels;
     uint8_t     bit_depth;
     uint32_t    frame_id;
     uint64_t    ts_ns;
     uiox_mic_frame_state_t state;
     uint8_t     in_use;
     struct uiox_mic_frame *next;
 } uiox_mic_frame_t;
 
 /* =========================================================================
  * Sample ring buffer (SPSC lock-free, mono int16)
  * ====================================================================== */
 
 #define UIOX_MIC_RING_SAMPLES       8192u   /**< Power of 2              */
 #define UIOX_MIC_RING_MASK          (UIOX_MIC_RING_SAMPLES - 1u)
 
 typedef struct {
     int16_t          buf[UIOX_MIC_RING_SAMPLES];
     volatile uint32_t head;
     volatile uint32_t tail;
     uint32_t          overflow;
     uint32_t          underrun;
 } uiox_mic_ring_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_mic_buf_init       (void);
 uiox_mic_frame_t  *uiox_mic_buf_alloc_cap  (void);
 uiox_mic_frame_t  *uiox_mic_buf_alloc_proc (void);
 void               uiox_mic_buf_free       (uiox_mic_frame_t *f);
 uint8_t            uiox_mic_buf_cap_free   (void);
 
 /* Ring buffer */
 void     uiox_mic_ring_init  (uiox_mic_ring_t *r);
 uint32_t uiox_mic_ring_write (uiox_mic_ring_t *r,
                                const int16_t *samples, uint32_t n);
 uint32_t uiox_mic_ring_read  (uiox_mic_ring_t *r,
                                int16_t *samples, uint32_t n);
 uint32_t uiox_mic_ring_avail (const uiox_mic_ring_t *r);
 uint32_t uiox_mic_ring_space (const uiox_mic_ring_t *r);
 void     uiox_mic_ring_flush (uiox_mic_ring_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MIC_BUF_H */
 