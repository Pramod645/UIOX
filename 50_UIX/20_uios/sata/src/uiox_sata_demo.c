/**
 * @file  uiox_sata_demo.c
 * @brief UIOX SATA III stack demo — stub AHCI HAL + full stack exercise.
 *        Mirrors uiox_sd_demo.c in structure.
 * @date  2026-06-12
 */

 #include "uiox_sata_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub AHCI register bank and device storage
  * ====================================================================== */
 
 /* GHC register space (indexed by offset / 4) */
 static uint32_t s_ghc[0x80 / 4];
 
 /* Per-port register space (port 0 only, indexed by offset / 4) */
 static uint32_t s_px[0x80 / 4];
 
 /* Simulated disk storage: 64 MB = 131072 × 512 B sectors */
 #define SIM_DISK_SECTORS  131072u
 static uint8_t s_disk[SIM_DISK_SECTORS * UIOX_SATA_SECTOR_SIZE];
 
 /* SMART data buffer (512 bytes) */
 static uint8_t s_smart_buf[512];
 
 /* Simulation flags */
 static bool s_dev_attached     = false;
 static bool s_sim_attach       = false;
 static bool s_sim_detach       = false;
 static bool s_sim_error        = false;
 static bool s_sim_ncq_done     = false;
 static bool s_sim_smart_warn   = false;
 
 /*
  * Canned IDENTIFY response (512 bytes / 256 words).
  * We fill key fields; unused words stay 0.
  */
 static uint16_t s_identify[ATA_ID_WORDS];
 
 static void stub_identify_preset(void)
 {
     memset(s_identify, 0, sizeof(s_identify));
 
     /* Words 10–19: serial number "SN-UIOX-SSD-001" (byte-swapped) */
     const char *serial = "SN-UIOX-SSD-001     ";   /* 20 chars */
     for (int i = 0; i < 10; i++) {
         s_identify[ATA_ID_SERIAL + i] =
             (uint16_t)((uint8_t)serial[i*2] << 8u) |
             (uint8_t)serial[i*2 + 1];
     }
 
     /* Words 23–26: firmware "FW01.00 " */
     const char *fw = "FW01.00 ";
     for (int i = 0; i < 4; i++) {
         s_identify[ATA_ID_FW_REV + i] =
             (uint16_t)((uint8_t)fw[i*2] << 8u) |
             (uint8_t)fw[i*2 + 1];
     }
 
     /* Words 27–46: model "UIOX SATA III SSD 64GB          " */
     const char *model =
         "UIOX SATA III SSD 64GB          "
         "        ";                            /* 40 chars */
     for (int i = 0; i < 20; i++) {
         s_identify[ATA_ID_MODEL + i] =
             (uint16_t)((uint8_t)model[i*2] << 8u) |
             (uint8_t)model[i*2 + 1];
     }
 
     /* Word 49: capabilities — LBA supported */
     s_identify[49] = 0x0200u;
 
     /* Word 75: NCQ depth - 1 = 31 (32 tags) */
     s_identify[ATA_ID_QUEUE_DEPTH] = 31u;
 
     /* Word 76: SATA caps — NCQ supported, Gen3 */
     s_identify[ATA_ID_SATA_CAPS] = 0x010Eu;
 
     /* Word 82: SMART supported */
     s_identify[82] = 0x4001u;
 
     /* Word 83: 48-bit LBA supported */
     s_identify[83] = 0x4400u;
 
     /* Word 169: TRIM / DSM supported */
     s_identify[169] = 0x0001u;
 
     /* Words 100–103: 48-bit LBA total sectors = SIM_DISK_SECTORS */
     s_identify[ATA_ID_LBA48_SECTORS]     = (uint16_t)( SIM_DISK_SECTORS        & 0xFFFFu);
     s_identify[ATA_ID_LBA48_SECTORS + 1] = (uint16_t)((SIM_DISK_SECTORS >> 16) & 0xFFFFu);
     s_identify[ATA_ID_LBA48_SECTORS + 2] = 0u;
     s_identify[ATA_ID_LBA48_SECTORS + 3] = 0u;
 
     /* Word 217: rotation rate 0x0001 = non-rotating (SSD) */
     s_identify[ATA_ID_RPM] = 0x0001u;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_sata_hw_t *hw)
 {
     (void)hw;
     memset(s_ghc, 0, sizeof(s_ghc));
     memset(s_px,  0, sizeof(s_px));
     memset(s_disk, 0xA5u, sizeof(s_disk));
     memset(s_smart_buf, 0, sizeof(s_smart_buf));
     /* Preset SMART data: attribute 5 (reallocated sectors) = 0 */
     s_smart_buf[0] = 0x10u;  /* data structure revision */
     stub_identify_preset();
 
     /* GHC: AHCI enabled, version 1.3.1 */
     s_ghc[AHCI_GHC_GHC / 4] = AHCI_GHC_GHC_AHCI_EN;
     s_ghc[AHCI_GHC_VS  / 4] = 0x00010301u;  /* v1.3.1 */
     s_ghc[AHCI_GHC_PI  / 4] = 0x00000001u;  /* port 0 implemented */
     s_ghc[AHCI_GHC_CAP / 4] = AHCI_CAP_S64A | AHCI_CAP_SNCQ |
                                 (31u << AHCI_CAP_NCS_SHIFT) | /* 32 cmd slots */
                                 0u;           /* 1 port */
 
     printf("  [hal] init  %s  BAR5=0x%08lX  IRQ=%u  port=%u\n",
            hw->model, (unsigned long)hw->bar5, hw->irq, hw->port);
     return 0;
 }
 
 static void stub_deinit(uiox_sata_hw_t *hw) { (void)hw; }
 
 static int stub_port_start(uiox_sata_hw_t *hw, uint8_t port)
 {
     (void)hw; (void)port;
     /* FRE + ST */
     s_px[AHCI_PX_CMD / 4] |= AHCI_PX_CMD_FRE | AHCI_PX_CMD_ST |
                                AHCI_PX_CMD_SUD | AHCI_PX_CMD_POD;
     /* Simulate device attached at startup */
     if (s_dev_attached)
         s_px[AHCI_PX_SSTS / 4] = AHCI_PX_SSTS_DET_COMM |
                                    AHCI_PX_SSTS_SPD_GEN3;
     else
         s_px[AHCI_PX_SSTS / 4] = AHCI_PX_SSTS_DET_NONE;
 
     s_px[AHCI_PX_SIG / 4] = s_dev_attached ? AHCI_SIG_ATA : 0xFFFFFFFFu;
     printf("  [hal] port_start  port=%u  SSTS=0x%08X\n",
            port, s_px[AHCI_PX_SSTS / 4]);
     return 0;
 }
 
 static void stub_port_stop(uiox_sata_hw_t *hw, uint8_t port)
 {
     (void)hw; (void)port;
     s_px[AHCI_PX_CMD / 4] &=
         ~(AHCI_PX_CMD_ST | AHCI_PX_CMD_FRE);
     printf("  [hal] port_stop  port=%u\n", port);
 }
 
 static uint32_t stub_ghc_read(uiox_sata_hw_t *hw, uint32_t off)
 {
     (void)hw;
     return s_ghc[(off >> 2u) & 0x1Fu];
 }
 
 static void stub_ghc_write(uiox_sata_hw_t *hw, uint32_t off, uint32_t val)
 {
     (void)hw;
     uint32_t idx = (off >> 2u) & 0x1Fu;
     /* GHC IS is W1C */
     if (off == AHCI_GHC_IS) s_ghc[idx] &= ~val;
     else                     s_ghc[idx]  =  val;
 }
 
 static uint32_t stub_px_read(uiox_sata_hw_t *hw,
                               uint8_t port, uint32_t off)
 {
     (void)hw; (void)port;
     /* Synthesise SSTS from simulation state */
     if (off == AHCI_PX_SSTS) {
         return s_dev_attached
                ? (AHCI_PX_SSTS_DET_COMM | AHCI_PX_SSTS_SPD_GEN3)
                : AHCI_PX_SSTS_DET_NONE;
     }
     return s_px[(off >> 2u) & 0x1Fu];
 }
 
 static void stub_px_write(uiox_sata_hw_t *hw,
                            uint8_t port, uint32_t off, uint32_t val)
 {
     (void)hw; (void)port;
     /* PxIS is W1C */
     if (off == AHCI_PX_IS) s_px[(off >> 2u) & 0x1Fu] &= ~val;
     else                    s_px[(off >> 2u) & 0x1Fu]  =  val;
     if (off == AHCI_PX_IE)
         printf("  [hal] PxIE <- 0x%08X\n", val);
 }
 
 static int stub_cmd_issue(uiox_sata_hw_t *hw, uint8_t port,
                            uint8_t slot,
                            const uiox_sata_fis_h2d_t *fis,
                            bool write, uintptr_t data_phys,
                            uint32_t len)
 {
     (void)hw; (void)port; (void)data_phys; (void)len;
     printf("  [hal] cmd_issue  slot=%u  cmd=0x%02X  write=%d\n",
            slot, fis->command, (int)write);
     /* Set CI bit, will be cleared when "complete" */
     s_px[AHCI_PX_CI / 4] |= (1u << slot);
     /* Signal D2H IRQ */
     hw->pending_irq |= UIOX_SATA_IRQ_D2H;
     return 0;
 }
 
 static int stub_read_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                               uint8_t *buf, uint32_t count)
 {
     (void)hw;
 
     /* Special case: count == 0 means IDENTIFY */
     if (count == 0u) {
         memcpy(buf, s_identify, sizeof(s_identify));
         printf("  [hal] IDENTIFY\n");
         return 0;
     }
 
     if (lba + count > SIM_DISK_SECTORS) return -ERANGE;
     memcpy(buf, &s_disk[lba * UIOX_SATA_SECTOR_SIZE],
            (size_t)count * UIOX_SATA_SECTOR_SIZE);
     printf("  [hal] read_sectors  lba=%llu  count=%u\n",
            (unsigned long long)lba, count);
     return 0;
 }
 
 static int stub_write_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                const uint8_t *buf, uint32_t count)
 {
     (void)hw;
     if (lba + count > SIM_DISK_SECTORS) return -ERANGE;
     memcpy(&s_disk[lba * UIOX_SATA_SECTOR_SIZE], buf,
            (size_t)count * UIOX_SATA_SECTOR_SIZE);
     printf("  [hal] write_sectors lba=%llu  count=%u\n",
            (unsigned long long)lba, count);
     return 0;
 }
 
 static int stub_ncq_read(uiox_sata_hw_t *hw, uint64_t lba,
                           uint8_t *buf, uint32_t count, uint8_t tag)
 {
     printf("  [hal] NCQ read  lba=%llu  count=%u  tag=%u\n",
            (unsigned long long)lba, count, tag);
     return stub_read_sectors(hw, lba, buf, count);
 }
 
 static int stub_ncq_write(uiox_sata_hw_t *hw, uint64_t lba,
                            const uint8_t *buf, uint32_t count, uint8_t tag)
 {
     printf("  [hal] NCQ write lba=%llu  count=%u  tag=%u\n",
            (unsigned long long)lba, count, tag);
     return stub_write_sectors(hw, lba, buf, count);
 }
 
 static int stub_port_reset(uiox_sata_hw_t *hw, uint8_t port)
 {
     (void)hw; (void)port;
     printf("  [hal] COMRESET  port=%u\n", port);
     /* After reset, re-check device presence */
     s_px[AHCI_PX_SSTS / 4] = s_dev_attached
                                ? (AHCI_PX_SSTS_DET_COMM |
                                   AHCI_PX_SSTS_SPD_GEN3)
                                : AHCI_PX_SSTS_DET_NONE;
     s_px[AHCI_PX_SIG  / 4] = s_dev_attached
                                ? AHCI_SIG_ATA
                                : 0xFFFFFFFFu;
     hw->sif_stats_resets_inc = true;  /* signal reset to IF */
     return 0;
 }
 
 static void stub_gpio_w(uiox_sata_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 
 static bool stub_gpio_r(uiox_sata_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return false; }
 
 static int stub_smart_read(uiox_sata_hw_t *hw, uint8_t *buf)
 {
     (void)hw;
     memcpy(buf, s_smart_buf, 512u);
     printf("  [hal] SMART read\n");
     if (s_sim_smart_warn) {
         printf("  [hal] SMART: pre-fail attribute detected!\n");
         hw->pending_irq |= UIOX_SATA_IRQ_ERROR;  /* signal upper layers */
     }
     return 0;
 }
 
 static void stub_isr(uiox_sata_hw_t *hw)
 {
     if (!hw) return;
     if (s_sim_attach) {
         s_dev_attached = true;
         hw->pending_irq |= UIOX_SATA_IRQ_HOTPLUG;
         s_px[AHCI_PX_IS / 4] |= AHCI_PX_IS_PCS;
         printf("  [hal] ISR: DEV_ATTACH\n");
     } else if (s_sim_detach) {
         s_dev_attached = false;
         hw->pending_irq |= UIOX_SATA_IRQ_HOTPLUG;
         s_px[AHCI_PX_IS / 4] |= AHCI_PX_IS_PCS;
         printf("  [hal] ISR: DEV_DETACH\n");
     } else if (s_sim_error) {
         hw->pending_irq |= UIOX_SATA_IRQ_ERROR;
         s_px[AHCI_PX_IS / 4] |= AHCI_PX_IS_TFES;
         printf("  [hal] ISR: ERROR\n");
     } else if (s_sim_ncq_done) {
         hw->pending_irq |= UIOX_SATA_IRQ_NCQ_DONE;
         s_px[AHCI_PX_IS / 4] |= AHCI_PX_IS_SDBS;
         /* Clear all NCQ active tags */
         hw->ncq_active = 0u;
         printf("  [hal] ISR: NCQ_DONE\n");
     } else {
         hw->pending_irq |= UIOX_SATA_IRQ_DMA_DONE;
         s_px[AHCI_PX_IS / 4] |= AHCI_PX_IS_DHRS;
     }
     /* Set GHC IS bit for port 0 */
     s_ghc[AHCI_GHC_IS / 4] |= 0x00000001u;
 }
 
 static const uiox_sata_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .port_start    = stub_port_start,
     .port_stop     = stub_port_stop,
     .ghc_read      = stub_ghc_read,
     .ghc_write     = stub_ghc_write,
     .px_read       = stub_px_read,
     .px_write      = stub_px_write,
     .cmd_issue     = stub_cmd_issue,
     .read_sectors  = stub_read_sectors,
     .write_sectors = stub_write_sectors,
     .ncq_read      = stub_ncq_read,
     .ncq_write     = stub_ncq_write,
     .port_reset    = stub_port_reset,
     .gpio_write    = stub_gpio_w,
     .gpio_read     = stub_gpio_r,
     .smart_read    = stub_smart_read,
     .isr           = stub_isr,
 };
 
 static uiox_sata_hw_t s_hw = {
     .bar5       = 0xF0600000uL,
     .irq        = 19u,
     .caps       = UIOX_SATA_CAP_NCQ      | UIOX_SATA_CAP_SATA3    |
                   UIOX_SATA_CAP_AHCI     | UIOX_SATA_CAP_DMA       |
                   UIOX_SATA_CAP_48BIT_LBA| UIOX_SATA_CAP_SMART     |
                   UIOX_SATA_CAP_TRIM     | UIOX_SATA_CAP_DEVSLP    |
                   UIOX_SATA_CAP_HOTPLUG  | UIOX_SATA_CAP_ALPM      |
                   UIOX_SATA_CAP_LED,
     .ctrl_type  = UIOX_SATA_CTRL_AHCI,
     .model      = "Intel 9-Series AHCI SATA III Controller",
     .port       = 0u,
     .num_ports  = 1u,
     .dev_type   = UIOX_SATA_DEV_ATA,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_sata_event(uiox_sata_ev_t ev,
                            const uiox_sata_evt_t *data, void *ctx)
 {
     (void)ctx;
     if (data)
         printf("  [event] %-16s  lba=%llu  sectors=%u"
                "  status=%d  err=0x%08X\n",
                uiox_sata_ev_name(ev),
                (unsigned long long)data->lba,
                data->sector_count,
                data->status,
                data->error_reg);
     else
         printf("  [event] %-16s\n", uiox_sata_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX SATA III Controller Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open (no device) ---\n");
     uiox_sata_device_t       dev;
     uiox_sata_open_params_t  p = {
         .hw      = &s_hw,
         .hw_ops  = &stub_ops,
         .evt_cb  = on_sata_event,
     };
     int rc = uiox_sata_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start (no device attached) ---\n");
     rc = uiox_sata_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_sata_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate device attach ---\n");
     s_sim_attach = true;
     stub_isr(&s_hw);
     s_sim_attach = false;
     for (uint32_t t = 10u; t <= 30u; t += 10u)
         uiox_sata_tick(&dev, t);
     printf("  State: %s\n", uiox_sata_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info after IDENTIFY ---\n");
     uiox_sata_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write sectors (LBA 0, 8 sectors = 4 KB) ---\n");
     static uint8_t tx_buf[8 * UIOX_SATA_SECTOR_SIZE];
     for (uint32_t i = 0u; i < sizeof(tx_buf); i++)
         tx_buf[i] = (uint8_t)(i & 0xFFu);
     rc = uiox_sata_write(&dev, 0u, tx_buf, 8u);
     printf("  write rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read back sectors (LBA 0, 8 sectors) ---\n");
     static uint8_t rx_buf[8 * UIOX_SATA_SECTOR_SIZE];
     rc = uiox_sata_read(&dev, 0u, rx_buf, 8u);
     printf("  read rc=%d\n", rc);
     bool match = (memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0);
     printf("  Data match: %s\n", match ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Flush write cache ---\n");
     rc = uiox_sata_flush(&dev);
     printf("  flush rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- NCQ write (LBA 100, 4 sectors, tag=0) ---\n");
     static uint8_t ncq_tx[4 * UIOX_SATA_SECTOR_SIZE];
     memset(ncq_tx, 0xBBu, sizeof(ncq_tx));
     rc = uiox_sata_proto_ncq_write(&dev.subsys.proto, 100u,
                                     ncq_tx, 4u);
     printf("  ncq_write rc=%d  ncq_active=0x%08X\n",
            rc, s_hw.ncq_active);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate NCQ completion ---\n");
     s_sim_ncq_done = true;
     stub_isr(&s_hw);
     s_sim_ncq_done = false;
     uiox_sata_tick(&dev, 40u);
     printf("  ncq_active after done=0x%08X\n", s_hw.ncq_active);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- NCQ read (LBA 100, 4 sectors) ---\n");
     static uint8_t ncq_rx[4 * UIOX_SATA_SECTOR_SIZE];
     rc = uiox_sata_proto_ncq_read(&dev.subsys.proto, 100u,
                                    ncq_rx, 4u);
     printf("  ncq_read rc=%d\n", rc);
     bool ncq_match = (memcmp(ncq_tx, ncq_rx, sizeof(ncq_tx)) == 0);
     printf("  NCQ Data match: %s\n", ncq_match ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- TRIM (LBA 200, 64 sectors) ---\n");
     rc = uiox_sata_trim(&dev, 200u, 64u);
     printf("  trim rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- SMART read ---\n");
     static uint8_t smart_data[512];
     rc = uiox_sata_smart_read(&dev, smart_data);
     printf("  smart_read rc=%d  data[0]=0x%02X\n",
            rc, smart_data[0]);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate SMART pre-fail warning ---\n");
     s_sim_smart_warn = true;
     rc = uiox_sata_smart_read(&dev, smart_data);
     printf("  smart_read (warn) rc=%d\n", rc);
     s_sim_smart_warn = false;
     s_hw.pending_irq = 0u;  /* clear the injected error IRQ */
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Block buffer pool alloc / write-back ---\n");
     uiox_sata_blk_t *blk = uiox_sata_blk_alloc();
     if (blk) {
         memset(blk->data, 0xCDu,
                UIOX_SATA_SECTORS_PER_BLK * UIOX_SATA_SECTOR_SIZE);
         blk->lba     = 500u;
         blk->sectors = UIOX_SATA_SECTORS_PER_BLK;
         blk->dirty   = true;
         rc = uiox_sata_write(&dev, blk->lba, blk->data, blk->sectors);
         printf("  dirty write-back rc=%d  blk_free_before=%u\n",
                rc, uiox_sata_blk_free_cnt());
         uiox_sata_blk_free(blk);
         printf("  blk_free_after=%u\n", uiox_sata_blk_free_cnt());
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Command record alloc ---\n");
     uiox_sata_cmd_t *cmd = uiox_sata_cmd_alloc();
     if (cmd) {
         cmd->lba          = 1000u;
         cmd->sector_count = 1u;
         cmd->write        = false;
         /* Build a minimal READ DMA EXT FIS */
         cmd->fis.fis_type = FIS_TYPE_REG_H2D;
         cmd->fis.c_pm     = 0x80u;  /* C=1: command */
         cmd->fis.command  = ATA_CMD_READ_DMA_EXT;
         cmd->fis.lba0     = (uint8_t)( cmd->lba        & 0xFFu);
         cmd->fis.lba1     = (uint8_t)((cmd->lba >>  8) & 0xFFu);
         cmd->fis.lba2     = (uint8_t)((cmd->lba >> 16) & 0xFFu);
         cmd->fis.lba3     = (uint8_t)((cmd->lba >> 24) & 0xFFu);
         cmd->fis.lba4     = 0u;
         cmd->fis.lba5     = 0u;
         cmd->fis.device   = 0x40u;  /* LBA mode */
         cmd->fis.countl   = (uint8_t)cmd->sector_count;
         cmd->fis.counth   = 0u;
         rc = uiox_sata_if_issue_cmd(&dev.subsys.sif, cmd);
         printf("  cmd_issue rc=%d  slot=%u  state=%d\n",
                rc, cmd->slot, (int)cmd->state);
         uiox_sata_if_free_slot(&dev.subsys.sif, cmd->slot);
         uiox_sata_cmd_free(cmd);
         printf("  cmd_free_after=%u\n", uiox_sata_cmd_free_cnt());
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read attempt with no device (simulate detach) ---\n");
     s_sim_detach = true;
     stub_isr(&s_hw);
     s_sim_detach = false;
     uiox_sata_tick(&dev, 50u);
     printf("  State: %s\n", uiox_sata_state_name(dev.subsys.state));
     rc = uiox_sata_read(&dev, 0u, rx_buf, 8u);
     printf("  Read rc=%d (%s)\n",
            rc, (rc == -ENODEV) ? "ENODEV — correct" : "unexpected");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Re-attach device ---\n");
     s_sim_attach = true;
     stub_isr(&s_hw);
     s_sim_attach = false;
     for (uint32_t t = 60u; t <= 80u; t += 10u)
         uiox_sata_tick(&dev, t);
     printf("  State: %s\n", uiox_sata_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate host controller error ---\n");
     s_sim_error = true;
     stub_isr(&s_hw);
     s_sim_error = false;
     uiox_sata_tick(&dev, 90u);
     printf("  State after error: %s\n",
            uiox_sata_state_name(dev.subsys.state));
     /* Recover */
     dev.subsys.state             = UIOX_SATA_STATE_READY;
     dev.subsys.proto.initialized = true;
     s_hw.pending_irq             = 0u;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Power management: standby then sleep ---\n");
     rc = uiox_sata_proto_standby(&dev.subsys.proto);
     printf("  standby rc=%d\n", rc);
     rc = uiox_sata_proto_sleep(&dev.subsys.proto);
     printf("  sleep   rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Tick loop (5 × 10 ms) ---\n");
     for (uint32_t t = 100u; t <= 140u; t += 10u)
         uiox_sata_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- AHCI register read-back ---\n");
     {
         uint32_t cap = uiox_sata_hw_ghc_read(&s_hw, AHCI_GHC_CAP);
         uint32_t ver = uiox_sata_hw_ghc_read(&s_hw, AHCI_GHC_VS);
         uint32_t pi  = uiox_sata_hw_ghc_read(&s_hw, AHCI_GHC_PI);
         printf("  GHC_CAP = 0x%08X\n", cap);
         printf("  GHC_VS  = 0x%08X  (AHCI v%u.%u.%u)\n",
                ver,
                (ver >> 16u) & 0xFFu,
                (ver >>  8u) & 0xFFu,
                (ver >>  0u) & 0xFFu);
         printf("  GHC_PI  = 0x%08X  (ports implemented)\n", pi);
         uint32_t ssts = uiox_sata_hw_px_read(&s_hw, 0u, AHCI_PX_SSTS);
         uint32_t sig  = uiox_sata_hw_px_read(&s_hw, 0u, AHCI_PX_SIG);
         printf("  PxSSTS  = 0x%08X  DET=%u SPD=%u\n",
                ssts,
                (uint8_t)(ssts & AHCI_PX_SSTS_DET_MASK),
                (uint8_t)((ssts & AHCI_PX_SSTS_SPD_MASK)
                           >> AHCI_PX_SSTS_SPD_SHIFT));
         printf("  PxSIG   = 0x%08X  (%s)\n", sig,
                (sig == AHCI_SIG_ATA)   ? "ATA"   :
                (sig == AHCI_SIG_ATAPI) ? "ATAPI" : "unknown");
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Capacity and identity ---\n");
     printf("  Capacity : %llu GB\n",
            (unsigned long long)(uiox_sata_capacity(&dev) >> 30u));
     printf("  Is SSD   : %s\n", uiox_sata_is_ssd(&dev) ? "YES" : "NO");
     printf("  Present  : %s\n",
            uiox_sata_is_present(&dev) ? "YES" : "NO");
     {
         const uiox_sata_ident_t *id = uiox_sata_ident(&dev);
         if (id) {
             printf("  Model    : %.40s\n", id->model_str);
             printf("  Serial   : %.20s\n", id->serial_str);
             printf("  Firmware : %.8s\n",  id->fw_str);
         }
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_sata_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_sata_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close ---\n");
     uiox_sata_stop(&dev);
     printf("  State: %s\n", uiox_sata_state_name(dev.subsys.state));
     uiox_sata_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX SATA III Controller Demo complete ===\n");
     return 0;
 }
 