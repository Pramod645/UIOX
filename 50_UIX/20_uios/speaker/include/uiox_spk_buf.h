/**
 * @file    uiox_spk_buf.h
 * @brief   UIOX Speaker PCM audio buffer pool and ring buffer.
 *
 * Two pools:
 *   PCM pool  — DMA-ready audio frames (double/triple buffered)
 *   MIX pool  — intermediate mixing scratch buffers
 *
 * The ring buffer provides lock-free SPSC audio sample streaming
 * from the application fill task to the DMA ISR refill task.
 *
 * @date    2026-06-01
 */

 #ifndef UIOX_SPK_BUF_H
 #define UIOX_SPK_BUF_H
 
 #include "uiox_spk_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PCM frame pool
  * 48kHz stereo 16-bit: 48000 × 2ch × 2B = 192 KB/s
  * 10ms frame = 1920 bytes; pool of 4 = 40ms latency budget
  * ====================================================================== */
 
 #define UIOX_SPK_PCM_POOL_SIZE    4
 #define UIOX_SPK_PCM_FRAME_SAMPLES 480    /**< 10 ms @ 48 kHz            */
 #define UIOX_SPK_PCM_MAX_BYTES    (UIOX_SPK_PCM_FRAME_SAMPLES * 2 * 4)
                                           /**< 480 × 2ch × 4B (32-bit)   */
 #define UIOX_SPK_MIX_POOL_SIZE    2
 #define UIOX_SPK_BUF_ALIGN        64
 
 /* =========================================================================
  * PCM frame descriptor
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPK_FRAME_FREE = 0,
     UIOX_SPK_FRAME_FILLING,
     UIOX_SPK_FRAME_READY,
     UIOX_SPK_FRAME_PLAYING,
 } uiox_spk_frame_state_t;
 
 typedef struct uiox_spk_pcm_frame {
     uint8_t    *data;            /**< CPU virtual address                  */
     uintptr_t   paddr;           /**< Physical address (for DMA)           */
     uint32_t    capacity;        /**< Allocated bytes                      */
     uint32_t    valid_bytes;     /**< Bytes of valid audio                 */
     uiox_spk_frame_state_t state;
     uint32_t    frame_id;
     uint8_t     in_use;
     struct uiox_spk_pcm_frame *next;
 } uiox_spk_pcm_frame_t;
 
 /* =========================================================================
  * Sample ring buffer (SPSC lock-free)
  * ====================================================================== */
 
 #define UIOX_SPK_RING_SAMPLES     (4096u)  /**< Power of 2, stereo int16 */
 #define UIOX_SPK_RING_MASK        (UIOX_SPK_RING_SAMPLES - 1u)
 
 typedef struct {
     int16_t          buf[UIOX_SPK_RING_SAMPLES * 2]; /**< Interleaved L+R */
     volatile uint32_t head;   /**< Write index (samples, not bytes)        */
     volatile uint32_t tail;   /**< Read index                              */
     uint32_t          overflow;
     uint32_t          underrun;
 } uiox_spk_ring_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void                uiox_spk_buf_init       (void);
 uiox_spk_pcm_frame_t *uiox_spk_buf_alloc_pcm(void);
 uiox_spk_pcm_frame_t *uiox_spk_buf_alloc_mix(void);
 void                uiox_spk_buf_free       (uiox_spk_pcm_frame_t *f);
 uint8_t             uiox_spk_buf_pcm_free   (void);
 
 /* Ring buffer API */
 void     uiox_spk_ring_init    (uiox_spk_ring_t *r);
 uint32_t uiox_spk_ring_write   (uiox_spk_ring_t *r,
                                  const int16_t *samples, uint32_t n_stereo);
 uint32_t uiox_spk_ring_read    (uiox_spk_ring_t *r,
                                  int16_t *samples, uint32_t n_stereo);
 uint32_t uiox_spk_ring_avail   (const uiox_spk_ring_t *r);
 uint32_t uiox_spk_ring_space   (const uiox_spk_ring_t *r);
 void     uiox_spk_ring_flush   (uiox_spk_ring_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SPK_BUF_H */
 