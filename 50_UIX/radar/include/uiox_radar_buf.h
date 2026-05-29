/**
 * @file    uiox_radar_buf.h
 * @brief   UIOX Radar frame buffer pool (ADC + processed frames).
 *
 * Two pools:
 *   RAW pool  — holds raw ADC data (one entry per radar frame)
 *   DSP pool  — holds processed output (range-Doppler maps, point clouds)
 *
 * All buffers are statically allocated to avoid heap fragmentation on
 * embedded/RTOS targets.
 *
 * @date    2026-05-26
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_RADAR_BUF_H
 #define UIOX_RADAR_BUF_H
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool sizing
  *   AWR1843: 4 RX × 256 chirps × 256 samples × 4 bytes (I+Q 16-bit each)
  *   = 4 × 256 × 256 × 4 = 1 048 576 bytes ≈ 1 MB per raw frame
  * ====================================================================== */
 
 #define UIOX_RADAR_RAW_POOL_SIZE    4           /**< Raw ADC frame pool depth  */
 #define UIOX_RADAR_DSP_POOL_SIZE    4           /**< DSP output pool depth     */
 #define UIOX_RADAR_RAW_FRAME_MAX    (1024*1024) /**< 1 MB raw ADC frame        */
 #define UIOX_RADAR_DSP_FRAME_MAX    (256*256*4) /**< Range-Doppler map output  */
 #define UIOX_RADAR_BUF_ALIGN        64
 
 /* =========================================================================
  * Frame types
  * ====================================================================== */
 
 typedef enum {
     UIOX_RADAR_FRAME_RAW = 0,  /**< Raw ADC samples (I+Q per RX per chirp)  */
     UIOX_RADAR_FRAME_RANGE,    /**< 1D range FFT output                      */
     UIOX_RADAR_FRAME_DOPPLER,  /**< 2D range-Doppler map                     */
     UIOX_RADAR_FRAME_CFAR,     /**< CFAR detection list                      */
     UIOX_RADAR_FRAME_POINTS,   /**< Final point cloud                        */
 } uiox_radar_frame_type_t;
 
 /* =========================================================================
  * Frame descriptor
  * ====================================================================== */
 
 typedef struct uiox_radar_frame {
     uint8_t    *vaddr;          /**< Virtual address of data                 */
     uintptr_t   paddr;          /**< Physical address (for DMA)              */
     uint32_t    capacity;       /**< Allocated byte capacity                 */
     uint32_t    used;           /**< Bytes of valid data written             */
 
     /* Geometry (filled in by subsystem after capture) */
     uint16_t    num_rx;         /**< Number of RX channels                   */
     uint16_t    num_tx;         /**< Number of TX channels (MIMO virtual)    */
     uint16_t    num_chirps;     /**< Chirps per frame                        */
     uint16_t    num_samples;    /**< ADC samples per chirp per RX            */
     uint16_t    num_range_bins; /**< Range FFT bins                          */
     uint16_t    num_doppler_bins;/**< Doppler FFT bins                       */
 
     uint64_t    ts_ns;          /**< Capture timestamp (nanoseconds)         */
     uint32_t    frame_id;       /**< Monotonic frame counter                 */
     uint8_t     type;           /**< uiox_radar_frame_type_t                 */
     uint8_t     in_use;         /**< Reference count                         */
 
     struct uiox_radar_frame *next; /**< Free-list linkage (internal)         */
 } uiox_radar_frame_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 /** Initialise both pools. Call once at boot before any buffer use. */
 void uiox_radar_buf_init(void);
 
 /** Allocate a raw ADC frame. Returns NULL if pool exhausted. */
 uiox_radar_frame_t *uiox_radar_buf_alloc_raw(void);
 
 /** Allocate a DSP output frame. Returns NULL if pool exhausted. */
 uiox_radar_frame_t *uiox_radar_buf_alloc_dsp(void);
 
 /** Increment reference count. */
 void uiox_radar_buf_ref (uiox_radar_frame_t *f);
 
 /** Decrement reference count; returns to pool when it reaches 0. */
 void uiox_radar_buf_free(uiox_radar_frame_t *f);
 
 /** Number of free raw frames. */
 uint8_t uiox_radar_buf_raw_free_count(void);
 
 /** Number of free DSP frames. */
 uint8_t uiox_radar_buf_dsp_free_count(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_BUF_H */
 