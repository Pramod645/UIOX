/**
 * @file    uiox_us_buf.h
 * @brief   UIOX Ultrasonic echo sample buffer pool.
 *
 * Provides statically allocated frame buffers for:
 *   RAW pool — raw ADC echo samples per measurement
 *   DSP pool — processed results (envelope, ToF, distance)
 *
 * @date    2026-05-26
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_US_BUF_H
 #define UIOX_US_BUF_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool sizing
  * HC-SR04 at 40 kHz, 60 ms window, 1 Msps ADC → 60 000 samples × 2 bytes
  * ====================================================================== */
 
 #define UIOX_US_RAW_POOL_SIZE     8
 #define UIOX_US_DSP_POOL_SIZE     8
 #define UIOX_US_RAW_FRAME_MAX     (65536 * 2)  /**< 64k samples × 2 bytes   */
 #define UIOX_US_DSP_FRAME_MAX     (1024  * 4)  /**< 1k float results         */
 #define UIOX_US_BUF_ALIGN         32
 
 /* =========================================================================
  * Frame types
  * ====================================================================== */
 
 typedef enum {
     UIOX_US_FRAME_RAW = 0,   /**< Raw ADC echo samples (int16)             */
     UIOX_US_FRAME_ENVELOPE,  /**< Rectified + filtered envelope (float)    */
     UIOX_US_FRAME_RESULT,    /**< ToF + distance result (float)            */
 } uiox_us_frame_type_t;
 
 /* =========================================================================
  * Frame descriptor
  * ====================================================================== */
 
 typedef struct uiox_us_frame {
     uint8_t    *vaddr;          /**< Virtual address                        */
     uintptr_t   paddr;          /**< Physical address (for DMA)             */
     uint32_t    capacity;       /**< Allocated byte capacity                */
     uint32_t    used;           /**< Bytes of valid data                    */
     uint16_t    num_samples;    /**< Number of ADC samples captured         */
     uint32_t    sample_rate_hz; /**< ADC sampling rate                      */
     uint8_t     channel;        /**< Sensor channel index                   */
     uint64_t    ts_ns;          /**< Capture timestamp (ns)                 */
     uint32_t    meas_id;        /**< Monotonic measurement counter          */
     uint8_t     type;           /**< uiox_us_frame_type_t                   */
     uint8_t     in_use;         /**< Reference count                        */
     struct uiox_us_frame *next; /**< Free-list linkage                      */
 } uiox_us_frame_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void             uiox_us_buf_init(void);
 uiox_us_frame_t *uiox_us_buf_alloc_raw(void);
 uiox_us_frame_t *uiox_us_buf_alloc_dsp(void);
 void             uiox_us_buf_ref  (uiox_us_frame_t *f);
 void             uiox_us_buf_free (uiox_us_frame_t *f);
 uint8_t          uiox_us_buf_raw_free(void);
 uint8_t          uiox_us_buf_dsp_free(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_US_BUF_H */
 