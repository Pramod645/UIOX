/**
 * @file  uiox_sd_demo.c
 * @brief UIOX SD Card Reader stack demo — stub HAL + full stack exercise.
 * @date  2026-06-11
 */

 #include "uiox_sd_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub register bank and card simulation
  * ====================================================================== */
 
 static uint32_t s_regs[0x80 / 4];   /* SDIO host controller registers    */
 static uint8_t  s_card_mem[1024 * UIOX_SD_BLOCK_SIZE]; /* 512 KB stub card */
 
 static bool s_card_inserted   = false;
 static bool s_write_protect   = false;
 static bool s_sim_insert      = false;
 static bool s_sim_remove      = false;
 static bool s_sim_wp_change   = false;
 static bool s_sim_error       = false;
 
 /* Simulated CSD v2 for a 512 MB SDHC card
  * C_SIZE = (capacity / (512*1024)) - 1 = (512*1024*1024 / 524288) - 1 = 999
  */
 static const uint8_t s_csd_sdhc[16] = {
     0x40,0x0E,0x00,0x32, 0x5B,0x59,0x00,0x03,
     0xE7,0xFF,0x7F,0x80, 0x0A,0x40,0x40,0x93
 };
 static const uint8_t s_cid[16] = {
     0x03,0x53,0x44,0x53, 0x43,0x33,0x32,0x47,
     0x00,0x00,0x00,0x00, 0x20,0x01,0x26,0xAB
 };
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_sd_hw_t *hw)
 {
     (void)hw;
     memset(s_regs, 0, sizeof(s_regs));
     memset(s_card_mem, 0xA5u, sizeof(s_card_mem));
     /* Preset capabilities register */
     s_regs[SDIO_REG_CAPS0 / 4] = 0x01E00080u;  /* SDIO HC v3, 50 MHz */
     printf("  [hal] init  %s  base=0x%08lX  IRQ=%u  bus=%s\n",
            hw->model, (unsigned long)hw->base, hw->irq,
            uiox_sd_bus_name(hw->bus_type));
     return 0;
 }
 static void stub_deinit(uiox_sd_hw_t *hw) { (void)hw; }
 
 static int stub_power_on(uiox_sd_hw_t *hw)
 {
     (void)hw;
     s_regs[SDIO_REG_PWR_CTRL / 4] = 0x0Fu;
     printf("  [hal] power ON\n");
     return 0;
 }
 static void stub_power_off(uiox_sd_hw_t *hw)
 {
     (void)hw;
     s_regs[SDIO_REG_PWR_CTRL / 4] = 0x00u;
     printf("  [hal] power OFF\n");
 }
 
 static uint32_t stub_reg_read(uiox_sd_hw_t *hw, uint32_t off)
 {
     (void)hw;
     uint32_t idx = (off >> 2u) & 0x1Fu;
     uint32_t val = s_regs[idx];
     /* Synthesise PRESENT_STATE */
     if (off == SDIO_REG_PRESENT_STATE) {
         val = 0u;
         if (s_card_inserted) val |= SDIO_PS_CARD_INSERTED;
         if (s_write_protect)  val |= SDIO_PS_WRITE_PROTECT;
         val |= SDIO_PS_SPACE_AVAILABLE | SDIO_PS_DATA_AVAILABLE;
     }
     return val;
 }
 
 static void stub_reg_write(uiox_sd_hw_t *hw, uint32_t off, uint32_t val)
 {
     (void)hw;
     uint32_t idx = (off >> 2u) & 0x1Fu;
     /* Writing INT_STATUS clears bits (W1C) */
     if (off == SDIO_REG_INT_STATUS)
         s_regs[idx] &= ~val;
     else
         s_regs[idx] = val;
     if (off == SDIO_REG_INT_SIGNAL_EN)
         printf("  [hal] INT_SIGNAL_EN <- 0x%08X\n", val);
 }
 
 static int stub_set_clock(uiox_sd_hw_t *hw, uint32_t hz)
 {
     (void)hw;
     printf("  [hal] set_clock %u Hz\n", hz);
     s_regs[SDIO_REG_CLK_CTRL / 4] = SDIO_CLK_INT_CLK_EN |
                                       SDIO_CLK_INT_CLK_STABLE |
                                       SDIO_CLK_SD_CLK_EN;
     return 0;
 }
 
 static int stub_set_bus_width(uiox_sd_hw_t *hw, uint8_t w)
 {
     (void)hw;
     printf("  [hal] set_bus_width %u-bit\n", w);
     if (w == 4u)
         s_regs[SDIO_REG_HOST_CTRL1 / 4] |= SDIO_HC1_4BIT_MODE;
     else
         s_regs[SDIO_REG_HOST_CTRL1 / 4] &= ~SDIO_HC1_4BIT_MODE;
     return 0;
 }
 
 static int stub_send_cmd(uiox_sd_hw_t *hw, uint8_t cmd,
                           uint32_t arg, uiox_sd_resp_t resp_type,
                           uint32_t *resp)
 {
     (void)hw; (void)arg;
     if (resp) memset(resp, 0, 4 * sizeof(uint32_t));
 
     printf("  [hal] CMD%-2u  arg=0x%08X  resp=%d\n", cmd, arg, resp_type);
 
     if (s_sim_error) { s_sim_error = false; return -EIO; }
 
     switch (cmd) {
     case SD_CMD0_GO_IDLE:
         break;
     case SD_CMD8_SEND_IF_COND:
         if (resp) resp[0] = arg; /* echo back */
         break;
     case SD_CMD2_ALL_SEND_CID:
         if (resp) memcpy(resp, s_cid, 16u);
         break;
     case SD_CMD3_SEND_REL_ADDR:
         if (resp) resp[0] = 0xAAAA0000u; /* RCA = 0xAAAA */
         break;
     case SD_CMD9_SEND_CSD:
         if (resp) memcpy(resp, s_csd_sdhc, 16u);
         break;
     case SD_CMD7_SELECT_CARD:
     case SD_CMD16_SET_BLOCKLEN:
     case SD_CMD55_APP_CMD:
     case SD_ACMD6_SET_BUS_WIDTH:
     case SD_CMD6_SWITCH_FUNC:
     case 32u: case 33u: case 38u:
         if (resp) resp[0] = 0x00000900u; /* R1: ready, no error */
         break;
     case SD_ACMD41_SD_SEND_OP_COND:
         /* Card powered up: CCS=1 (SDHC) */
         if (resp) resp[0] = SD_ACMD41_OCR_BUSY | SD_ACMD41_CCS |
                              0x00FF8000u;
         break;
     case SD_CMD13_SEND_STATUS:
         if (resp) resp[0] = 0x00000900u;
         break;
     case SD_CMD17_READ_SINGLE_BLOCK:
     case SD_CMD24_WRITE_BLOCK:
         if (resp) resp[0] = 0x00000900u;
         break;
     default:
         if (resp) resp[0] = 0u;
         break;
     }
     return 0;
 }
 
 static int stub_read_blocks(uiox_sd_hw_t *hw, uint32_t lba,
                              uint8_t *buf, uint32_t count)
 {
     (void)hw;
     uint32_t max_lba = (uint32_t)(sizeof(s_card_mem) / UIOX_SD_BLOCK_SIZE);
     if (lba + count > max_lba) return -ERANGE;
     memcpy(buf, &s_card_mem[lba * UIOX_SD_BLOCK_SIZE],
            count * UIOX_SD_BLOCK_SIZE);
     printf("  [hal] read_blocks  lba=%u  count=%u\n", lba, count);
     return 0;
 }
 
 static int stub_write_blocks(uiox_sd_hw_t *hw, uint32_t lba,
                               const uint8_t *buf, uint32_t count)
 {
     (void)hw;
     if (s_write_protect) return -EROFS;
     uint32_t max_lba = (uint32_t)(sizeof(s_card_mem) / UIOX_SD_BLOCK_SIZE);
     if (lba + count > max_lba) return -ERANGE;
     memcpy(&s_card_mem[lba * UIOX_SD_BLOCK_SIZE], buf,
            count * UIOX_SD_BLOCK_SIZE);
     printf("  [hal] write_blocks lba=%u  count=%u\n", lba, count);
     return 0;
 }
 
 static int stub_dma_read(uiox_sd_hw_t *hw, uint32_t lba,
                           uintptr_t phys, uint32_t count)
 {
     (void)hw; (void)phys;
     printf("  [hal] dma_read  lba=%u  count=%u  phys=0x%lX\n",
            lba, count, (unsigned long)phys);
     return 0;
 }
 
 static int stub_dma_write(uiox_sd_hw_t *hw, uint32_t lba,
                            uintptr_t phys, uint32_t count)
 {
     (void)hw; (void)phys;
     printf("  [hal] dma_write lba=%u  count=%u  phys=0x%lX\n",
            lba, count, (unsigned long)phys);
     return 0;
 }
 
 /* SW CRC7 (polynomial 0x09) */
 static uint8_t stub_crc7(const uint8_t *data, uint8_t len)
 {
     uint8_t crc = 0u;
     for (uint8_t i = 0u; i < len; i++) {
         crc ^= data[i];
         for (uint8_t b = 0u; b < 8u; b++)
             crc = (crc & 0x80u) ? ((crc << 1u) ^ 0x09u) : (crc << 1u);
     }
     return crc >> 1u;
 }
 
 /* SW CRC16-CCITT (polynomial 0x1021) */
 static uint16_t stub_crc16(const uint8_t *data, uint32_t len)
 {
     uint16_t crc = 0u;
     for (uint32_t i = 0u; i < len; i++) {
         crc ^= (uint16_t)data[i] << 8u;
         for (uint8_t b = 0u; b < 8u; b++)
             crc = (crc & 0x8000u)
                   ? ((uint16_t)(crc << 1u) ^ 0x1021u)
                   : (uint16_t)(crc << 1u);
     }
     return crc;
 }
 
 static void stub_gpio_w(uiox_sd_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 
 static bool stub_gpio_r(uiox_sd_hw_t *hw, uint32_t p)
 {
     (void)hw;
     if (p == UIOX_SD_GPIO_CD) return !s_card_inserted; /* active-low */
     if (p == UIOX_SD_GPIO_WP) return  s_write_protect;
     return false;
 }
 
 static void stub_isr(uiox_sd_hw_t *hw)
 {
     if (!hw) return;
     if (s_sim_insert) {
         s_card_inserted = true;
         hw->pending_irq |= UIOX_SD_IRQ_CARD_INSERT;
         printf("  [hal] ISR: CARD_INSERT\n");
     } else if (s_sim_remove) {
         s_card_inserted = false;
         hw->pending_irq |= UIOX_SD_IRQ_CARD_REMOVE;
         printf("  [hal] ISR: CARD_REMOVE\n");
     } else if (s_sim_error) {
         hw->pending_irq |= UIOX_SD_IRQ_ERROR;
         printf("  [hal] ISR: ERROR\n");
     } else {
         hw->pending_irq |= UIOX_SD_IRQ_XFER_DONE;
     }
 }
 
 static const uiox_sd_hw_ops_t stub_ops = {
     .init         = stub_init,
     .deinit       = stub_deinit,
     .power_on     = stub_power_on,
     .power_off    = stub_power_off,
     .reg_read     = stub_reg_read,
     .reg_write    = stub_reg_write,
     .set_clock    = stub_set_clock,
     .set_bus_width= stub_set_bus_width,
     .send_cmd     = stub_send_cmd,
     .read_blocks  = stub_read_blocks,
     .write_blocks = stub_write_blocks,
     .dma_read     = stub_dma_read,
     .dma_write    = stub_dma_write,
     .crc7         = stub_crc7,
     .crc16        = stub_crc16,
     .gpio_write   = stub_gpio_w,
     .gpio_read    = stub_gpio_r,
     .isr          = stub_isr,
 };
 
 static uiox_sd_hw_t s_hw = {
     .base        = 0xFE340000uL,
     .irq         = 79u,
     .caps        = UIOX_SD_CAP_SDIO_4BIT | UIOX_SD_CAP_DMA          |
                    UIOX_SD_CAP_HIGH_SPEED | UIOX_SD_CAP_CD_GPIO      |
                    UIOX_SD_CAP_WP_GPIO    | UIOX_SD_CAP_CRC_HW       |
                    UIOX_SD_CAP_MULTIBLOCK | UIOX_SD_CAP_AUTO_CMD12,
     .bus_type    = UIOX_SD_BUS_SDIO_4BIT,
     .model       = "Raspberry Pi 4B SDIO Host Controller",
     .clk_id_hz   = 400000u,
     .clk_xfer_hz = 50000000u,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_sd_event(uiox_sd_ev_t ev,
                          const uiox_sd_evt_t *data, void *ctx)
 {
     (void)ctx;
     if (data)
         printf("  [event] %-16s  lba=%u  count=%u  status=%d\n",
                uiox_sd_ev_name(ev), data->lba, data->count, data->status);
     else
         printf("  [event] %-16s\n", uiox_sd_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX SD Card Reader SDIO/SPI Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open (no card) ---\n");
     uiox_sd_device_t      dev;
     uiox_sd_open_params_t p = {
         .hw      = &s_hw,
         .hw_ops  = &stub_ops,
         .evt_cb  = on_sd_event,
     };
     int rc = uiox_sd_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start (no card present) ---\n");
     rc = uiox_sd_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_sd_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info (no card) ---\n");
     uiox_sd_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate card insert ---\n");
     s_sim_insert = true;
     stub_isr(&s_hw);
     s_sim_insert = false;
     for (uint32_t t = 10u; t <= 30u; t += 10u)
         uiox_sd_tick(&dev, t);
     printf("  State: %s\n", uiox_sd_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Card info after init ---\n");
     uiox_sd_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write block (LBA 0) ---\n");
     static uint8_t tx_buf[UIOX_SD_BLOCK_SIZE];
     for (uint32_t i = 0u; i < UIOX_SD_BLOCK_SIZE; i++)
         tx_buf[i] = (uint8_t)(i & 0xFFu);
     rc = uiox_sd_write(&dev, 0u, tx_buf, 1u);
     printf("  write rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read back block (LBA 0) ---\n");
     static uint8_t rx_buf[UIOX_SD_BLOCK_SIZE];
     rc = uiox_sd_read(&dev, 0u, rx_buf, 1u);
     printf("  read rc=%d\n", rc);
     bool match = (memcmp(tx_buf, rx_buf, UIOX_SD_BLOCK_SIZE) == 0);
     printf("  Data match: %s\n", match ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Multi-block write (LBA 1–4) ---\n");
     static uint8_t tx_multi[4 * UIOX_SD_BLOCK_SIZE];
     memset(tx_multi, 0xBBu, sizeof(tx_multi));
     rc = uiox_sd_write(&dev, 1u, tx_multi, 4u);
     printf("  multi-write rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Multi-block read (LBA 1–4) ---\n");
     static uint8_t rx_multi[4 * UIOX_SD_BLOCK_SIZE];
     rc = uiox_sd_read(&dev, 1u, rx_multi, 4u);
     printf("  multi-read rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Erase (LBA 10–19) ---\n");
     rc = uiox_sd_erase(&dev, 10u, 19u);
     printf("  erase rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Switch to high-speed mode ---\n");
     rc = uiox_sd_proto_switch_hs(&dev.subsys.proto);
     printf("  switch_hs rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- CRC7 / CRC16 verification ---\n");
     static const uint8_t crc_data[] = { 0x40, 0x00, 0x00, 0x00, 0x00 };
     uint8_t  c7  = uiox_sd_hw_crc7(&s_hw, crc_data, sizeof(crc_data));
     uint16_t c16 = uiox_sd_hw_crc16(&s_hw, rx_buf, 16u);
     printf("  CRC7(CMD0 frame) = 0x%02X  (expected 0x4A)\n", c7);
     printf("  CRC16(first 16 bytes) = 0x%04X\n", c16);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate write-protect active ---\n");
     s_write_protect     = true;
     s_sim_wp_change     = true;
     /* Force WP bit in present state — next tick will detect */
     s_hw.write_protect  = false;  /* was clear; tick will see change */
     uiox_sd_tick(&dev, 40u);
     rc = uiox_sd_write(&dev, 0u, tx_buf, 1u);
     printf("  Write attempt on WP card: rc=%d (%s)\n",
            rc, (rc == -EROFS) ? "EROFS — correct" : "unexpected");
     s_write_protect = false;
     s_hw.write_protect = false;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Block buffer pool alloc/free ---\n");
     uiox_sd_block_t *blk = uiox_sd_block_alloc();
     if (blk) {
        printf("  Block alloc OK  lba=%u  free_after=%u\n",
               blk->lba, uiox_sd_block_free_cnt());
        /* Fill with test pattern */
        memset(blk->data, 0xCDu, UIOX_SD_BLOCK_SIZE);
        blk->lba   = 99u;
        blk->len   = UIOX_SD_BLOCK_SIZE;
        blk->dirty = true;
        /* Write-back through stack */
        rc = uiox_sd_write(&dev, blk->lba, blk->data, 1u);
        printf("  Write-back dirty block: rc=%d\n", rc);
        uiox_sd_block_free(blk);
        printf("  Block freed  free_after=%u\n",
               uiox_sd_block_free_cnt());
    } else {
        printf("  Block alloc FAILED (pool empty)\n");
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Command pool alloc/free ---\n");
    uiox_sd_cmd_t *cmd = uiox_sd_cmd_alloc();
    if (cmd) {
        cmd->cmd_idx   = SD_CMD13_SEND_STATUS;
        cmd->arg       = (uint32_t)s_hw.card.rca << 16u;
        cmd->resp_type = SD_RESP_R1;
        rc = uiox_sd_if_send_cmd(&dev.subsys.sif,
                                  cmd->cmd_idx, cmd->arg,
                                  cmd->resp_type, cmd->resp);
        cmd->status = rc;
        printf("  CMD13 status resp=0x%08X  rc=%d\n",
               cmd->resp[0], rc);
        uiox_sd_cmd_free(cmd);
        printf("  Cmd pool free: %u / %u\n",
               uiox_sd_cmd_free_cnt(), UIOX_SD_CMD_POOL_SIZE);
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate card removal ---\n");
    s_sim_remove = true;
    stub_isr(&s_hw);
    s_sim_remove = false;
    uiox_sd_tick(&dev, 50u);
    printf("  State: %s\n", uiox_sd_state_name(dev.subsys.state));
    printf("  Card present: %s\n",
           uiox_sd_is_present(&dev) ? "YES" : "NO");

    /* ------------------------------------------------------------------ */
    printf("\n--- Read attempt with no card ---\n");
    rc = uiox_sd_read(&dev, 0u, rx_buf, 1u);
    printf("  Read rc=%d (%s)\n",
           rc, (rc == -ENODEV) ? "ENODEV — correct" : "unexpected");

    /* ------------------------------------------------------------------ */
    printf("\n--- Re-insert card ---\n");
    s_sim_insert = true;
    stub_isr(&s_hw);
    s_sim_insert = false;
    for (uint32_t t = 60u; t <= 80u; t += 10u)
        uiox_sd_tick(&dev, t);
    printf("  State: %s\n", uiox_sd_state_name(dev.subsys.state));

    /* ------------------------------------------------------------------ */
    printf("\n--- Simulate host controller error ---\n");
    s_sim_error       = true;
    s_hw.pending_irq |= UIOX_SD_IRQ_ERROR;
    uiox_sd_tick(&dev, 90u);
    printf("  State after error: %s\n",
           uiox_sd_state_name(dev.subsys.state));

    /* ------------------------------------------------------------------ */
    printf("\n--- Tick loop (5 × 10 ms, steady state) ---\n");
    /* Recover: re-init after error */
    dev.subsys.state = UIOX_SD_STATE_READY;
    dev.subsys.proto.initialized = true;
    s_sim_error = false;
    for (uint32_t t = 100u; t <= 140u; t += 10u)
        uiox_sd_tick(&dev, t);

    /* ------------------------------------------------------------------ */
    printf("\n--- Capacity and card info ---\n");
    printf("  Capacity       : %llu MB\n",
           (unsigned long long)(uiox_sd_capacity_bytes(&dev) >> 20u));
    printf("  Blocks         : %llu\n",
           (unsigned long long) uiox_sd_capacity_blocks(&dev));
    printf("  Write-protect  : %s\n",
           uiox_sd_is_write_prot(&dev) ? "YES" : "NO");
    {
        const uiox_sd_card_t *ci = uiox_sd_card_info(&dev);
        if (ci) {
            printf("  Card type      : %s\n",
                   uiox_sd_card_type_name(ci->card_type));
            printf("  RCA            : 0x%04X\n", ci->rca);
            printf("  OCR            : 0x%08X\n", ci->ocr);
            printf("  CID[0..3]      : %02X %02X %02X %02X\n",
                   ci->cid[0], ci->cid[1], ci->cid[2], ci->cid[3]);
        }
    }

    /* ------------------------------------------------------------------ */
    printf("\n--- Statistics ---\n");
    uiox_sd_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    printf("\n--- Final device info ---\n");
    uiox_sd_print_info(&dev);

    /* ------------------------------------------------------------------ */
    printf("\n--- Stop and close ---\n");
    uiox_sd_stop(&dev);
    printf("  State: %s\n", uiox_sd_state_name(dev.subsys.state));
    uiox_sd_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX SD Card Reader SDIO/SPI Demo complete ===\n");
    return 0;
}
 