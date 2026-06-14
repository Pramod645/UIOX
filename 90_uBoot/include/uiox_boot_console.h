#ifndef UIOX_BOOT_CONSOLE_H
#define UIOX_BOOT_CONSOLE_H
/*
 * uiox_boot_console.h - Bootloader console output.
 */
#include "uiox_boot_types.h"

void uboot_puts      (const char *s);
void uboot_putc      (char c);
void uboot_puthex8   (uboot_u8_t  v);
void uboot_puthex16  (uboot_u16_t v);
void uboot_puthex32  (uboot_u32_t v);
void uboot_puthex64  (uboot_u64_t v);
void uboot_putdec    (uboot_u32_t v);
void uboot_printf    (const char *fmt, ...);
void uboot_banner    (void);

#endif /* UIOX_BOOT_CONSOLE_H */
