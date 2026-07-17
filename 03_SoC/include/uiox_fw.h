/**
 * @file  uiox_fw.h
 * @brief UIOX Firmware — master umbrella include (updated v1.1).
 *
 * Adds all new HAL modules:
 *   uiox_fw_i2c   — I2C master (sensors, RTC, PMIC)
 *   uiox_fw_spi   — SPI master (flash, displays, ALS)
 *   uiox_fw_wdt   — Watchdog timer (SP805 / x86 iTCO)
 *   uiox_fw_dma   — DMA controller abstraction
 *   uiox_fw_pcie  — PCIe ECAM early init + BAR assignment
 *
 * @version 1.1.0
 * @date    2026-07-07
 */

 #ifndef UIOX_FW_H
 #define UIOX_FW_H
 
 /* ── Existing modules (unchanged) ────────────────────────────── */
 #include "uiox_fw_types.h"
 #include "uiox_fw_hw.h"
 #include "uiox_fw_irq.h"
 #include "uiox_fw_uart.h"
 #include "uiox_fw_timer.h"
 #include "uiox_fw_gpio.h"
 #include "uiox_fw_clock.h"
 #include "uiox_fw_power.h"
 #include "uiox_fw_mem.h"
 #include "uiox_fw_storage.h"
 #include "uiox_fw_net.h"
 #include "uiox_fw_devsw.h"
 #include "uiox_fw_sensor.h"
 #include "uiox_fw_post.h"
 #include "uiox_fw_psci.h"
 #include "uiox_fw_secboot.h"
 #include "uiox_fw_tz.h"
 
 /* ── New HAL modules ─────────────────────────────────────────── */
 #include "uiox_fw_i2c.h"     /**< I2C master — DW APB / PL031        */
 #include "uiox_fw_spi.h"     /**< SPI master — PL022 SSP             */
 #include "uiox_fw_wdt.h"     /**< Watchdog — SP805 / iTCO            */
 #include "uiox_fw_dma.h"     /**< DMA controller — PL080 / SW fallbk */
 #include "uiox_fw_pcie.h"    /**< PCIe ECAM — NVMe/SATA/NIC early init*/
 
 #define UIOX_FW_VERSION_STR  "UIOX Firmware v1.1"
 #define UIOX_FW_URL          "github.com/Pramod645/UIOX"
 
 /* Firmware printf (uses registered UART) */
 void uiox_fw_printf(const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 void uiox_fw_puts  (const char *s);
 void uiox_fw_putc  (char c);
 
 /* Stage log macros */
 #define FW_LOG(subsys, fmt, ...) \
     uiox_fw_printf("[FW] %-8s: " fmt "\n", (subsys), ##__VA_ARGS__)
 #define FW_OK()     uiox_fw_puts("OK\n")
 #define FW_ERR(fmt, ...) \
     uiox_fw_printf("[FW] ERROR: " fmt "\n", ##__VA_ARGS__)
 #define FW_FATAL(fmt, ...) \
     do { uiox_fw_printf("[FW] FATAL: " fmt "\n", ##__VA_ARGS__); \
          for (;;) __asm__ volatile("" ::: "memory"); } while(0)
 
 #endif /* UIOX_FW_H */
 