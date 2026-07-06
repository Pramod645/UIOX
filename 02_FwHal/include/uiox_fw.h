/**
 * @file  uiox_fw.h
 * @brief UIOX Firmware — master include for all firmware APIs.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_H
 #define UIOX_FW_H
 
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

/* Add inside uiox_fw.h after the existing #include lines */
#include "uiox_fw_post.h"
#include "uiox_fw_secboot.h"
#include "uiox_fw_tz.h"
#include "uiox_fw_psci.h"


 
 #define UIOX_FW_VERSION_STR  "UIOX Firmware v1.0"
 #define UIOX_FW_URL          "github.com/Pramod645/UIOX"
 
 /* Firmware printf (uses registered UART) */
 void uiox_fw_printf(const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 void uiox_fw_puts  (const char *s);
 void uiox_fw_putc  (char c);
 
 /* Firmware log macros */
 #define FW_LOG(subsys, fmt, ...) \
     uiox_fw_printf("[FW] %-8s: " fmt "\n", (subsys), ##__VA_ARGS__)
 #define FW_OK()     uiox_fw_puts("OK\n")
 #define FW_ERR(fmt, ...) \
     uiox_fw_printf("[FW] ERROR: " fmt "\n", ##__VA_ARGS__)
 #define FW_FATAL(fmt, ...) \
     do { uiox_fw_printf("[FW] FATAL: " fmt "\n", ##__VA_ARGS__); \
          for (;;) __asm__ volatile("" ::: "memory"); } while(0)
 
 #endif /* UIOX_FW_H */
 