/**
 * @file  uiox_boot_console.h
 * @brief UIOX Bootloader — early serial console, printf, and BOOT_xxx macros.
 *
 * All BOOT_LOG / BOOT_OK / BOOT_ERR / BOOT_FATAL macros are defined here.
 * Include this (via uiox_boot.h) before using any BOOT_xxx call.
 *
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
void uiox_boot_putc(char c);
void uiox_boot_puts(const char *s);
void uiox_boot_printf(const char *fmt, ...)
     __attribute__((format(printf, 1, 2)));

/* Each macro prints prefix / message / newline as three calls.
 * __VA_ARGS__ always carries at least the format string, so it is never
 * an empty variadic list — ISO C99 clean under -Wpedantic, no ##, no
 * dummy argument, and it works with or without extra arguments. */

 #define BOOT_LOG(stage, ...)                                          \
 do {                                                              \
     uiox_boot_printf("[BOOT] Stage %u: ", (unsigned)(stage));     \
     uiox_boot_printf(__VA_ARGS__);                                \
     uiox_boot_puts("\n");                                         \
 } while (0)

#define BOOT_OK()   uiox_boot_puts("OK\n")

#define BOOT_ERR(...)                                                 \
 do {                                                              \
     uiox_boot_puts("[BOOT] ERROR: ");                             \
     uiox_boot_printf(__VA_ARGS__);                                \
     uiox_boot_puts("\n");                                         \
 } while (0)

#define BOOT_WARN(...)                                                \
 do {                                                              \
     uiox_boot_puts("[BOOT] WARN:  ");                             \
     uiox_boot_printf(__VA_ARGS__);                                \
     uiox_boot_puts("\n");                                         \
 } while (0)

#define BOOT_INFO(...)                                                \
 do {                                                              \
     uiox_boot_puts("[BOOT] INFO:  ");                             \
     uiox_boot_printf(__VA_ARGS__);                                \
     uiox_boot_puts("\n");                                         \
 } while (0)

#define BOOT_FATAL(...)                                               \
 do {                                                              \
     uiox_boot_puts("[BOOT] FATAL: ");                             \
     uiox_boot_printf(__VA_ARGS__);                                \
     uiox_boot_puts("\n[BOOT] System halted.\n");                  \
     for (;;)                                                      \
         __asm__ volatile("" ::: "memory");                       \
 } while (0)

#define BOOT_ASSERT(cond, ...)                                        \
 do {                                                              \
     if (!(cond)) {                                                \
         uiox_boot_puts("[BOOT] FATAL: assert(" #cond "): ");      \
         uiox_boot_printf(__VA_ARGS__);                            \
         uiox_boot_puts("\n[BOOT] System halted.\n");              \
         for (;;)                                                  \
             __asm__ volatile("" ::: "memory");                   \
     }                                                             \
 } while (0)

#define BOOT_CHECK(expr, label)                                       \
 do {                                                              \
     uiox_boot_err_t _rc = (expr);                                 \
     if (_rc != UIOX_BOOT_OK)                                     \
         BOOT_FATAL(label " failed (err=%d)", (int)_rc);           \
 } while (0)

 #ifndef UIOX_BOOT_VERSION_STR
  #define UIOX_BOOT_VERSION_STR  "UIOX Bootloader v1.0"
#endif
#ifndef UIOX_BOOT_URL
  #define UIOX_BOOT_URL          "github.com/Pramod645/UIOX"
#endif

#define BOOT_BANNER(arch_str)                                          \
    uiox_boot_printf("\n" UIOX_BOOT_VERSION_STR                        \
                     " (%s) [" UIOX_BOOT_URL "]\n", (arch_str))

#ifdef __cplusplus
}
#endif
#endif /* UIOX_BOOT_CONSOLE_H */
