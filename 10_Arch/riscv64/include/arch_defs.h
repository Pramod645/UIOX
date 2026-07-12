/*
 * 10_Arch/riscv64/include/arch_defs.h
 * UIOX RISC-V 64-bit architecture definitions.
 *
 * Mirrors the structure of 10_Arch/arm64/include/arch_defs.h for
 * RISC-V RV64IMAFDC targets (QEMU virt / SiFive U74).
 */
#ifndef UIOX_ARCH_DEFS_RISCV64_H
#define UIOX_ARCH_DEFS_RISCV64_H

/* ── Identity ─────────────────────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "riscv64"
#define UIOX_ARCH_RISCV64       1
#define UIOX_BITS               64
#define UIOX_ENDIAN_LITTLE      1

/* ── Word / pointer sizes ─────────────────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU virt machine) ──────────────────────────── */
#define PHYS_DRAM_BASE          0x80000000UL
#define PHYS_DRAM_SIZE          0x08000000UL   /* 128 MB                   */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x10000000UL

/* ── CLINT ────────────────────────────────────────────────────────────── */
#define CLINT_BASE              0x02000000UL
#define CLINT_SIZE              0x00010000UL
#define CLINT_MSIP(hart)        (CLINT_BASE + 0x0000UL + (hart)*4UL)
#define CLINT_MTIMECMP(hart)    (CLINT_BASE + 0x4000UL + (hart)*8UL)
#define CLINT_MTIME             (CLINT_BASE + 0xBFF8UL)

/* ── PLIC ─────────────────────────────────────────────────────────────── */
#define PLIC_BASE               0x0C000000UL
#define PLIC_SIZE               0x04000000UL
#define PLIC_PRIORITY(n)        (PLIC_BASE + 4UL*(n))
#define PLIC_PENDING(w)         (PLIC_BASE + 0x1000UL + (w)*4UL)
#define PLIC_ENABLE(ctx)        (PLIC_BASE + 0x2000UL + (ctx)*0x80UL)
#define PLIC_THRESHOLD(ctx)     (PLIC_BASE + 0x200000UL + (ctx)*0x1000UL)
#define PLIC_CLAIM(ctx)         (PLIC_BASE + 0x200004UL + (ctx)*0x1000UL)
#define PLIC_MAX_IRQ            127u
#define PLIC_UART_IRQ           10u
#define PLIC_VIRTIO_IRQ_BASE    1u

/* ── NS16550A UART ────────────────────────────────────────────────────── */
#define UART0_BASE              0x10000000UL
#define UART0_IRQ               10
#define UART0_RBR               (UART0_BASE + 0x00UL)  /* RX buffer        */
#define UART0_THR               (UART0_BASE + 0x00UL)  /* TX holding       */
#define UART0_IER               (UART0_BASE + 0x01UL)  /* Interrupt enable */
#define UART0_IIR               (UART0_BASE + 0x02UL)  /* Interrupt ID     */
#define UART0_FCR               (UART0_BASE + 0x02UL)  /* FIFO control     */
#define UART0_LCR               (UART0_BASE + 0x03UL)  /* Line control     */
#define UART0_MCR               (UART0_BASE + 0x04UL)  /* Modem control    */
#define UART0_LSR               (UART0_BASE + 0x05UL)  /* Line status      */
#define UART0_DLL               (UART0_BASE + 0x00UL)  /* Divisor latch lo */
#define UART0_DLM               (UART0_BASE + 0x01UL)  /* Divisor latch hi */

/* UART LSR bits */
#define UART_LSR_DR             (1u << 0)  /* Data ready                   */
#define UART_LSR_THRE           (1u << 5)  /* TX holding register empty    */

/* ── VirtIO ───────────────────────────────────────────────────────────── */
#define VIRTIO_BASE             0x10001000UL
#define VIRTIO_STRIDE           0x1000UL
#define VIRTIO_IRQ_BASE         1u
#define VIRTIO_SLOTS            8u

/* ── Compiler helpers ─────────────────────────────────────────────────── */
#define ARCH_INLINE             __attribute__((always_inline)) inline
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_SECTION(s)         __attribute__((section(s)))

/* ── Barrier macros ───────────────────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("fence rw,rw" ::: "memory")
#define arch_rmb()  __asm__ volatile("fence r,r"   ::: "memory")
#define arch_wmb()  __asm__ volatile("fence w,w"   ::: "memory")
#define arch_isb()  __asm__ volatile("fence.i"     ::: "memory")
#define arch_dsb()  __asm__ volatile("fence rw,rw" ::: "memory")
#define arch_nop()  __asm__ volatile("nop")
#define arch_wfi()  __asm__ volatile("wfi")

/* ── IRQ control (sstatus.SIE bit) ───────────────────────────────────── */
#define arch_irq_disable() \
    __asm__ volatile("csrc sstatus, %0" :: "r"(0x2u) : "memory")
#define arch_irq_enable() \
    __asm__ volatile("csrs sstatus, %0" :: "r"(0x2u) : "memory")
#define arch_irq_save(flags) \
    __asm__ volatile("csrr %0, sstatus" : "=r"(flags) :: "memory")
#define arch_irq_restore(flags) \
    __asm__ volatile("csrw sstatus, %0" :: "r"(flags) : "memory")

/* ── CSR shorthand ────────────────────────────────────────────────────── */
#define CSR_READ(csr)       ({ uint64_t _v; \
    __asm__ volatile("csrr %0, " csr : "=r"(_v)); _v; })
#define CSR_WRITE(csr, v)   \
    __asm__ volatile("csrw " csr ", %0" :: "r"((uint64_t)(v)) : "memory")
#define CSR_SET(csr, bits)  \
    __asm__ volatile("csrs " csr ", %0" :: "r"((uint64_t)(bits)) : "memory")
#define CSR_CLEAR(csr, bits)\
    __asm__ volatile("csrc " csr ", %0" :: "r"((uint64_t)(bits)) : "memory")

/* ── Cross-layer UIOX config ──────────────────────────────────────────── */
#define UIOX_BLOCK_SIZE         512
#define UIOX_MAX_BLOCKS         1024
#define UIOX_MAX_INODES         128
#define UIOX_IRQ_MAX            128
#define UIOX_MMIO_REGIONS       8

#endif /* UIOX_ARCH_DEFS_RISCV64_H */
