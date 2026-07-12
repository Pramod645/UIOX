/**
 * @file  uiox_boot_hw_riscv64.c
 * @brief UIOX Bootloader — RISC-V RV64GC hardware ops.
 *
 * Target  : QEMU -machine virt (riscv64)
 * UART    : NS16550A @ 0x10000000  (SiFive UART16550, 8-entry FIFO)
 * Timer   : CLINT mtime/mtimecmp  @ 0x02000000 (uiox_cpu_hw.h clint_base)
 * Reset   : SiFive Test Finisher  @ 0x100000   (QEMU virt test device)
 *
 * Mirrors the structure of uiox_boot_hw_arm64.c and uiox_boot_hw_x86.c:
 *   - static hardware init functions
 *   - uiox_boot_hw_ops_t vtable
 *   - uiox_boot_hw_riscv64_register() called from entry .S
 *
 * Cross-references:
 *   uiox_cpu_hw.h   : UIOX_CPU_ARCH_RV64, clint_base, uiox_cpu_csr_read()
 *   uiox_boot_hw.h  : uiox_boot_hw_ops_t, mmio_read32/write32
 *   DTB riscv64.dts : UART @ 0x10000000, CLINT @ 0x02000000
 *
 * @version 1.0.0
 * @date    2026-07-12
 */
#include "uiox_boot.h"

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
 * UART helpers — 8-bit register access (NS16550 is byte-stride)
 * ====================================================================== */
static inline void uart_write8(uint32_t off, uint8_t val)
{
    *((volatile uint8_t *)(UIOX_RV_UART_BASE + off)) = val;
}
static inline uint8_t uart_read8(uint32_t off)
{
    return *((volatile uint8_t *)(UIOX_RV_UART_BASE + off));
}

/* =========================================================================
 * NS16550A UART init — 115200 8N1
 * QEMU virt clock for the UART divisor: 3.6864 MHz → divisor = 2
 * (3686400 / 16 / 115200 = 2)
 * Mirrors com1_init() in uiox_boot_hw_x86.c exactly.
 * ====================================================================== */
static void ns16550_init(void)
{
    uart_write8(NS16550_IER, 0x00u);  /* disable all interrupts           */
    uart_write8(NS16550_LCR, 0x80u);  /* DLAB = 1 (access divisor regs)   */
    uart_write8(NS16550_DLL, 0x02u);  /* divisor low  = 2 → 115200 baud  */
    uart_write8(NS16550_DLM, 0x00u);  /* divisor high = 0                 */
    uart_write8(NS16550_LCR, 0x03u);  /* 8N1, DLAB = 0                    */
    uart_write8(NS16550_FCR, 0xC7u);  /* enable+clear FIFOs, 14-byte trig */
    uart_write8(NS16550_MCR, 0x0Bu);  /* RTS, DTR, OUT2                   */
}

/* Blocking single-character transmit — mirrors com1_putc() / pl011_putc() */
static void ns16550_putc(char c)
{
    while (!(uart_read8(NS16550_LSR) & NS16550_LSR_THRE))
        ;
    uart_write8(NS16550_THR, (uint8_t)c);
}

/* =========================================================================
 * CLINT timer — mtime read
 * uiox_cpu_hw.h: uiox_cpu_hw_timestamp() uses the same counter.
 * mtime is 64-bit at 0x0200BFF8 (little-endian, two 32-bit reads on RV32,
 * single 64-bit load on RV64).
 * ====================================================================== */
static uint64_t riscv64_get_ticks(void)
{
    return *((volatile uint64_t *)UIOX_RV_CLINT_MTIME);
}

/* Busy-wait in microseconds.
 * QEMU virt CLINT runs at 10 MHz (10 ticks/µs).
 * Mirrors arm32_udelay() and x86_udelay().                           */
static void riscv64_udelay(uint32_t us)
{
    uint64_t start = riscv64_get_ticks();
    uint64_t wait  = (uint64_t)us * 10u;  /* 10 MHz CLINT clock           */
    while ((riscv64_get_ticks() - start) < wait)
        ;
}

/* =========================================================================
 * Cache / fence operations
 * RISC-V has no dedicated cache-management MMIO at boot.
 * fence.i  = I-cache invalidate (instruction-fetch fence)
 * fence    = D-cache writeback fence (ordering, not explicit flush)
 * Mirrors arm64_dcache_flush / arm64_icache_inv pattern.
 * ====================================================================== */
static void riscv64_dcache_flush(uintptr_t start, size_t len)
{
    (void)start; (void)len;
    /* RISC-V: 'fence' ensures prior stores are visible to subsequent loads.
     * No explicit cache-line clean instruction in the base ISA.
     * SiFive extensions (CBO) are available but not required here.      */
    __asm__ volatile("fence rw, rw" ::: "memory");
}

static void riscv64_icache_inv(void)
{
    /* fence.i: synchronise instruction stream with data writes.
     * Required after writing code into memory (e.g. ksign image load). */
    __asm__ volatile("fence.i" ::: "memory");
}

static void riscv64_barrier(void)
{
    __asm__ volatile("fence rw, rw" ::: "memory");
}

/* =========================================================================
 * PLIC early init — set all source priorities to 0 (disabled),
 * set S-mode threshold to 0 (accept all enabled interrupts).
 * Called once from uiox_boot_hw_riscv64_register() before handoff.
 * ====================================================================== */
static void plic_init(void)
{
    /* Disable all 64 interrupt sources by setting priority = 0 */
    for (uint32_t i = 0u; i < 64u; i++)
        mmio_write32(UIOX_RV_PLIC_PRIO(i), 0u);

    /* S-mode context 0 threshold = 0 (allow all when re-enabled) */
    mmio_write32((uintptr_t)UIOX_RV_PLIC_THRESH_S, 0u);
}

/* =========================================================================
 * Platform reset
 * QEMU virt: write RESET token to the test device @ 0x100000.
 * Mirrors arm64_reset() (PSCI) and arm32_reset() (watchdog).
 * ====================================================================== */
static void __attribute__((noreturn)) riscv64_reset(void)
{
    /* QEMU virt test finisher — write 0x7777 → system reset */
    mmio_write32((uintptr_t)UIOX_RV_TEST_BASE, UIOX_RV_TEST_RESET);
    /* If the write didn't take (real hardware), spin using wfi */
    for (;;)
        __asm__ volatile("wfi");
}

/* =========================================================================
 * Ops vtable — matches uiox_boot_hw_ops_t in uiox_boot_hw.h exactly.
 * Field order: init, uart_putc, dcache_flush, icache_inv,
 *              get_ticks, udelay, reset, barrier
 * ====================================================================== */
static const uiox_boot_hw_ops_t riscv64_ops = {
    .init         = ns16550_init,
    .uart_putc    = ns16550_putc,
    .dcache_flush = riscv64_dcache_flush,
    .icache_inv   = riscv64_icache_inv,
    .get_ticks    = riscv64_get_ticks,
    .udelay       = riscv64_udelay,
    .reset        = riscv64_reset,
    .barrier      = riscv64_barrier,
};

/* =========================================================================
 * Registration — called from uiox_boot_entry_riscv64.S (step 5)
 * before uiox_boot_main().  Mirrors:
 *   uiox_boot_hw_arm64_register()  in uiox_boot_hw_arm64.c
 *   uiox_boot_hw_arm32_register()  in uiox_boot_hw_arm32.c
 *   uiox_boot_hw_x86_register()    in uiox_boot_hw_x86.c
 * ====================================================================== */
void uiox_boot_hw_riscv64_register(void)
{
    plic_init();
    uiox_boot_hw_register(&riscv64_ops);
    /* ns16550_init() is called inside uiox_boot_hw_register()
     * via ops->init(), so the UART is live after this call returns.  */
}
