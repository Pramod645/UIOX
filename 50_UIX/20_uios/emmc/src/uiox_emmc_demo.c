/**
 * @file  uiox_emmc_demo.c
 * @brief UIOX eMMC stack demo — stub HAL + full stack exercise.
 * @date  2026-06-12
 */

 #include "uiox_emmc_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub register bank and flash storage
  * ====================================================================== */
 
 static uint32_t s_regs[0x80 / 4];
 
 /* Simulated flash: 64 MB user data */
 #define SIM_SECTORS  131072u
 static uint8_t s_flash[SIM_SECTORS * UIOX_EMMC_BLOCK_SIZE];
 
 /* Boot partition storage (4 MB each) */
 #define SIM_BOOT_SECTORS  8192u
 static uint8_t s_boot1[SIM_BOOT_SECTORS * UIOX_EMMC_BLOCK_SIZE];
 static uint8_t s_boot2[SIM_BOOT_SECTORS * UIOX_EMMC_BLOCK_SIZE];
 
 /* Simulated EXT_CSD */
 static uint8_t s_ext_csd[UIOX_EMMC_EXT_CSD_LEN];
 static bool    s_sim_error        = false;
 static bool    s_sim_health_warn  = false;
 static bool    s_sim_eol_warn     = false;
 
 static void stub_ext_csd_preset(void)
 {
     memset(s_ext_csd, 0, sizeof(s_ext_csd));
     /* SEC_COUNT = SIM_SECTORS (LE 32-bit at byte 212) */
     s_ext_csd[EXT_CSD_SEC_COUNT]     = (uint8_t)( SIM_SECTORS        & 0xFFu);
     s_ext_csd[EXT_CSD_SEC_COUNT + 1] = (uint8_t)((SIM_SECTORS >>  8) & 0xFFu);
     s_ext_csd[EXT_CSD_SEC_COUNT + 2] = (uint8_t)((SIM_SECTORS >> 16) & 0xFFu);
     s_ext_csd[EXT_CSD_SEC_COUNT + 3] = 0u;
     /* DEVICE_TYPE: HS400 + HS200 + HS52 */
     s_ext_csd[EXT_CSD_DEVICE_TYPE] =
         EXT_CSD_DEVICE_TYPE_HS400 |
         EXT_CSD_DEVICE_TYPE_HS200 |
         EXT_CSD_DEVICE_TYPE_HS_52;
     /* CACHE_SIZE: 4096 KB = 0x1000 (LE 32-bit at byte 249) */
     s_ext_csd[EXT_CSD_CACHE_SIZE]     = 0x00u;
     s_ext_csd[EXT_CSD_CACHE_SIZE + 1] = 0x10u;
     s_ext_csd[EXT_CSD_CACHE_SIZE + 2] = 0x00u;
     s_ext_csd[EXT_CSD_CACHE_SIZE + 3] = 0x00u;
     /* BOOT_SIZE_MULT: 32 → 32 × 128 KB = 4 MB per boot partition */
     s_ext_csd[EXT_CSD_BOOT_SIZE_MULT] = 32u;
     /* RPMB_SIZE_MULT: 8 → 8 × 128 KB = 1 MB */
     s_ext_csd[EXT_CSD_RPMB_SIZE_MULT] = 8u;
     /* HC_ERASE_GRP_SIZE: 4 (× 512 KB = 2 MB erase group) */
     s_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] = 4u;
     /* TRIM supported: CLASS_6_CTRL bit 0 */
     s_ext_csd[EXT_CSD_CLASS_6_CTRL] = 0x01u;
     /* CACHE_CTRL: off initially */
     s_ext_csd[EXT_CSD_CACHE_CTRL] = 0u;
     /* PRE_EOL_INFO: 1 = NORMAL */
     s_ext_csd[EXT_CSD_PRE_EOL_INFO] = EXT_CSD_PRE_EOL_NORMAL;
     /* DEVICE_LIFE_EST_A/B: 1 = 0–10% used */
     s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_A] = 1u;
     s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_B] = 1u;
     /* BKOPS_STATUS bit 0 */
     s_ext_csd[EXT_CSD_BKOPS_STATUS] = 0x01u;
     /* POWER_OFF_NOTIF bit 0 */
     s_ext_csd[EXT_CSD_POWER_OFF_NOTIF] = 0x01u;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_emmc_hw_t *hw)
 {
     (void)hw;
     memset(s_regs, 0, sizeof(s_regs));
     memset(s_flash, 0xA5u, sizeof(s_flash));
     memset(s_boot1, 0xBBu, sizeof(s_boot1));
     memset(s_boot2, 0xCCu, sizeof(s_boot2));
     stub_ext_csd_preset();
     /* Preset CAPS: HS400, 8-bit, DMA */
     s_regs[EMMC_HC_CAPS0 / 4] = 0x01E80080u;
     printf("  [hal] init  %s  base=0x%08lX  IRQ=%u\n",
            hw->model, (unsigned long)hw->base, hw->irq);
     return 0;
 }
 static void stub_deinit(uiox_emmc_hw_t *hw) { (void)hw; }
 
 static int stub_power_on(uiox_emmc_hw_t *hw)
 {
     (void)hw;
     s_regs[EMMC_HC_PWR_CTRL / 4] = 0x0Fu;
     printf("  [hal] power ON\n");
     return 0;
 }
 static void stub_power_off(uiox_emmc_hw_t *hw)
 {
     (void)hw;
     s_regs[EMMC_HC_PWR_CTRL / 4] = 0u;
     printf("  [hal] power OFF\n");
 }
 
 static uint32_t stub_reg_read(uiox_emmc_hw_t *hw, uint32_t off)
 {
     (void)hw;
     return s_regs[(off >> 2u) & 0x1Fu];
 }
 static void stub_reg_write(uiox_emmc_hw_t *hw, uint32_t off, uint32_t val)
 {
     (void)hw;
     /* INT_STATUS is W1C */
     if (off == EMMC_HC_INT_STATUS)
         s_regs[(off >> 2u) & 0x1Fu] &= ~val;
     else
         s_regs[(off >> 2u) & 0x1Fu]  =  val;
     if (off == EMMC_HC_INT_SIGNAL_EN)
         printf("  [hal] INT_SIGNAL_EN <- 0x%08X\n", val);
 }
 
 static int stub_set_clock(uiox_emmc_hw_t *hw, uint32_t hz)
 {
     (void)hw;
     printf("  [hal] set_clock  %u Hz\n", hz);
     return 0;
 }
 static int stub_set_bus_width(uiox_emmc_hw_t *hw, uint8_t w)
 {
     (void)hw;
     printf("  [hal] set_bus_width  %u-bit\n", w);
     if (w == 8u)
         s_regs[EMMC_HC_HOST_CTRL1 / 4] |= EMMC_HC1_8BIT;
     else if (w == 4u)
         s_regs[EMMC_HC_HOST_CTRL1 / 4] |= EMMC_HC1_4BIT;
     return 0;
 }
 static int stub_set_speed_mode(uiox_emmc_hw_t *hw, uiox_emmc_speed_t speed)
 {
     (void)hw;
     printf("  [hal] set_speed  %s\n", uiox_emmc_speed_name(speed));
     return 0;
 }
 
 static int stub_send_cmd(uiox_emmc_hw_t *hw, uint8_t cmd,
                           uint32_t arg, uiox_emmc_resp_t resp_type,
                           uint32_t *resp)
 {
     (void)hw; (void)arg; (void)resp_type;
     if (s_sim_error && cmd != MMC_CMD0_GO_IDLE) {
         printf("  [hal] CMD%-2u  SIMULATED ERROR\n", cmd);
         return -EIO;
     }
     printf("  [hal] CMD%-2u  arg=0x%08X\n", cmd, arg);
     if (!resp) return 0;
     memset(resp, 0, 16u);
     switch (cmd) {
     case MMC_CMD1_SEND_OP_COND:
         /* OCR: powered-up, HCS */
         resp[0] = MMC_CMD1_OCR_BUSY | MMC_CMD1_HCS | 0x00FF8080u;
         break;
     case MMC_CMD2_ALL_SEND_CID:
         /* CID: MID=0x15, PNM="UIOEMM", PRV=1, PSN=0x12345678 */
         resp[0] = 0x150100FFu;
         resp[1] = 0x55494F45u;  /* "UIOE" */
         resp[2] = 0x4D4D0112u;  /* "MM"   */
         resp[3] = 0x34567800u;
         break;
     case MMC_CMD3_SET_REL_ADDR:
     case MMC_CMD7_SELECT_CARD:
     case MMC_CMD6_SWITCH:
     case MMC_CMD16_SET_BLOCKLEN:
     case MMC_CMD35_ERASE_START:
     case MMC_CMD36_ERASE_END:
     case MMC_CMD38_ERASE:
         resp[0] = 0x00000900u;   /* R1: ready */
         break;
     case MMC_CMD9_SEND_CSD:
         /* CSD v2 placeholder */
         resp[0] = 0x400E0032u;
         resp[1] = 0x5B590003u;
         resp[2] = 0xE7FF7F80u;
         resp[3] = 0x0A404093u;
         break;
     case MMC_CMD13_SEND_STATUS:
         resp[0] = 0x00000900u;
         break;
     default:
         resp[0] = 0u;
         break;
     }
     return 0;
 }
 
 static int stub_read_blocks(uiox_emmc_hw_t *hw, uint32_t lba,
                              uint8_t *buf, uint32_t count)
 {
     if (s_sim_error) return -EIO;
     /* Route to appropriate partition storage */
     switch (hw->active_part) {
     case UIOX_EMMC_PART_BOOT1:
         if (lba + count > SIM_BOOT_SECTORS) return -ERANGE;
         memcpy(buf, &s_boot1[lba * UIOX_EMMC_BLOCK_SIZE],
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     case UIOX_EMMC_PART_BOOT2:
         if (lba + count > SIM_BOOT_SECTORS) return -ERANGE;
         memcpy(buf, &s_boot2[lba * UIOX_EMMC_BLOCK_SIZE],
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     default:
         if (lba + count > SIM_SECTORS) return -ERANGE;
         memcpy(buf, &s_flash[lba * UIOX_EMMC_BLOCK_SIZE],
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     }
     printf("  [hal] read_blocks  part=%s  lba=%u  count=%u\n",
            uiox_emmc_part_name(hw->active_part), lba, count);
     return 0;
 }
 
 static int stub_write_blocks(uiox_emmc_hw_t *hw, uint32_t lba,
                               const uint8_t *buf, uint32_t count)
 {
     if (s_sim_error) return -EIO;
     switch (hw->active_part) {
     case UIOX_EMMC_PART_BOOT1:
         if (lba + count > SIM_BOOT_SECTORS) return -ERANGE;
         memcpy(&s_boot1[lba * UIOX_EMMC_BLOCK_SIZE], buf,
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     case UIOX_EMMC_PART_BOOT2:
         if (lba + count > SIM_BOOT_SECTORS) return -ERANGE;
         memcpy(&s_boot2[lba * UIOX_EMMC_BLOCK_SIZE], buf,
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     default:
         if (lba + count > SIM_SECTORS) return -ERANGE;
         memcpy(&s_flash[lba * UIOX_EMMC_BLOCK_SIZE], buf,
                (size_t)count * UIOX_EMMC_BLOCK_SIZE);
         break;
     }
     printf("  [hal] write_blocks part=%s  lba=%u  count=%u\n",
            uiox_emmc_part_name(hw->active_part), lba, count);
     return 0;
 }
 
 static int stub_read_ext_csd(uiox_emmc_hw_t *hw, uint8_t *buf)
 {
     (void)hw;
     /* Inject health warnings if simulated */
     if (s_sim_health_warn) {
         s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_A] = 8u;
         s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_B] = 9u;
     }
     if (s_sim_eol_warn)
         s_ext_csd[EXT_CSD_PRE_EOL_INFO] = EXT_CSD_PRE_EOL_WARNING;
     memcpy(buf, s_ext_csd, UIOX_EMMC_EXT_CSD_LEN);
     printf("  [hal] read_ext_csd\n");
     return 0;
 }
 
 static int stub_write_ext_csd(uiox_emmc_hw_t *hw,
                                uint8_t index, uint8_t val)
 {
     (void)hw;
     s_ext_csd[index] = val;
     printf("  [hal] write_ext_csd  [%u] <- 0x%02X\n", index, val);
     return 0;
 }
 
 static int stub_tuning(uiox_emmc_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] HS200/HS400 tuning OK\n");
     return 0;
 }
 
 static uint8_t stub_crc7(const uint8_t *data, uint8_t len)
 {
     uint8_t crc = 0u;
     for (uint8_t i = 0u; i < len; i++) {
         crc ^= data[i];
         for (uint8_t b = 0u; b < 8u; b++)
             crc = (crc & 0x80u)
                   ? (uint8_t)((crc << 1u) ^ 0x09u)
                   : (uint8_t) (crc << 1u);
     }
     return crc >> 1u;
 }
 
 static uint16_t stub_crc16(const uint8_t *data, uint32_t len)
 {
     uint16_t crc = 0u;
     for (uint32_t i = 0u; i < len; i++) {
         crc ^= (uint16_t)data[i] << 8u;
         for (uint8_t b = 0u; b < 8u; b++)
             crc = (crc & 0x8000u)
                   ? (uint16_t)((crc << 1u) ^ 0x1021u)
                   : (uint16_t) (crc << 1u);
     }
     return crc;
 }
 
 static void stub_gpio_w(uiox_emmc_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 static bool stub_gpio_r(uiox_emmc_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return true; }
 
 static void stub_isr(uiox_emmc_hw_t *hw)
 {
     if (!hw) return;
     if (s_sim_error) {
         hw->pending_irq |= UIOX_EMMC_IRQ_ERROR;
         s_regs[EMMC_HC_INT_STATUS / 4] |= EMMC_INT_ERROR;
     } else {
         hw->pending_irq |= UIOX_EMMC_IRQ_XFER_DONE;
         s_regs[EMMC_HC_INT_STATUS / 4] |= EMMC_INT_XFER_COMPLETE;
     }
     printf("  [hal] ISR  pending=0x%08X\n", hw->pending_irq);
 }
 
 static const uiox_emmc_hw_ops_t stub_ops = {
     .init           = stub_init,
     .deinit         = stub_deinit,
     .power_on       = stub_power_on,
     .power_off      = stub_power_off,
     .reg_read       = stub_reg_read,
     .reg_write      = stub_reg_write,
     .set_clock      = stub_set_clock,
     .set_bus_width  = stub_set_bus_width,
     .set_speed_mode = stub_set_speed_mode,
     .send_cmd       = stub_send_cmd,
     .read_blocks    = stub_read_blocks,
     .write_blocks   = stub_write_blocks,
     .read_ext_csd   = stub_read_ext_csd,
     .write_ext_csd  = stub_write_ext_csd,
     .tuning         = stub_tuning,
     .crc7           = stub_crc7,
     .crc16          = stub_crc16,
     .gpio_write     = stub_gpio_w,
     .gpio_read      = stub_gpio_r,
     .isr            = stub_isr,
 };
 
 static uiox_emmc_hw_t s_hw = {
     .base     = 0xFE340000uL,
     .irq      = 126u,
     .caps     = UIOX_EMMC_CAP_8BIT      | UIOX_EMMC_CAP_4BIT      |
                 UIOX_EMMC_CAP_HS200     | UIOX_EMMC_CAP_HS400      |
                 UIOX_EMMC_CAP_DDR52     | UIOX_EMMC_CAP_DMA        |
                 UIOX_EMMC_CAP_CACHE     | UIOX_EMMC_CAP_TRIM       |
                 UIOX_EMMC_CAP_RPMB      | UIOX_EMMC_CAP_BKOPS      |
                 UIOX_EMMC_CAP_PON       | UIOX_EMMC_CAP_HEALTH     |
                 UIOX_EMMC_CAP_RST_N,
     .model    = "Raspberry Pi CM4 eMMC 5.1 SDIO Controller",
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_emmc_event(uiox_emmc_ev_t ev,
                            const uiox_emmc_evt_t *data, void *ctx)
 {
     (void)ctx;
     if (data)
         printf("  [event] %-16s  part=%-6s  lba=%u  sectors=%u"
                "  status=%d\n",
                uiox_emmc_ev_name(ev),
                uiox_emmc_part_name(data->part),
                data->lba, data->sectors, data->status);
     else
         printf("  [event] %-16s\n", uiox_emmc_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX eMMC Embedded Flash SDIO Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open ---\n");
     uiox_emmc_device_t      dev;
     uiox_emmc_open_params_t p = {
         .hw     = &s_hw,
         .hw_ops = &stub_ops,
         .evt_cb = on_emmc_event,
     };
     int rc = uiox_emmc_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start (init sequence: CMD0→CMD1→CID→RCA→CSD→SELECT"
            "→EXT_CSD→BUS_WIDTH→HS400→CACHE) ---\n");
     rc = uiox_emmc_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_emmc_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info ---\n");
     uiox_emmc_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write user partition (LBA 0, 8 sectors) ---\n");
     static uint8_t tx[8 * UIOX_EMMC_BLOCK_SIZE];
     for (uint32_t i = 0u; i < sizeof(tx); i++)
         tx[i] = (uint8_t)(i & 0xFFu);
     rc = uiox_emmc_write(&dev, UIOX_EMMC_PART_USER, 0u, tx, 8u);
     printf("  write user rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read back user partition (LBA 0, 8 sectors) ---\n");
     static uint8_t rx[8 * UIOX_EMMC_BLOCK_SIZE];
     rc = uiox_emmc_read(&dev, UIOX_EMMC_PART_USER, 0u, rx, 8u);
     printf("  read  user rc=%d  match=%s\n",
            rc, (memcmp(tx, rx, sizeof(tx)) == 0) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write boot partition 1 (LBA 0, 4 sectors) ---\n");
     static uint8_t boot_data[4 * UIOX_EMMC_BLOCK_SIZE];
     memset(boot_data, 0xBEu, sizeof(boot_data));
     rc = uiox_emmc_write(&dev, UIOX_EMMC_PART_BOOT1, 0u, boot_data, 4u);
     printf("  write BOOT1 rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read boot partition 1 ---\n");
     static uint8_t boot_rx[4 * UIOX_EMMC_BLOCK_SIZE];
     rc = uiox_emmc_read(&dev, UIOX_EMMC_PART_BOOT1, 0u, boot_rx, 4u);
     printf("  read  BOOT1 rc=%d  match=%s\n",
            rc,
            (memcmp(boot_data, boot_rx, sizeof(boot_data)) == 0)
            ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Switch back to USER partition ---\n");
     rc = uiox_emmc_read(&dev, UIOX_EMMC_PART_USER, 0u, rx, 1u);
     printf("  User partition re-read rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Flush write cache ---\n");
     rc = uiox_emmc_flush(&dev);
     printf("  flush rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- TRIM (LBA 100, 64 sectors) ---\n");
     rc = uiox_emmc_trim(&dev, 100u, 64u);
     printf("  trim rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Background operations (BKOPS) ---\n");
     rc = uiox_emmc_bkops(&dev);
     printf("  bkops rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Health check ---\n");
     uint8_t pre_eol = 0u, life_a = 0u, life_b = 0u;
     rc = uiox_emmc_health(&dev, &pre_eol, &life_a, &life_b);
     printf("  health rc=%d  pre_eol=%u  life_A=%u  life_B=%u\n",
            rc, pre_eol, life_a, life_b);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate health warning (life_A=8, life_B=9) ---\n");
     s_sim_health_warn = true;
     /* Force health poll by advancing timer */
     dev.subsys.health_poll_ms = UIOX_EMMC_HEALTH_INTERVAL_MS;
     uiox_emmc_tick(&dev, 100u);
     s_sim_health_warn = false;
     /* Reset */
     s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_A] = 1u;
     s_ext_csd[EXT_CSD_DEVICE_LIFE_EST_B] = 1u;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate EOL warning ---\n");
     s_sim_eol_warn = true;
     dev.subsys.health_poll_ms = UIOX_EMMC_HEALTH_INTERVAL_MS;
     uiox_emmc_tick(&dev, 110u);
     s_sim_eol_warn = false;
     s_ext_csd[EXT_CSD_PRE_EOL_INFO] = EXT_CSD_PRE_EOL_NORMAL;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Block pool alloc / dirty write-back ---\n");
     uiox_emmc_blk_t *blk = uiox_emmc_blk_alloc();
     if (blk) {
         memset(blk->data, 0xEEu,
                UIOX_EMMC_SECTORS_PER_BLK * UIOX_EMMC_BLOCK_SIZE);
         blk->lba     = 200u;
         blk->sectors = UIOX_EMMC_SECTORS_PER_BLK;
         blk->part    = UIOX_EMMC_PART_USER;
         blk->dirty   = true;
         rc = uiox_emmc_write(&dev, blk->part, blk->lba,
                               blk->data, blk->sectors);
         printf("  dirty write-back rc=%d  blk_free=%u\n",
                rc, uiox_emmc_blk_free_cnt());
         uiox_emmc_blk_free(blk);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- CRC7 / CRC16 verification ---\n");
     {
         static const uint8_t cmd0_frame[] =
             { 0x40, 0x00, 0x00, 0x00, 0x00 };
         uint8_t  c7  = uiox_emmc_hw_crc7(&s_hw, cmd0_frame,
                                            sizeof(cmd0_frame));
         uint16_t c16 = uiox_emmc_hw_crc16(&s_hw, rx, 16u);
         printf("  CRC7(CMD0)       = 0x%02X  (expected 0x4A)\n", c7);
         printf("  CRC16(rx[0..15]) = 0x%04X\n", c16);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate transfer error ---\n");
     s_sim_error = true;
     stub_isr(&s_hw);
     uiox_emmc_tick(&dev, 120u);
     printf("  State after error: %s\n",
            uiox_emmc_state_name(dev.subsys.state));
     /* Recover */
     s_sim_error                    = false;
     s_hw.pending_irq               = 0u;
     dev.subsys.state               = UIOX_EMMC_STATE_READY;
     dev.subsys.proto.initialized   = true;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Tick loop (5 × 10 ms) ---\n");
     for (uint32_t t = 200u; t <= 240u; t += 10u)
         uiox_emmc_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- MMIO register read-back ---\n");
     {
         uint32_t caps  = uiox_emmc_hw_reg_read(&s_hw, EMMC_HC_CAPS0);
         uint32_t hctrl = uiox_emmc_hw_reg_read(&s_hw, EMMC_HC_HOST_CTRL1);
         printf("  HC_CAPS0      = 0x%08X\n", caps);
         printf("  HC_HOST_CTRL1 = 0x%08X  8bit=%d  4bit=%d  HS=%d\n",
                hctrl,
                !!(hctrl & EMMC_HC1_8BIT),
                !!(hctrl & EMMC_HC1_4BIT),
                !!(hctrl & EMMC_HC1_HIGH_SPEED));
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_emmc_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_emmc_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close (flush + PON + power-off) ---\n");
     uiox_emmc_stop(&dev);
     printf("  State: %s\n", uiox_emmc_state_name(dev.subsys.state));
     uiox_emmc_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX eMMC Embedded Flash SDIO Demo complete ===\n");
     return 0;
 }
 