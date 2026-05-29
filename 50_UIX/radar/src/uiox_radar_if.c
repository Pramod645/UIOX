/**
 * @file    uiox_radar_if.c
 * @brief   UIOX Radar interface driver implementation.
 * @date    2026-05-26
 */

 #include "uiox_radar_if.h"
 #include <string.h>
 #include <errno.h>
 
 /* Bytes per ADC sample: 4 bytes for complex I+Q (2×int16) */
 #define UIOX_RADAR_SAMPLE_BYTES     4
 
 int uiox_radar_if_config(uiox_radar_if_t    *rif,
                           uiox_radar_hw_t    *hw,
                           uiox_radar_if_type_t type,
                           uint8_t  num_rx,
                           uint8_t  num_tx,
                           uint16_t num_chirps,
                           uint16_t num_samples,
                           uiox_radar_adc_fmt_t adc_fmt)
 {
     if (!rif || !hw) return -EINVAL;
     memset(rif, 0, sizeof(*rif));
 
     rif->hw          = hw;
     rif->type        = type;
     rif->num_rx      = num_rx;
     rif->num_tx      = num_tx;
     rif->num_chirps  = num_chirps;
     rif->num_samples = num_samples;
     rif->adc_fmt     = adc_fmt;
 
     /* Compute frame size:
      * raw bytes = num_rx × num_chirps × num_samples × 4 (I+Q int16)
      * For MIMO: num_tx virtual channels interleaved in chirp dimension */
     rif->frame_bytes = (uint32_t)num_rx *
                        (uint32_t)num_tx *
                        (uint32_t)num_chirps *
                        (uint32_t)num_samples *
                        UIOX_RADAR_SAMPLE_BYTES;
 
     /* Validate against pool capacity */
     if (rif->frame_bytes > UIOX_RADAR_RAW_FRAME_MAX) return -ENOMEM;
 
     /* Program ADC capture into HAL */
     const uiox_radar_hw_ops_t *ops =
         (const uiox_radar_hw_ops_t *)hw->priv;
     if (!ops || !ops->set_adc) return -ENOSYS;
 
     int rc = ops->set_adc(hw, num_samples,
                           (uint16_t)(num_tx * num_chirps), adc_fmt);
     if (rc < 0) return rc;
 
     /* Initialise buffer pool for this geometry */
     uiox_radar_buf_init();
 
     rif->primed = 0;
     return 0;
 }
 
 int uiox_radar_if_prime(uiox_radar_if_t *rif, int count)
 {
     if (!rif || !rif->hw) return -EINVAL;
     const uiox_radar_hw_ops_t *ops =
         (const uiox_radar_hw_ops_t *)rif->hw->priv;
     if (!ops || !ops->dma_queue) return -ENOSYS;
 
     int queued = 0;
     for (int i = 0; i < count; i++) {
         uiox_radar_frame_t *f = uiox_radar_buf_alloc_raw();
         if (!f) break;
 
         /* Tag geometry onto frame before queuing */
         f->num_rx      = rif->num_rx;
         f->num_tx      = rif->num_tx;
         f->num_chirps  = rif->num_chirps;
         f->num_samples = rif->num_samples;
 
         int rc = ops->dma_queue(rif->hw, f->paddr, rif->frame_bytes);
         if (rc < 0) {
             uiox_radar_buf_free(f);
             break;
         }
         queued++;
     }
 
     if (queued > 0) rif->primed = 1;
     return queued > 0 ? queued : -ENOBUFS;
 }
 
 uiox_radar_frame_t *uiox_radar_if_dequeue(uiox_radar_if_t *rif)
 {
     if (!rif || !rif->hw || !rif->primed) return NULL;
 
     const uiox_radar_hw_ops_t *ops =
         (const uiox_radar_hw_ops_t *)rif->hw->priv;
     if (!ops || !ops->dma_complete) return NULL;
 
     uintptr_t phys  = 0;
     uint32_t  bytes = 0;
     int rc = ops->dma_complete(rif->hw, &phys, &bytes);
     if (rc <= 0) return NULL;
 
     /* Walk raw pool to find matching paddr */
     extern uiox_radar_frame_t s_raw_desc[];
     for (int i = 0; i < UIOX_RADAR_RAW_POOL_SIZE; i++) {
         if (s_raw_desc[i].paddr == phys && s_raw_desc[i].in_use) {
             s_raw_desc[i].used = bytes;
             return &s_raw_desc[i];
         }
     }
     return NULL;
 }
 