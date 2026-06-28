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
 
 /* =========================================================================
  * Core console API — implemented in uiox_boot_console.c
  * ====================================================================== */
 
 /** Must be called once after uiox_boot_hw_register(). */
 void uiox_boot_console_init(void);
 
 /** Write one character (translates '\n' → '\r\n'). */
 void uiox_boot_putc(char c);
 
 /** Write a NUL-terminated string. */
 void uiox_boot_puts(const char *s);
 
 /**
  * Minimal printf.
  * Supported format specifiers:
  *   %c  %s  %d  %u  %x  %X  %llu  %lld  %llx  %p  %%
  * Width field supported for %x/%X (e.g. %08x, %016llx).
  * No float support (boot environment).
  */
 void uiox_boot_printf(const char *fmt, ...)
      __attribute__((format(printf, 1, 2)));
 
 /* =========================================================================
  * BOOT_LOG — stage progress message
  *
  *   BOOT_LOG(2, "Memory");
  *   → "[BOOT] Stage 2: Memory\n"
  *
  *   BOOT_LOG(4, "Loaded %lu bytes", n);
  *   → "[BOOT] Stage 4: Loaded 12345 bytes\n"
  * ====================================================================== */
 
 #define BOOT_LOG(stage, fmt, ...)                                         \
     uiox_boot_printf("[BOOT] Stage %u: " fmt "\n",                        \
                      (unsigned)(stage), ##__VA_ARGS__)
 
 /* =========================================================================
  * BOOT_OK — print "OK\n" on success (companion to BOOT_LOG)
  * ====================================================================== */
 
 #define BOOT_OK()                                                         \
     uiox_boot_puts("OK\n")
 
 /* =========================================================================
  * BOOT_ERR — non-fatal error message (execution continues)
  *
  *   BOOT_ERR("FAT32 mount failed (%d)", rc);
  *   → "[BOOT] ERROR: FAT32 mount failed (-4)\n"
  * ====================================================================== */
 
 #define BOOT_ERR(fmt, ...)                                                \
     uiox_boot_printf("[BOOT] ERROR: " fmt "\n", ##__VA_ARGS__)
 
 /* =========================================================================
  * BOOT_WARN — warning message (execution continues)
  * ====================================================================== */
 
 #define BOOT_WARN(fmt, ...)                                               \
     uiox_boot_printf("[BOOT] WARN:  " fmt "\n", ##__VA_ARGS__)
 
 /* =========================================================================
  * BOOT_INFO — informational message (no prefix formatting)
  * ====================================================================== */
 
 #define BOOT_INFO(fmt, ...)                                               \
     uiox_boot_printf("[BOOT] INFO:  " fmt "\n", ##__VA_ARGS__)
 
 /* =========================================================================
  * BOOT_FATAL — print error then halt forever (never returns).
  *
  *   BOOT_FATAL("out of memory");
  *   → "[BOOT] FATAL: out of memory\n"
  *   → spins forever
  *
  * The do{} while(1) ensures the compiler knows this path never returns
  * even without noreturn, avoiding "control reaches end of non-void
  * function" warnings in callers.
  * ====================================================================== */
 
 #define BOOT_FATAL(fmt, ...)                                              \
     do {                                                                  \
         uiox_boot_printf("[BOOT] FATAL: " fmt "\n", ##__VA_ARGS__);      \
         uiox_boot_puts("[BOOT] System halted.\n");                        \
         for (;;)                                                          \
             __asm__ volatile("" ::: "memory");                           \
     } while (0)
 
 /* =========================================================================
  * BOOT_ASSERT — assert a condition, BOOT_FATAL if false
  *
  *   BOOT_ASSERT(ptr != NULL, "kernel buffer is NULL");
  * ====================================================================== */
 
 #define BOOT_ASSERT(cond, fmt, ...)                                       \
     do {                                                                  \
         if (!(cond))                                                      \
             BOOT_FATAL("assert(" #cond "): " fmt, ##__VA_ARGS__);        \
     } while (0)
 
 /* =========================================================================
  * BOOT_CHECK — check a uiox_boot_err_t, BOOT_FATAL on error
  *
  *   BOOT_CHECK(uiox_boot_mem_probe(dtb_pa, &map), "memory probe");
  * ====================================================================== */
 
 #define BOOT_CHECK(expr, label)                                           \
     do {                                                                  \
         uiox_boot_err_t _rc = (expr);                                     \
         if (_rc != UIOX_BOOT_OK)                                         \
             BOOT_FATAL(label " failed (err=%d)", (int)_rc);              \
     } while (0)
 
 /* =========================================================================
  * BOOT_BANNER — print the version banner line
  * ====================================================================== */
 
 #ifndef UIOX_BOOT_VERSION_STR
   #define UIOX_BOOT_VERSION_STR  "UIOX Bootloader v1.0"
 #endif
 #ifndef UIOX_BOOT_URL
   #define UIOX_BOOT_URL          "github.com/Pramod645/UIOX"
 #endif
 
 #define BOOT_BANNER(arch_str)                                             \
     uiox_boot_printf("\n" UIOX_BOOT_VERSION_STR                          \
                      " (%s) [" UIOX_BOOT_URL "]\n", (arch_str))
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_CONSOLE_H */
 