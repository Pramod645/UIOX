/**
 * @file    uiox_us_if.c
 * @brief   UIOX Ultrasonic interface driver implementation.
 * @date    2026-05-26
 */

 #include "uiox_us_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_us_if_config(uiox_us_if_t    *uif,
                        uiox_us_hw_t    *hw,
                        uiox_us_if_type_t type,
                        uint8_t          num_channels,
                        uint32_t         pulse_width_us,
                        uint32_t         period_ms,
                        uint32_t         timeout_ms,
                        uint32_t         sample_rate_hz,
                        uint32_t         echo_window_us)
 {
     if (!uif || !hw) return -EINVAL;
     memset(uif, 0, sizeof(*uif));
 
     uif->hw                    = hw;
     uif->type                  = type;
     uif->num_channels          = num_channels;
     uif->trig_cfg.pulse_width_us = pulse_width_us;
     uif->trig_cfg.period_ms    = period_ms;
     uif->trig_cfg.timeout_ms   = timeout_ms;
     uif->sample_rate_hz        = sample_rate_hz;
     uif->echo_window_us        = echo_window_us;
 
     /* Initialise buffer pool */
     uiox_us_buf_init();
 
     /* Validate ADC echo window fits in raw frame */
     if (type == UIOX_US_IF_ADC) {
         uint32_t max_samples = (uint32_t)(
             (uint64_t)sample_rate_hz *
             (uint64_t)echo_window_us / 1000000ULL);
         if (max_samples * 2u > UIOX_US_RAW_FRAME_MAX) return -ENOMEM;
     }
 
     uif->primed = true;
     return 0;
 }
 
 int64_t uiox_us_if_measure(uiox_us_if_t     *uif,
                              uint8_t           ch,
                              uiox_us_frame_t **raw_out)
 {
     if (!uif || !uif->hw || !uif->primed) return -EINVAL;
     if (ch >= uif->num_channels)          return -EINVAL;
     if (raw_out) *raw_out = NULL;
 
     /* 1. Fire trigger */
     int rc = uiox_us_hw_trigger(uif->hw, ch, &uif->trig_cfg);
     if (rc < 0) return (int64_t)rc;
 
     const uiox_us_hw_ops_t *ops =
         (const uiox_us_hw_ops_t *)uif->hw->priv;
 
     if (uif->type == UIOX_US_IF_ADC && ops && ops->dma_queue) {
         /* ADC mode: queue a raw buffer and wait for DMA completion */
         uiox_us_frame_t *f = uiox_us_buf_alloc_raw();
         if (!f) return -ENOMEM;
 
         f->channel        = ch;
         f->sample_rate_hz = uif->sample_rate_hz;
         f->num_samples    = (uint16_t)(
             (uint64_t)uif->sample_rate_hz *
             (uint64_t)uif->echo_window_us / 1000000ULL);
 
         rc = ops->dma_queue(uif->hw, f->paddr, f->num_samples * 2u);
         if (rc < 0) { uiox_us_buf_free(f); return (int64_t)rc; }
 
         uintptr_t phys  = 0;
         uint32_t  bytes = 0;
         rc = ops->dma_complete
              ? ops->dma_complete(uif->hw, &phys, &bytes) : -ENOSYS;
         if (rc <= 0) { uiox_us_buf_free(f); return (int64_t)rc; }
 
         f->used = bytes;
         if (raw_out) *raw_out = f;
         return (int64_t)bytes;
 
     } else {
         /* GPIO / SPI / UART mode: wait for echo pulse width */
         int64_t ticks = uiox_us_hw_echo_wait(uif->hw, ch,
                                               uif->trig_cfg.timeout_ms);
         return ticks;
     }
 }
 