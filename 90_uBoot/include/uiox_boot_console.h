/**
 * @file  uiox_boot_console.h
 * @brief UIOX Bootloader — early serial console and printf.
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_CONSOLE_H
 #define UIOX_BOOT_CONSOLE_H
 
 #include "uiox_boot_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void uiox_boot_console_init(void);
 void uiox_boot_putc  (char c);
 void uiox_boot_puts  (const char *s);
 void uiox_boot_printf(const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 
 /* Convenience stage-log prefix */
 #define BOOT_LOG(stage, fmt, ...)  \
     uiox_boot_printf("[BOOT] Stage %u: " fmt "\n", (stage), ##__VA_ARGS__)
 #define BOOT_OK()  uiox_boot_puts("OK\n")
 #define BOOT_ERR(fmt, ...) \
     uiox_boot_printf("[BOOT] ERROR: " fmt "\n", ##__VA_ARGS__)
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_CONSOLE_H */
 