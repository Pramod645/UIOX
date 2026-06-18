/**
 * @file  uiox_boot_hw.h
 * @brief UIOX Bootloader — HW HAL: MMIO helpers, ops vtable, cache ops.
 *
 * Each architecture implements uiox_boot_hw_ops_t and calls
 * uiox_boot_hw_register() from its entry point before any C code runs.
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_HW_H
 #define UIOX_BOOT_HW_H
 
 #include "uiox_boot_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * MMIO accessors (volatile, no caching)
  * ====================================================================== */
 
 static inline void mmio_write32(uintptr_t addr, uint32_t val)
 { *((volatile uint32_t *)addr) = val; }
 
 static inline uint32_t mmio_read32(uintptr_t addr)
 { return *((volatile uint32_t *)addr); }
 
 static inline void mmio_write8(uintptr_t addr, uint8_t val)
 { *((volatile uint8_t *)addr) = val; }
 
 static inline uint8_t mmio_read8(uintptr_t addr)
 { return *((volatile uint8_t *)addr); }
 
 /* =========================================================================
  * Platform UART base addresses (same as arch_defs.h in UIOX kernel)
  * ====================================================================== */
 
 /* ARM64 / ARM32 — PL011 on QEMU virt / versatilepb */
 #define UIOX_PL011_BASE_ARM64   0x09000000u  /**< QEMU virt               */
 #define UIOX_PL011_BASE_ARM32   0x101F1000u  /**< QEMU versatilepb        */
 /* PL011 register offsets */
 #define PL011_DR                0x000u
 #define PL011_FR                0x018u
 #define PL011_FR_TXFF           (1u << 5)
 #define PL011_IBRD              0x024u
 #define PL011_FBRD              0x028u
 #define PL011_LCR_H             0x02Cu
 #define PL011_CR                0x030u
 #define PL011_CR_UARTEN         (1u << 0)
 #define PL011_CR_TXE            (1u << 8)
 #define PL011_CR_RXE            (1u << 9)
 #define PL011_LCR_WLEN8         (0x3u << 5)
 #define PL011_LCR_FEN           (1u << 4)
 
 /* x86_64 — 16550 COM1 */
 #define UIOX_COM1_PORT          0x3F8u       /**< COM1 I/O port base      */
 #define COM1_THR                0u
 #define COM1_LSR                5u
 #define COM1_LSR_THRE           (1u << 5)
 #define COM1_LCR                3u
 #define COM1_DLL                0u
 #define COM1_DLM                1u
 #define COM1_IER                1u
 #define COM1_FCR                2u
 #define COM1_MCR                4u
 
 /* =========================================================================
  * GIC-400 base (ARM64 QEMU virt)
  * ====================================================================== */
 
 #define UIOX_GICD_BASE_ARM64    0x08000000u
 #define UIOX_GICC_BASE_ARM64    0x08010000u
 #define GICD_CTLR               0x000u
 #define GICC_CTLR               0x000u
 #define GICC_PMR                0x004u
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     /** One-time early hardware init (clocks, UART, caches disabled). */
     void (*init)        (void);
 
     /** Put a single byte to the debug UART (blocking). */
     void (*uart_putc)   (char c);
 
     /** Flush D-cache range [start, start+len). */
     void (*dcache_flush)(uintptr_t start, size_t len);
 
     /** Invalidate I-cache. */
     void (*icache_inv)  (void);
 
     /** Read core timer / TSC (monotonic, arbitrary epoch). */
     uint64_t (*get_ticks)(void);
 
     /** Simple busy-wait in microseconds. */
     void (*udelay)      (uint32_t us);
 
     /** Reboot / reset the platform. */
     void (*reset)       (void) __attribute__((noreturn));
 
     /** Platform-specific memory-barrier (ISB/DSB on ARM, MFENCE on x86). */
     void (*barrier)     (void);
 } uiox_boot_hw_ops_t;
 
 /* =========================================================================
  * HAL API
  * ====================================================================== */
 
 /** Called from arch entry point — registers the platform ops table. */
 void uiox_boot_hw_register(const uiox_boot_hw_ops_t *ops);
 
 /** Retrieve the registered ops (assert-safe NULL guard). */
 const uiox_boot_hw_ops_t *uiox_boot_hw_ops(void);
 
 /* Convenience wrappers */
 void     uiox_boot_hw_init        (void);
 void     uiox_boot_hw_uart_putc   (char c);
 void     uiox_boot_hw_dcache_flush(uintptr_t start, size_t len);
 void     uiox_boot_hw_icache_inv  (void);
 uint64_t uiox_boot_hw_get_ticks   (void);
 void     uiox_boot_hw_udelay      (uint32_t us);
 void     uiox_boot_hw_reset       (void) __attribute__((noreturn));
 void     uiox_boot_hw_barrier     (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_HW_H */
 