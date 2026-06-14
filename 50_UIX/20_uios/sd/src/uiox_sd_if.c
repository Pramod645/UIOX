/**
 * @file  uiox_sd_if.c
 * @brief UIOX SD interface driver — CMD/DAT, CRC, bus width.
 * @date  2026-06-11
 */

 #include "uiox_sd_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_sd_if_config(uiox_sd_if_t *sif, uiox_sd_hw_t *hw)
 {
     if (!sif || !hw) return -EINVAL;
     memset(sif, 0, sizeof(*sif));
     sif->hw        = hw;
     sif->primed    = true;
     sif->bus_width = 1u;
     uiox_sd_buf_init();
     return 0;
 }
 
 int uiox_sd_if_start(uiox_sd_if_t *sif)
 {
     if (!sif || !sif->primed) return -EINVAL;
     int rc = uiox_sd_hw_power_on(sif->hw);
     if (rc < 0) return rc;
     /* Start at identification clock */
     rc = uiox_sd_hw_set_clock(sif->hw, sif->hw->clk_id_hz);
     if (rc < 0) return rc;
     /* Enable card-detect and transfer-complete interrupts */
     uiox_sd_hw_reg_write(sif->hw, SDIO_REG_INT_STATUS_EN,
                           SDIO_INT_CMD_COMPLETE | SDIO_INT_XFER_COMPLETE |
                           SDIO_INT_CARD_INSERT  | SDIO_INT_CARD_REMOVE   |
                           SDIO_INT_ERROR);
     uiox_sd_hw_reg_write(sif->hw, SDIO_REG_INT_SIGNAL_EN,
                           SDIO_INT_CMD_COMPLETE | SDIO_INT_XFER_COMPLETE |
                           SDIO_INT_CARD_INSERT  | SDIO_INT_CARD_REMOVE   |
                           SDIO_INT_ERROR);
     return 0;
 }
 
 void uiox_sd_if_stop(uiox_sd_if_t *sif)
 {
     if (!sif) return;
     uiox_sd_hw_reg_write(sif->hw, SDIO_REG_INT_SIGNAL_EN, 0u);
     uiox_sd_hw_reg_write(sif->hw, SDIO_REG_INT_STATUS_EN, 0u);
     uiox_sd_hw_power_off(sif->hw);
 }
 
 int uiox_sd_if_send_cmd(uiox_sd_if_t *sif, uint8_t cmd,
                          uint32_t arg, uiox_sd_resp_t resp_type,
                          uint32_t *resp)
 {
     if (!sif) return -EINVAL;
     sif->stats.cmds_sent++;
     int rc = uiox_sd_hw_send_cmd(sif->hw, cmd, arg, resp_type, resp);
     if (rc < 0) sif->stats.errors++;
     return rc;
 }
 
 int uiox_sd_if_read(uiox_sd_if_t *sif, uint32_t lba,
                      uint8_t *buf, uint32_t count)
 {
     if (!sif || !buf || !count) return -EINVAL;
     int rc = uiox_sd_hw_read_blocks(sif->hw, lba, buf, count);
     if (rc == 0) {
         sif->stats.blocks_read += count;
         sif->stats.bytes_read  += (uint64_t)count * UIOX_SD_BLOCK_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_sd_if_write(uiox_sd_if_t *sif, uint32_t lba,
                       const uint8_t *buf, uint32_t count)
 {
     if (!sif || !buf || !count) return -EINVAL;
     int rc = uiox_sd_hw_write_blocks(sif->hw, lba, buf, count);
     if (rc == 0) {
         sif->stats.blocks_written += count;
         sif->stats.bytes_written  += (uint64_t)count * UIOX_SD_BLOCK_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_sd_if_set_clock(uiox_sd_if_t *sif, uint32_t hz)
 {
     if (!sif) return -EINVAL;
     return uiox_sd_hw_set_clock(sif->hw, hz);
 }
 
 int uiox_sd_if_set_bus_width(uiox_sd_if_t *sif, uint8_t width)
 {
     if (!sif || (width != 1u && width != 4u)) return -EINVAL;
     int rc = uiox_sd_hw_set_bus_width(sif->hw, width);
     if (rc == 0) sif->bus_width = width;
     return rc;
 }
 
 uiox_sd_evt_t *uiox_sd_if_irq_handle(uiox_sd_if_t *sif, uint32_t now_ms)
 {
     if (!sif) return NULL;
     uint32_t irq = sif->hw->pending_irq;
     if (!irq) return NULL;
 
     sif->hw->pending_irq = 0u;
     sif->stats.irq_count++;
 
     /* Acknowledge hardware interrupt status */
     uint32_t hw_status = uiox_sd_hw_reg_read(sif->hw, SDIO_REG_INT_STATUS);
     uiox_sd_hw_reg_write(sif->hw, SDIO_REG_INT_STATUS, hw_status);
 
     uiox_sd_evt_t *e = uiox_sd_evt_alloc();
     if (!e) { sif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->status       = 0;
 
     if (irq & UIOX_SD_IRQ_CARD_INSERT) {
         e->type = UIOX_SD_EVT_CARD_INSERT;
     } else if (irq & UIOX_SD_IRQ_CARD_REMOVE) {
         e->type = UIOX_SD_EVT_CARD_REMOVE;
     } else if (irq & UIOX_SD_IRQ_XFER_DONE) {
         e->type = UIOX_SD_EVT_READ_DONE;
     } else if (irq & UIOX_SD_IRQ_ERROR) {
         e->type   = UIOX_SD_EVT_ERROR;
         e->status = -EIO;
         sif->stats.errors++;
     } else {
         e->type = UIOX_SD_EVT_CMD_DONE;
     }
     return e;
 }
 
 void uiox_sd_if_stats_get(const uiox_sd_if_t *sif,
                             uiox_sd_if_stats_t *out)
 { if (!sif || !out) return; memcpy(out, &sif->stats, sizeof(*out)); }
 