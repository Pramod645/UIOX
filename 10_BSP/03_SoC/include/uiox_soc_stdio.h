/**
 * @file    uiox_soc_stdio.h
 * @brief   UIOX SoC — bare-metal printf / puts / snprintf replacement.
 *
 * Replaces #include <stdio.h> in all SoC backend files.
 * No libc dependency.  All output goes through uiox_soc_hw_uart_putc()
 * via the registered HW vtable.
 *
 * Provided functions:
 *   uiox_printf(fmt, ...)         — subset of printf: %s %d %u %x %llu %p
 *   uiox_snprintf(buf, n, fmt)    — bounded string format (no libc)
 *   uiox_puts(s)                  — write string + newline via UART
 *
 * @version 1.0.0
 * @date    2026-07-18
 */

 #ifndef UIOX_SOC_STDIO_H
 #define UIOX_SOC_STDIO_H
 
 #include "uiox_base_types.h"
 #include "uiox_soc_hw.h"   /* uiox_soc_hw_uart_putc() */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── Public API ──────────────────────────────────────────────────────── */
 
 /**
  * Formatted output via the registered HW UART.
  * Supports: %c %s %d %i %u %x %X %p %lld %llu %llx %% and width for %x.
  */
 void uiox_printf (const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 
 /**
  * Write @s followed by '\n' via the registered HW UART.
  */
 void uiox_puts   (const char *s);
 
 /**
  * Write a single character via the registered HW UART.
  */
 void uiox_putc   (char c);
 
 /**
  * Bounded string format — no libc, no malloc.
  * Returns number of characters that would have been written (like snprintf).
  */
 int  uiox_snprintf(char *buf, uiox_size_t n, const char *fmt, ...)
      __attribute__((format(printf, 3, 4)));
 
 /*
  * Drop-in macros so existing code that calls printf() / snprintf() in
  * the SoC backend files compiles without any source edits beyond
  * replacing the #include.
  */
 #define printf(...)          uiox_printf(__VA_ARGS__)
 #define snprintf(b, n, ...)  uiox_snprintf((b), (n), __VA_ARGS__)
 #define puts(s)              uiox_puts(s)
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_STDIO_H */
 