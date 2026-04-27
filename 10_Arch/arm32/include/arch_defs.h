#ifndef UIOX_ARCH_DEFS_ARM32_H
#define UIOX_ARCH_DEFS_ARM32_H

/*
 * arch/arm32/include/arch_defs.h
 *
 * ARMv7-A 32-bit architecture definitions.
 */

/* ── Identity ─────────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "arm32"
#define UIOX_ARCH_ARM32         1
#define UIOX_BITS               32
#define UIOX_ENDIAN_LITTLE      1

/* ── Word / pointer sizes ─────────────────────────────────── */
#define UIOX_PTR_SIZE           4
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         32

/* ── Physical memory map (QEMU virt / versatilepb) ──────── */
#define PHYS_DRAM_BASE          0x60000000UL
#define PHYS_DRAM_SIZE          0x04000000UL    /* 64 MB           */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x01000000UL

/* ── GIC-400 (same register layout as ARM64 sim) ─────────── */
#define GIC_DIST_BASE           0x08000000UL
#define GIC_CPU_BASE            0x08010000UL
#define GIC_DIST_CTLR           (GIC_DIST_BASE + 0x000)
#define GIC_DIST_ISENABLER0     (GIC_DIST_BASE + 0x100)
#define GIC_CPU_CTLR            (GIC_CPU_BASE  + 0x000)
#define GIC_CPU_IAR             (GIC_CPU_BASE  + 0x00C)
#define GIC_CPU_EOIR            (GIC_CPU_BASE  + 0x010)

/* ── PL011 UART ───────────────────────────────────────────── */
#define UART0_BASE              0x10009000UL
#define UART0_DR                (UART0_BASE + 0x000)
#define UART0_FR                (UART0_BASE + 0x018)
#define UART0_IBRD              (UART0_BASE + 0x024)
#define UART0_FBRD              (UART0_BASE + 0x028)
#define UART0_LCR_H             (UART0_BASE + 0x02C)
#define UART0_CR                (UART0_BASE + 0x030)
#define UART0_IMSC              (UART0_BASE + 0x038)
#define UART0_ICR               (UART0_BASE + 0x044)
#define UART0_IRQ               44

/* ── SP804 Timer ──────────────────────────────────────────── */
#define TIMER0_BASE             0x10011000UL
#define TIMER0_LOAD             (TIMER0_BASE + 0x000)
#define TIMER0_VALUE            (TIMER0_BASE + 0x004)
#define TIMER0_CTRL             (TIMER0_BASE + 0x008)
#define TIMER0_INTCLR           (TIMER0_BASE + 0x00C)
#define TIMER0_IRQ              36

/* ── IDE / CF disk controller ────────────────────────────── */
#define IDE_BASE                0x1000A000UL
#define IDE_IRQ                 46

/* ── ARM32 CPSR mode / flag bits ─────────────────────────── */
#define CPSR_MODE_USR           0x10
#define CPSR_MODE_FIQ           0x11
#define CPSR_MODE_IRQ           0x12
#define CPSR_MODE_SVC           0x13
#define CPSR_MODE_ABT           0x17
#define CPSR_MODE_UND           0x1B
#define CPSR_MODE_SYS           0x1F
#define CPSR_I_BIT              (1u << 7)   /* IRQ disable        */
#define CPSR_F_BIT              (1u << 6)   /* FIQ disable        */
#define CPSR_T_BIT              (1u << 5)   /* Thumb state        */

/* ── Compiler helpers ────────────────────────────────────── */
#define ARCH_INLINE             __attribute__((always_inline)) inline
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_SECTION(s)         __attribute__((section(s)))

/* ── Barrier macros ───────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("dmb\n\t"  ::: "memory")
#define arch_rmb()  __asm__ volatile("dmb\n\t"  ::: "memory")
#define arch_wmb()  __asm__ volatile("dmb\n\t"  ::: "memory")
#define arch_isb()  __asm__ volatile("isb\n\t"  ::: "memory")
#define arch_dsb()  __asm__ volatile("dsb\n\t"  ::: "memory")
#define arch_nop()  __asm__ volatile("nop\n\t")
#define arch_wfi()  __asm__ volatile("wfi\n\t")
#define arch_wfe()  __asm__ volatile("wfe\n\t")
#define arch_sev()  __asm__ volatile("sev\n\t")

/* ── IRQ control ─────────────────────────────────────────── */
#define arch_irq_disable() \
    __asm__ volatile( \
        "mrs r0, cpsr\n\t" \
        "orr r0, r0, #0x80\n\t" \
        "msr cpsr_c, r0\n\t" \
        ::: "r0", "memory")

#define arch_irq_enable() \
    __asm__ volatile( \
        "mrs r0, cpsr\n\t" \
        "bic r0, r0, #0x80\n\t" \
        "msr cpsr_c, r0\n\t" \
        ::: "r0", "memory")

#define arch_irq_save(flags) \
    __asm__ volatile("mrs %0, cpsr\n\t" : "=r"(flags) :: "memory")

#define arch_irq_restore(flags) \
    __asm__ volatile("msr cpsr_c, %0\n\t" :: "r"(flags) : "memory")

/* ── Cross-layer UIOX config ─────────────────────────────── */
#define UIOX_BLOCK_SIZE         512
#define UIOX_MAX_BLOCKS         1024
#define UIOX_MAX_INODES         128
#define UIOX_MAJOR_BLK_MAX      8
#define UIOX_MAJOR_CHR_MAX      8
#define UIOX_CBLOCK_POOL        256
#define UIOX_IRQ_MAX            64
#define UIOX_DMA_MAX_DESC       16
#define UIOX_MMIO_REGIONS       8

#endif /* UIOX_ARCH_DEFS_ARM32_H */
