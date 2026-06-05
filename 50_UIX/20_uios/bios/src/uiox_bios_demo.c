/**
 * @file    uiox_bios_demo.c
 * @brief   UIOX BIOS stack end-to-end demonstration.
 *
 * Demonstrates: HAL init → flash detect → POST → E820 memmap →
 *   ACPI locate → EFI variable get/set → CMOS read/write →
 *   flash read/write/verify → firmware update simulation →
 *   TPM command → statistics → teardown.
 *
 * @date    2026-06-04
 */

 #include "uiox_bios_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Simulated flash memory (16 MB NOR flash)
  * ====================================================================== */
 
 #define FLASH_SIZE   (16u * 1024u * 1024u)   /* 16 MB */
 
 static uint8_t s_flash[FLASH_SIZE];          /* Simulated NOR flash        */
 
 static void flash_init_sim(void)
 {
     /* NOR flash erased state = 0xFF */
     memset(s_flash, 0xFF, sizeof(s_flash));
 
     /* Write fake BIOS signature at start of BIOS region (offset 0xE00000) */
     static const uint8_t bios_sig[] = {
         0x55, 0xAA,              /* Boot signature                        */
         'U','I','O','X','B','I','O','S',  /* Vendor string                */
         0x03, 0x02, 0x01,        /* Version 3.2.1                         */
     };
     memcpy(&s_flash[0xE00000], bios_sig, sizeof(bios_sig));
 
     /* Fake ACPI RSDP at 0xF0000 (within E820 conventional range) */
     static const char rsdp_sig[] = "RSD PTR ";
     memcpy(&s_flash[0xF0000], rsdp_sig, 8);
 
     /* CMOS: set year = 2026 (BCD), month=6, day=4 */
     s_flash[0xE0] = 0x26;   /* Year  BCD 2026 low byte */
     s_flash[0xE1] = 0x06;   /* Month BCD 6             */
     s_flash[0xE2] = 0x04;   /* Day   BCD 4             */
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static uint8_t  s_cmos[256] = {0};
 static uint8_t  s_tpm_resp[64];
 static bool     s_wp = true;
 static uint32_t s_spi_ops = 0;
 
 static int stub_init(uiox_bios_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  SPI flash  base=0x%lX  size=%u KB\n",
            (unsigned long)hw->spi_base,
            hw->geo.total_bytes / 1024u);
     flash_init_sim();
     /* Pre-load CMOS with realistic defaults */
     s_cmos[UIOX_CMOS_RTC_YEAR]    = 0x26u;  /* BCD 26 = 2026 */
     s_cmos[UIOX_CMOS_RTC_MONTH]   = 0x06u;
     s_cmos[UIOX_CMOS_RTC_DAY]     = 0x04u;
     s_cmos[UIOX_CMOS_RTC_HOURS]   = 0x10u;
     s_cmos[UIOX_CMOS_RTC_MINUTES] = 0x30u;
     s_cmos[UIOX_CMOS_RTC_SECONDS] = 0x00u;
     s_cmos[UIOX_CMOS_BOOT_DEV]    = 0x01u;  /* 0=HDD, 1=NVMe */
     /* Compute CMOS checksum */
     uint16_t cksum = 0;
     for (uint8_t i = 0x10u; i <= 0x2Du; i++) cksum += s_cmos[i];
     s_cmos[UIOX_CMOS_CHECKSUM_HI] = (uint8_t)(cksum >> 8u);
     s_cmos[UIOX_CMOS_CHECKSUM_LO] = (uint8_t)(cksum & 0xFFu);
     return 0;
 }
 
 static void stub_deinit(uiox_bios_hw_t *hw) { (void)hw; }
 
 static int stub_spi_read(uiox_bios_hw_t *hw,
                           uint32_t offset, void *buf, uint32_t len)
 {
     (void)hw;
     if (offset + len > FLASH_SIZE) return -ERANGE;
     memcpy(buf, &s_flash[offset], len);
     s_spi_ops++;
     return 0;
 }
 
 static int stub_spi_write(uiox_bios_hw_t *hw,
                            uint32_t offset, const void *buf, uint32_t len)
 {
     (void)hw;
     if (s_wp) return -EACCES;
     if (offset + len > FLASH_SIZE) return -ERANGE;
     /* NOR flash: can only clear bits (AND with existing) */
     for (uint32_t i = 0; i < len; i++)
         s_flash[offset + i] &= ((const uint8_t *)buf)[i];
     s_spi_ops++;
     printf("  [hal] SPI write  off=0x%06X  len=%u\n", offset, len);
     return 0;
 }
 
 static int stub_erase_sector(uiox_bios_hw_t *hw, uint32_t offset)
 {
     (void)hw;
     if (s_wp) return -EACCES;
     if (offset + 4096u > FLASH_SIZE) return -ERANGE;
     memset(&s_flash[offset], 0xFF, 4096u);
     printf("  [hal] SPI erase sector  off=0x%06X\n", offset);
     s_spi_ops++;
     return 0;
 }
 
 static int stub_erase_block(uiox_bios_hw_t *hw, uint32_t offset)
 {
     (void)hw;
     if (s_wp) return -EACCES;
     if (offset + 65536u > FLASH_SIZE) return -ERANGE;
     memset(&s_flash[offset], 0xFF, 65536u);
     printf("  [hal] SPI erase block  off=0x%06X\n", offset);
     return 0;
 }
 
 static int stub_erase_chip(uiox_bios_hw_t *hw)
 {
     (void)hw;
     if (s_wp) return -EACCES;
     memset(s_flash, 0xFF, FLASH_SIZE);
     printf("  [hal] SPI chip erase\n");
     return 0;
 }
 
 static int stub_read_status(uiox_bios_hw_t *hw,
                              uint8_t *sr1, uint8_t *sr2)
 {
     (void)hw;
     *sr1 = s_wp ? UIOX_SPI_SR_SRP : 0x00u;
     *sr2 = 0x00u;
     return 0;
 }
 
 static int stub_read_jedec(uiox_bios_hw_t *hw,
                             uint8_t *mfr, uint16_t *dev)
 {
     (void)hw;
     *mfr = 0xEFu;        /* Winbond */
     *dev = 0x4018u;      /* W25Q128 (16 MB) */
     printf("  [hal] JEDEC  mfr=0x%02X  dev=0x%04X  (Winbond W25Q128)\n",
            *mfr, *dev);
     return 0;
 }
 
 static int stub_wait_ready(uiox_bios_hw_t *hw, uint32_t timeout_ms)
 { (void)hw; (void)timeout_ms; return 0; }
 
 static int stub_set_wp(uiox_bios_hw_t *hw, bool protect)
 {
     (void)hw;
     s_wp = protect;
     printf("  [hal] WP# → %s\n", protect ? "PROTECTED" : "UNPROTECTED");
     return 0;
 }
 
 static bool stub_get_wp(uiox_bios_hw_t *hw)
 { (void)hw; return s_wp; }
 
 static uint8_t stub_cmos_read(uiox_bios_hw_t *hw, uint8_t idx)
 { (void)hw; return s_cmos[idx]; }
 
 static void stub_cmos_write(uiox_bios_hw_t *hw, uint8_t idx, uint8_t val)
 { (void)hw; s_cmos[idx] = val; }
 
 static int stub_smm_enter(uiox_bios_hw_t *hw)
 { (void)hw; printf("  [hal] SMM enter\n"); return 0; }
 
 static void stub_smm_exit(uiox_bios_hw_t *hw)
 { (void)hw; printf("  [hal] SMM exit\n"); }
 
 static int stub_tpm_send(uiox_bios_hw_t *hw,
                           const uint8_t *cmd, uint16_t cmd_len,
                           uint8_t *resp, uint16_t *resp_len)
 {
     (void)hw;
     printf("  [hal] TPM cmd  len=%u  tag=0x%02X%02X\n",
            cmd_len, cmd[0], cmd[1]);
     /* Stub response: TPM_RC_SUCCESS = 0x000 */
     memset(s_tpm_resp, 0, sizeof(s_tpm_resp));
     s_tpm_resp[0] = 0x80; s_tpm_resp[1] = 0x01;  /* TPM_ST_NO_SESSIONS */
     s_tpm_resp[2] = 0x00; s_tpm_resp[3] = 0x00;
     s_tpm_resp[4] = 0x00; s_tpm_resp[5] = 0x0A;  /* responseSize = 10  */
     *resp_len = 10u;
     memcpy(resp, s_tpm_resp, *resp_len);
     return 0;
 }
 
 static int stub_microcode(uiox_bios_hw_t *hw,
                            const void *ucode, uint32_t size)
 {
     (void)hw; (void)ucode;
     printf("  [hal] microcode update  size=%u bytes\n", size);
     return 0;
 }
 
 static bool stub_gpio_read(uiox_bios_hw_t *hw, uint32_t pin)
 { (void)hw; (void)pin; return false; }
 
 static void stub_gpio_write(uiox_bios_hw_t *hw, uint32_t pin, bool val)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", pin, (int)val); }
 
 static void stub_isr_spi(uiox_bios_hw_t *hw) { (void)hw; }
 
 static const uiox_bios_hw_ops_t stub_ops = {
     .init             = stub_init,
     .deinit           = stub_deinit,
     .spi_read         = stub_spi_read,
     .spi_write        = stub_spi_write,
     .spi_erase_sector = stub_erase_sector,
     .spi_erase_block  = stub_erase_block,
     .spi_erase_chip   = stub_erase_chip,
     .spi_read_status  = stub_read_status,
     .spi_read_jedec   = stub_read_jedec,
     .spi_wait_ready   = stub_wait_ready,
     .set_wp           = stub_set_wp,
     .get_wp           = stub_get_wp,
     .cmos_read        = stub_cmos_read,
     .cmos_write       = stub_cmos_write,
     .smm_enter        = stub_smm_enter,
     .smm_exit         = stub_smm_exit,
     .tpm_send         = stub_tpm_send,
     .microcode_update = stub_microcode,
     .gpio_read        = stub_gpio_read,
     .gpio_write       = stub_gpio_write,
     .isr_spi_done     = stub_isr_spi,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_bios_hw_t s_hw = {
     .spi_base   = 0xFED01000uL,
     .flash_mmio = 0xFF000000uL,
     .irq_spi    = 20,
     .caps       = UIOX_BIOS_CAP_SPI_FLASH  |
                   UIOX_BIOS_CAP_SMM        |
                   UIOX_BIOS_CAP_TPM        |
                   UIOX_BIOS_CAP_SECURE_BOOT|
                   UIOX_BIOS_CAP_WP_GPIO   |
                   UIOX_BIOS_CAP_NVRAM_EFI  |
                   UIOX_BIOS_CAP_NVRAM_CMOS |
                   UIOX_BIOS_CAP_MICROCODE  |
                   UIOX_BIOS_CAP_ACPI       |
                   UIOX_BIOS_CAP_RECOVERY,
     .type       = UIOX_BIOS_TYPE_UEFI,
     .geo        = { .total_bytes  = FLASH_SIZE,
                     .sector_bytes = 4096u,
                     .block_bytes  = 65536u,
                     .page_bytes   = 256u,
                     .jedec_mfr    = 0xEFu,
                     .jedec_dev    = 0x4018u },
     .version    = "3.2.1",
     .vendor     = "AMI Aptio UIOX",
     .build_date = 20260604u,
     .wp_gpio_pin    = 12u,
     .recovery_pin   = 13u,
     .wp_active      = true,
     .tpm_base       = 0xFED40000uL,
     .tpm_i2c_addr   = 0x00u,
     /* Flash regions (Intel descriptor layout) */
     .regions    = {
         { "descriptor", 0x000000u, 0x001000u, true,  false, true  },
         { "ME",         0x001000u, 0x200000u, false, false, true  },
         { "BIOS",       0xC00000u, 0x400000u, true,  true,  false },
         { "GbE",        0x600000u, 0x008000u, true,  true,  false },
         { "PDR",        0x608000u, 0x008000u, true,  false, true  },
     },
     .num_regions = 5,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_bios_event(uiox_bios_evt_t evt, void *ctx)
 {
     (void)ctx;
     printf("  [event] %s\n", uiox_bios_evt_name(evt));
 }
 
 /* =========================================================================
  * Simulated firmware update image (small stub)
  * ====================================================================== */
 
 static uint8_t s_fw_image[4096];
 
 static void build_fw_image(void)
 {
     memset(s_fw_image, 0xFF, sizeof(s_fw_image));
     /* Write update marker */
     static const uint8_t hdr[] = {
         0x55,0xAA,
         'U','I','O','X','B','I','O','S',
         0x03,0x02,0x02,  /* Version 3.2.2 (updated) */
     };
     memcpy(s_fw_image, hdr, sizeof(hdr));
     /* Checksum placeholder at end of page */
     uint8_t cksum = 0;
     for (int i = 0; i < 255; i++) cksum += s_fw_image[i];
     s_fw_image[255] = (uint8_t)(~cksum + 1u);
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX BIOS Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_bios_device_t dev;
     uiox_bios_open_params_t p = {
         .hw                  = &s_hw,
         .hw_ops              = &stub_ops,
         .nvram_flash_offset  = 0xC00000u,  /* Start of BIOS region */
         .evt_cb              = on_bios_event,
     };
 
     int rc = uiox_bios_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     /* 2. Detect flash chip                                                */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Flash JEDEC detection ---\n");
     uint8_t mfr = 0; uint16_t dev_id = 0;
     uiox_bios_hw_read_jedec(&dev.subsys.bif.hw[0], &mfr, &dev_id);
 
     /* ------------------------------------------------------------------ */
     /* 3. Start (execute POST)                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- POST ---\n");
     rc = uiox_bios_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_bios_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     /* 4. Print BIOS info                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- BIOS information ---\n");
     uiox_bios_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 5. EFI variable operations                                          */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- EFI variables ---\n");
     static const uiox_efi_guid_t global = UIOX_GUID_GLOBAL;
     static const uiox_efi_guid_t setup  = UIOX_GUID_SETUP;
 
     /* Read BootOrder */
     uint16_t boot_order[8];
     uint32_t size = sizeof(boot_order);
     uint32_t attrs = 0;
     rc = uiox_bios_var_get(&dev, &global, "BootOrder",
                             boot_order, &size, &attrs);
     printf("  GET BootOrder  rc=%d  size=%u  order=[",rc,size);
     for (uint32_t i = 0; i < size/2u; i++)
         printf("0x%04X%s", boot_order[i], i+1<size/2u?",":"");
     printf("]\n");
 
     /* Set custom setup variable */
     uint8_t setup_data[] = { 0x01u, 0x00u, 0x01u, 0x00u };
     rc = uiox_bios_var_set(&dev, &setup, "SetupData",
                             setup_data, sizeof(setup_data),
                             UIOX_EFI_VAR_NV | UIOX_EFI_VAR_BS);
     printf("  SET SetupData  rc=%d  size=%zu  attrs=0x%02X\n",
            rc, sizeof(setup_data),
            UIOX_EFI_VAR_NV | UIOX_EFI_VAR_BS);
 
     /* Read it back */
     uint8_t readback[16];
     uint32_t rb_size = sizeof(readback);
     rc = uiox_bios_var_get(&dev, &setup, "SetupData",
                             readback, &rb_size, &attrs);
     printf("  GET SetupData  rc=%d  data=[%02X %02X %02X %02X]\n",
            rc, readback[0], readback[1], readback[2], readback[3]);
 
     /* Delete a variable */
     rc = uiox_bios_var_del(&dev, &setup, "SetupData");
     printf("  DEL SetupData  rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     /* 6. CMOS operations                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- CMOS ---\n");
     uint8_t year  = uiox_bios_cmos_get(&dev, UIOX_CMOS_RTC_YEAR);
     uint8_t month = uiox_bios_cmos_get(&dev, UIOX_CMOS_RTC_MONTH);
     uint8_t day   = uiox_bios_cmos_get(&dev, UIOX_CMOS_RTC_DAY);
     uint8_t hour  = uiox_bios_cmos_get(&dev, UIOX_CMOS_RTC_HOURS);
     uint8_t min   = uiox_bios_cmos_get(&dev, UIOX_CMOS_RTC_MINUTES);
     printf("  RTC date/time  : 20%02X-%02X-%02X  %02X:%02X (BCD)\n",
            year, month, day, hour, min);
     printf("  Boot device    : 0x%02X\n",
            uiox_bios_cmos_get(&dev, UIOX_CMOS_BOOT_DEV));
     printf("  CMOS checksum  : %s\n",
            uiox_bios_nvram_cmos_valid(&dev.subsys.nvram) ? "VALID" : "INVALID");
 
     /* Update boot device via CMOS */
     uiox_bios_cmos_set(&dev, UIOX_CMOS_BOOT_DEV, 0x02u); /* USB */
     printf("  Updated boot device → 0x%02X (USB)\n",
            uiox_bios_cmos_get(&dev, UIOX_CMOS_BOOT_DEV));
 
     /* ------------------------------------------------------------------ */
     /* 7. Flash read operation                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Flash read ---\n");
     uint8_t bios_hdr[16];
     rc = uiox_bios_flash_read(&dev, 0xE00000u, bios_hdr, sizeof(bios_hdr));
     printf("  Read BIOS region header (rc=%d):\n  ", rc);
     for (int i = 0; i < 16; i++) printf("%02X ", bios_hdr[i]);
     printf("\n");
 
     /* ------------------------------------------------------------------ */
     /* 8. Flash write + verify                                             */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Flash write + verify (4 KB sector) ---\n");
     static uint8_t test_sector[4096];
     memset(test_sector, 0xA5u, sizeof(test_sector));
     test_sector[0] = 0xDE; test_sector[1] = 0xAD;
     test_sector[2] = 0xBE; test_sector[3] = 0xEF;
 
     rc = uiox_bios_flash_write(&dev, 0xC00000u,
                                 test_sector, sizeof(test_sector));
     printf("  Write 4 KB @ 0xC00000  rc=%d\n", rc);
 
     rc = uiox_bios_flash_verify(&dev, 0xC00000u,
                                  test_sector, sizeof(test_sector));
     printf("  Verify        rc=%d  (%s)\n", rc, rc==0?"PASS":"FAIL");
 
     /* ------------------------------------------------------------------ */
     /* 9. Firmware update simulation                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Firmware update (4 KB at BIOS region) ---\n");
     build_fw_image();
     rc = uiox_bios_flash_update(&dev, 0xC01000u,
                                  s_fw_image, sizeof(s_fw_image));
     printf("  Update rc=%d\n", rc);
     rc = uiox_bios_flash_verify(&dev, 0xC01000u,
                                  s_fw_image, sizeof(s_fw_image));
     printf("  Verify rc=%d  (%s)\n", rc, rc==0?"PASS":"FAIL");
 
     /* ------------------------------------------------------------------ */
     /* 10. Write-protect                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Write-protect ---\n");
     uiox_bios_set_wp(&dev, true);
     printf("  WP active: %s\n", uiox_bios_get_wp(&dev) ? "YES" : "NO");
     /* Attempt write while protected */
     rc = uiox_bios_flash_write(&dev, 0xC02000u, test_sector, 256u);
     printf("  Write while WP: rc=%d (%s)\n", rc,
            rc == -EACCES ? "EACCES — blocked correctly" : "unexpected");
 
     /* ------------------------------------------------------------------ */
     /* 11. TPM command                                                     */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- TPM command (TPM2_GetCapability) ---\n");
     static const uint8_t tpm_get_cap[] = {
         0x80,0x01,               /* TPM_ST_NO_SESSIONS               */
         0x00,0x00,0x00,0x16,     /* commandSize = 22                 */
         0x00,0x00,0x01,0x7A,     /* TPM_CC_GetCapability             */
         0x00,0x00,0x00,0x06,     /* TPM_CAP_TPM_PROPERTIES           */
         0x00,0x00,0x01,0x00,     /* property: TPM_PT_FAMILY_INDICATOR*/
         0x00,0x00,0x00,0x01,     /* propertyCount = 1                */
     };
     uint8_t tpm_resp[64];
     uint16_t resp_len = sizeof(tpm_resp);
     rc = uiox_bios_tpm_send(&dev, tpm_get_cap, sizeof(tpm_get_cap),
                              tpm_resp, &resp_len);
     printf("  TPM response  rc=%d  len=%u  RC=0x%02X%02X\n",
            rc, resp_len, tpm_resp[6], tpm_resp[7]);
 
     /* ------------------------------------------------------------------ */
     /* 12. ACPI info                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- ACPI tables ---\n");
     printf("  RSDP  : 0x%016llX  valid=%s\n",
            (unsigned long long)dev.subsys.svc.acpi.rsdp_phys,
            dev.subsys.svc.acpi.valid ? "yes" : "no");
     printf("  MADT  : 0x%016llX\n",
            (unsigned long long)dev.subsys.svc.acpi.madt_phys);
     printf("  FADT  : 0x%016llX\n",
            (unsigned long long)dev.subsys.svc.acpi.fadt_phys);
 
     /* ------------------------------------------------------------------ */
     /* 13. Periodic tick                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Periodic tick (5 × 10 ms) ---\n");
     for (uint32_t t = 10u; t <= 50u; t += 10u)
         uiox_bios_tick(&dev, t);
     printf("  Uptime: %llu ms\n",
            (unsigned long long)dev.subsys.uptime_ms);
 
     /* ------------------------------------------------------------------ */
     /* 14. Statistics                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_bios_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 15. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_bios_stop(&dev);
     printf("  State: %s\n", uiox_bios_state_name(dev.subsys.state));
     uiox_bios_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX BIOS Demo complete ===\n");
     return 0;
 }
 