/**
 * @file  uiox_emmc_if.c
 * @brief UIOX eMMC interface driver — CMD/DAT, bus config, IRQ.
 * @date  2026-06-12
 */

 #include "uiox_emmc_if.h"
 
 int uiox_emmc_if_config(uiox_emmc_if_t *eif, uiox_emmc_hw_t *hw)
 {
     if (!eif || !hw) return -EINVAL;
     memset(eif, 0, sizeof(*eif));
     eif->hw        = hw;
     eif->primed    = true;
     eif->bus_width = 1u;
     eif->speed     = UIOX_EMMC_SPEED_IDENT;
     uiox_emmc_buf_init();
     return 0;
 }
 
 int uiox_emmc_if_start(uiox_emmc_if_t *eif)
 {
     if (!eif || !eif->primed) return -EINVAL;
     int rc = uiox_emmc_hw_power_on(eif->hw);
     if (rc < 0) return rc;
     /* Start at identification clock (400 kHz) */
     rc = uiox_emmc_hw_set_clock(eif->hw, 400000u);
     if (rc < 0) return rc;
     /* Enable interrupts */
     uiox_emmc_hw_reg_write(eif->hw, EMMC_HC_INT_STATUS_EN,
                             EMMC_INT_CMD_COMPLETE | EMMC_INT_XFER_COMPLETE |
                             EMMC_INT_ERROR);
     uiox_emmc_hw_reg_write(eif->hw, EMMC_HC_INT_SIGNAL_EN,
                             EMMC_INT_CMD_COMPLETE | EMMC_INT_XFER_COMPLETE |
                             EMMC_INT_ERROR);
     return 0;
 }
 
 void uiox_emmc_if_stop(uiox_emmc_if_t *eif)
 {
     if (!eif) return;
     uiox_emmc_hw_reg_write(eif->hw, EMMC_HC_INT_SIGNAL_EN, 0u);
     uiox_emmc_hw_reg_write(eif->hw, EMMC_HC_INT_STATUS_EN, 0u);
     uiox_emmc_hw_power_off(eif->hw);
 }
 
 int uiox_emmc_if_send_cmd(uiox_emmc_if_t *eif, uint8_t cmd,
                            uint32_t arg, uiox_emmc_resp_t resp_type,
                            uint32_t *resp)
 {
     if (!eif) return -EINVAL;
     eif->stats.cmds_sent++;
     int rc = uiox_emmc_hw_send_cmd(eif->hw, cmd, arg, resp_type, resp);
     if (rc < 0) eif->stats.errors++;
     return rc;
 }
 
 int uiox_emmc_if_switch(uiox_emmc_if_t *eif,
                          uint8_t access, uint8_t index,
                          uint8_t val, uint8_t cmd_set)
 {
     if (!eif) return -EINVAL;
     /*
      * CMD6 (SWITCH) argument:
      * [25:24] = Access, [23:16] = Index, [15:8] = Value, [2:0] = Cmd Set
      */
     uint32_t arg = ((uint32_t)access  << 24u) |
                    ((uint32_t)index   << 16u) |
                    ((uint32_t)val     <<  8u) |
                    ((uint32_t)cmd_set &  0x7u);
     uint32_t resp;
     return uiox_emmc_if_send_cmd(eif, MMC_CMD6_SWITCH, arg,
                                   EMMC_RESP_R1B, &resp);
 }
 
 int uiox_emmc_if_read(uiox_emmc_if_t *eif, uint32_t lba,
                        uint8_t *buf, uint32_t count)
 {
     if (!eif || !buf || !count) return -EINVAL;
     int rc = uiox_emmc_hw_read_blocks(eif->hw, lba, buf, count);
     if (rc == 0) {
         eif->stats.blocks_read += count;
         eif->stats.bytes_read  +=
             (uint64_t)count * UIOX_EMMC_BLOCK_SIZE;
     } else {
         eif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_emmc_if_write(uiox_emmc_if_t *eif, uint32_t lba,
                         const uint8_t *buf, uint32_t count)
 {
     if (!eif || !buf || !count) return -EINVAL;
     int rc = uiox_emmc_hw_write_blocks(eif->hw, lba, buf, count);
     if (rc == 0) {
         eif->stats.blocks_written += count;
         eif->stats.bytes_written  +=
             (uint64_t)count * UIOX_EMMC_BLOCK_SIZE;
     } else {
         eif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_emmc_if_read_ext_csd(uiox_emmc_if_t *eif, uint8_t *buf)
 {
     if (!eif || !buf) return -EINVAL;
     return uiox_emmc_hw_read_ext_csd(eif->hw, buf);
 }
 
 int uiox_emmc_if_set_clock(uiox_emmc_if_t *eif, uint32_t hz)
 {
     if (!eif) return -EINVAL;
     return uiox_emmc_hw_set_clock(eif->hw, hz);
 }
 
 int uiox_emmc_if_set_bus_width(uiox_emmc_if_t *eif, uint8_t width)
 {
     if (!eif) return -EINVAL;
     int rc = uiox_emmc_hw_set_bus_width(eif->hw, width);
     if (rc == 0) eif->bus_width = width;
     return rc;
 }
 
 int uiox_emmc_if_set_speed(uiox_emmc_if_t *eif, uiox_emmc_speed_t speed)
 {
     if (!eif) return -EINVAL;
     int rc = uiox_emmc_hw_set_speed(eif->hw, speed);
     if (rc == 0) eif->speed = speed;
     return rc;
 }
 
 int uiox_emmc_if_select_part(uiox_emmc_if_t *eif, uiox_emmc_part_t part)
 {
     if (!eif) return -EINVAL;
     /* PARTITION_CONFIG [179]: [2:0] = PARTITION_ACCESS */
     int rc = uiox_emmc_if_switch(eif,
                                   MMC_SWITCH_WRITE_BYTE,
                                   EXT_CSD_PARTITION_CONFIG,
                                   (uint8_t)part, 0u);
     if (rc == 0) eif->hw->active_part = part;
     return rc;
 }
 
 uiox_emmc_evt_t *uiox_emmc_if_irq_handle(uiox_emmc_if_t *eif,
                                            uint32_t now_ms)
 {
     if (!eif) return NULL;
     uint32_t irq = eif->hw->pending_irq;
     if (!irq) return NULL;
 
     eif->hw->pending_irq = 0u;
     eif->stats.irq_count++;
 
     /* Acknowledge host controller */
     uint32_t istat = uiox_emmc_hw_reg_read(eif->hw, EMMC_HC_INT_STATUS);
     uiox_emmc_hw_reg_write(eif->hw, EMMC_HC_INT_STATUS, istat);
 
     uiox_emmc_evt_t *e = uiox_emmc_evt_alloc();
     if (!e) { eif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->part         = eif->hw->active_part;
 
     if (irq & UIOX_EMMC_IRQ_ERROR) {
         e->type   = UIOX_EMMC_EVT_ERROR;
         e->status = -EIO;
         eif->stats.errors++;
     } else if (irq & UIOX_EMMC_IRQ_XFER_DONE) {
         e->type   = UIOX_EMMC_EVT_READ_DONE;
         e->status = 0;
     } else {
         e->type   = UIOX_EMMC_EVT_CMD_DONE;
         e->status = 0;
     }
     return e;
 }
 
 void uiox_emmc_if_stats_get(const uiox_emmc_if_t *eif,
                               uiox_emmc_if_stats_t *out)
 { if (!eif || !out) return; memcpy(out, &eif->stats, sizeof(*out)); }
 