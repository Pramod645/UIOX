/**
 * @file    uiox_tb4_if.c
 * @brief   UIOX Thunderbolt 4 NHI interface driver implementation.
 * @date    2026-06-08
 */

 #include "uiox_tb4_if.h"
 #include "uiox_klibc.h"
 
 int uiox_tb4_if_config(uiox_tb4_if_t *tif, uiox_tb4_hw_t *hw)
 {
     if (!tif || !hw) return -EINVAL;
     memset(tif, 0, sizeof(*tif));
     tif->hw     = hw;
     tif->primed = true;
     uiox_tb4_buf_init();
     return 0;
 }
 
 int uiox_tb4_if_start(uiox_tb4_if_t *tif)
 {
     if (!tif || !tif->primed) return -EINVAL;
     int rc = uiox_tb4_hw_power_on(tif->hw);
     if (rc < 0) return rc;
 
     /* Enable interrupts */
     uiox_tb4_hw_nhi_write(tif->hw, NHI_INTERRUPT_MASK_CLR,
                            NHI_INT_TX_RING(0) | NHI_INT_RX_RING(0) |
                            NHI_INT_ICM | NHI_INT_HOTPLUG | NHI_INT_ERROR);
     return 0;
 }
 
 void uiox_tb4_if_stop(uiox_tb4_if_t *tif)
 {
     if (!tif) return;
     /* Mask all interrupts */
     uiox_tb4_hw_nhi_write(tif->hw, NHI_INTERRUPT_MASK_SET, 0xFFFFFFFFu);
     uiox_tb4_hw_power_off(tif->hw);
 }
 
 int uiox_tb4_if_tx(uiox_tb4_if_t *tif, uiox_tb4_frame_t *frame)
 {
     if (!tif || !frame || !frame->len) return -EINVAL;
     int rc = uiox_tb4_hw_tx_submit(tif->hw, frame->paddr,
                                     frame->len, true);
     if (rc == 0) {
         tif->stats.frames_tx++;
         tif->stats.bytes_tx += frame->len;
     } else {
         tif->stats.errors++;
     }
     return rc;
 }
 
 uiox_tb4_frame_t *uiox_tb4_if_rx(uiox_tb4_if_t *tif)
 {
     if (!tif) return NULL;
     uintptr_t phys = 0;
     uint32_t  len  = 0;
     int rc = uiox_tb4_hw_rx_poll(tif->hw, &phys, &len);
     if (rc <= 0) return NULL;
 
     uiox_tb4_frame_t *f = uiox_tb4_buf_alloc_rx();
     if (!f) { tif->stats.errors++; return NULL; }
     /* In real HW: map phys → virt; here identity map */
     f->paddr = phys;
     f->len   = len;
     tif->stats.frames_rx++;
     tif->stats.bytes_rx += len;
     return f;
 }
 
 int uiox_tb4_if_icm_cmd(uiox_tb4_if_t *tif,
                           const uint32_t *req, uint8_t req_dwords,
                           uint32_t *resp, uint8_t resp_max)
 {
     if (!tif || !req || !resp) return -EINVAL;
     int rc = uiox_tb4_hw_icm_send(tif->hw, req, req_dwords);
     if (rc < 0) return rc;
     tif->stats.icm_msgs_sent++;
     rc = uiox_tb4_hw_icm_recv(tif->hw, resp, resp_max);
     if (rc > 0) tif->stats.icm_msgs_recv++;
     return rc;
 }
 
 void uiox_tb4_if_irq_handle(uiox_tb4_if_t *tif)
 {
     if (!tif) return;
     uint32_t status = uiox_tb4_hw_nhi_read(tif->hw,
                                              NHI_INTERRUPT_STATUS);
     if (status & NHI_INT_HOTPLUG) {
         tif->stats.hotplug_events++;
         tif->hw->pending_irq |= NHI_INT_HOTPLUG;
     }
     if (status & NHI_INT_ERROR) tif->stats.errors++;
     /* Acknowledge */
     uiox_tb4_hw_nhi_write(tif->hw, NHI_INTERRUPT_STATUS, status);
 }
 
 void uiox_tb4_if_stats_get(const uiox_tb4_if_t *tif,
                             uiox_tb4_if_stats_t *out)
 { if (!tif || !out) return; memcpy(out, &tif->stats, sizeof(*out)); }
 