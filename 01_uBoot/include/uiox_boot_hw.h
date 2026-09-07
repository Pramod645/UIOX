/**
 * @file  uiox_boot_hw.h
 * @brief UIOX Bootloader — HW HAL: MMIO helpers, ops vtable, cache ops.
 *
 * Each architecture implements uiox_boot_hw_ops_t and calls
 * uiox_boot_hw_register() from its entry point before any C code runs.
 *
 * @version 1.1.0  (1.0.0 + RISC-V RV64GC additions — 2026-07-12)
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
 /* =========================================================================
 * MMIO accessors
 *
 * addr is declared as uintptr_t — the same integer width as a pointer on
 * every supported target (arm32=32-bit, arm64/x86-64/rv64=64-bit).
 * Casting (uintptr_t)addr → (volatile T *) is therefore always a
 * same-width integer-to-pointer cast, which is well-defined and
 * warning-free under -Wint-to-pointer-cast.
 *
 * DO NOT change the parameter type to uint32_t or uint64_t — that would
 * reintroduce the size-mismatch warning on 64-bit targets or 32-bit targets
 * respectively.
 * ====================================================================== */

 static inline void mmio_write32(__UINTPTR_TYPE__ addr, uint32_t val)
 { *((volatile uint32_t *)(__UINTPTR_TYPE__) addr) = val; }
 
 static inline uint32_t mmio_read32(__UINTPTR_TYPE__ addr)
 { return *((volatile uint32_t *)addr); }
 
 static inline void mmio_write8(__UINTPTR_TYPE__ addr, uint8_t val)
 { *((volatile uint8_t *)(__UINTPTR_TYPE__) addr) = val; }
 
 static inline uint8_t mmio_read8(__UINTPTR_TYPE__ addr)
 { return *((volatile uint8_t *)(__UINTPTR_TYPE__) addr); }

static inline void mmio_write64(__UINTPTR_TYPE__ addr, uint64_t val)
{
    *((volatile uint64_t *)(__UINTPTR_TYPE__)addr) = val;
}
static inline uint64_t mmio_read64(__UINTPTR_TYPE__ addr)
{
    return *((volatile uint64_t *)(__UINTPTR_TYPE__)addr);
}
 
 /* =========================================================================
  * Platform UART base addresses (same as arch_defs.h in UIOX kernel)
  * ====================================================================== */
 
 /* =========================================================================
  * Arch-specific MMIO base addresses (boot-time constants)
  * ====================================================================== */
 
 /* ARM64 */
 #define UIOX_PL011_BASE_ARM64   0x09000000UL /**< QEMU virt PL011        */
 #define UIOX_GIC_DIST_BASE      0x08000000UL
 #define UIOX_GIC_CPU_BASE       0x08010000UL

  /* =========================================================================
  * GIC-400 base (ARM64 QEMU virt)
  * ====================================================================== */
 
  #define UIOX_GICD_BASE_ARM64    0x08000000u
  #define UIOX_GICC_BASE_ARM64    0x08010000u
  #define GICD_CTLR               0x000u
  #define GICC_CTLR               0x000u
  #define GICC_PMR                0x004u
 
 /* ARM32 */
 #define UIOX_PL011_BASE_ARM32   0x101F1000UL /**< versatilepb PL011      */
 #define UIOX_SP804_BASE_ARM32   0x101E2000UL /**< versatilepb SP804      */

  /* PL011 register offsets (ARM64 + ARM32 shared) */
  #define PL011_DR                0x000u
  #define PL011_FR                0x018u
  #define PL011_IBRD              0x024u
  #define PL011_FBRD              0x028u
  #define PL011_LCR_H             0x02Cu
  #define PL011_CR                0x030u
  #ifndef PL011_FR_TXFF
  # define PL011_FR_TXFF          (1u << 5)
  #endif
  #ifndef PL011_LCR_WLEN8
  # define PL011_LCR_WLEN8        (3u << 5)
  #endif
  #ifndef PL011_LCR_FEN
  # define PL011_LCR_FEN          (1u << 4)
  #endif
  #ifndef PL011_CR_UARTEN
  # define PL011_CR_UARTEN        (1u << 0)
  #endif
  #ifndef PL011_CR_TXE
  # define PL011_CR_TXE           (1u << 8)
  #endif
  #ifndef PL011_CR_RXE
  # define PL011_CR_RXE           (1u << 9)
  #endif
 
 /* x86-64 */
 #define UIOX_COM1_PORT          0x3F8u       /**< COM1 I/O port base     */
 #define UIOX_PIT_PORT           0x40u        /**< PIT 8254 I/O port base */

 /* x86-64 COM1 register offsets */
 #define COM1_THR                0u
 #define COM1_LSR                5u
 #define COM1_LSR_THRE           (1u << 5)
 #define COM1_IER                1u
 #define COM1_FCR                2u
 #define COM1_LCR                3u
 #define COM1_MCR                4u
 #define COM1_DLL                0u
 #define COM1_DLM                1u
 
 /* RISC-V RV64GC */
 #define UIOX_RV_UART_BASE       0x10000000UL /**< NS16550A UART (QEMU)   */
 #define UIOX_RV_CLINT_BASE      0x02000000UL /**< CLINT                  */
 #define UIOX_RV_CLINT_MTIME     (UIOX_RV_CLINT_BASE + 0xBFF8UL)
 #define UIOX_RV_CLINT_MTIMECMP0 (UIOX_RV_CLINT_BASE + 0x4000UL)
 #define UIOX_RV_PLIC_BASE       0x0C000000UL /**< PLIC                   */
 #define UIOX_RV_TEST_BASE       0x00100000UL /**< QEMU test finisher     */
 

 /* RISC-V — NS16550A UART @ 0x10000000                     <<< NEW >>> */
#define NS16550_RBR             0x00u   /**< Receive Buffer  (read)    */
#define NS16550_THR             0x00u   /**< Transmit Holding (write)  */
#define NS16550_IER             0x01u   /**< Interrupt Enable          */
#define NS16550_FCR             0x02u   /**< FIFO Control (write)      */
#define NS16550_LCR             0x03u   /**< Line Control              */
#define NS16550_MCR             0x04u   /**< Modem Control             */
#define NS16550_LSR             0x05u   /**< Line Status               */
#define NS16550_DLL             0x00u   /**< Divisor Latch Low  DLAB=1 */
#define NS16550_DLM             0x01u   /**< Divisor Latch High DLAB=1 */
#define NS16550_LSR_THRE        (1u << 5) /**< TX Holding Reg Empty   */
#define NS16550_LSR_DR          (1u << 0) /**< Data Ready (RX)        */

/* =========================================================================
 * RISC-V QEMU virt MMIO addresses
 * Consistent with uiox_cpu_hw.h and uiox-riscv64.dts
 * ====================================================================== */

/* NS16550A UART — QEMU virt serial0
 * (SiFive UART16550 @ 0x10000000, matches DTB uart0 node)           */
#define UIOX_RV_UART_BASE       0x10000000UL

/* NS16550 register offsets (byte-stride) */
#define NS16550_RBR             0x00u   /* Receive Buffer (read)          */
#define NS16550_THR             0x00u   /* Transmit Holding (write)       */
#define NS16550_IER             0x01u   /* Interrupt Enable               */
#define NS16550_FCR             0x02u   /* FIFO Control (write)           */
#define NS16550_LCR             0x03u   /* Line Control                   */
#define NS16550_MCR             0x04u   /* Modem Control                  */
#define NS16550_LSR             0x05u   /* Line Status                    */
#define NS16550_DLL             0x00u   /* Divisor Latch Low  (DLAB=1)   */
#define NS16550_DLM             0x01u   /* Divisor Latch High (DLAB=1)   */

#define NS16550_LSR_THRE        (1u << 5) /* Transmit Holding Reg Empty  */
#define NS16550_LSR_DR          (1u << 0) /* Data Ready                  */

/* CLINT — Core-Local Interruptor
 * uiox_cpu_hw.h: clint_base field; mtime @ 0x0200BFF8                */
#define UIOX_RV_CLINT_BASE      0x02000000UL
#define UIOX_RV_CLINT_MTIME     (UIOX_RV_CLINT_BASE + 0xBFF8UL)
#define UIOX_RV_CLINT_MTIMECMP0 (UIOX_RV_CLINT_BASE + 0x4000UL)

/* PLIC — Platform-Level Interrupt Controller
 * uiox_cpu_hw.h: gic_base used for PLIC on RV64
 * 0x0C000000 matches uiox-riscv64.dts plic node                     */
#define UIOX_RV_PLIC_BASE       0x0C000000UL
#define UIOX_RV_PLIC_PRIO(n)    (UIOX_RV_PLIC_BASE + ((n) * 4UL))
#define UIOX_RV_PLIC_THRESH_S   (UIOX_RV_PLIC_BASE + 0x201000UL) /* S-mode ctx0 */

/* QEMU virt Test Finisher — used for reset/poweroff in simulation    */
#define UIOX_RV_TEST_BASE       0x00100000UL
#define UIOX_RV_TEST_PASS       0x5555u     /* write → QEMU exit(0)   */
#define UIOX_RV_TEST_FAIL       0x3333u     /* write → QEMU exit(1)   */
#define UIOX_RV_TEST_RESET      0x7777u     /* write → QEMU reset     */


 
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
 
 /* =========================================================================
 * Arch registration prototypes
 * Each arch entry .S calls exactly one of these before uiox_boot_main().
 * ====================================================================== */
//void uiox_boot_hw_arm64_register  (void);   /**< ARM64 Cortex-A76        */
//void uiox_boot_hw_arm32_register  (void);   /**< ARM32 Cortex-A9         */
//void uiox_boot_hw_x86_register    (void);   /**< x86-64                  */
//void uiox_boot_hw_riscv64_register(void);   /**< RISC-V RV64GC  <<< NEW */

 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_HW_H */
 