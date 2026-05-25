#ifndef UIOX_ARCH_DEFS_ARM64_H
#define UIOX_ARCH_DEFS_ARM64_H
/*
 * arch/arm64/include/arch_defs.h
 * ARMv8-A 64-bit architecture definitions.
 */

/* ── Identity ────────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "arm64"
#define UIOX_ARCH_ARM64         1
#define UIOX_BITS               64
#define UIOX_ENDIAN_LITTLE      1

/* ── Word / pointer sizes ────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU virt machine) ─────────────── */
#define PHYS_DRAM_BASE          0x40000000UL
#define PHYS_DRAM_SIZE          0x04000000UL    /* 64 MB           */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x01000000UL

/* ── GIC-400 ─────────────────────────────────────────────── */
#define GIC_DIST_BASE           0x08000000UL
#define GIC_CPU_BASE            0x08010000UL
#define GIC_DIST_CTLR           (GIC_DIST_BASE + 0x000)
#define GIC_DIST_ISENABLER0     (GIC_DIST_BASE + 0x100)
#define GIC_CPU_CTLR            (GIC_CPU_BASE  + 0x000)
#define GIC_CPU_IAR             (GIC_CPU_BASE  + 0x00C)
#define GIC_CPU_EOIR            (GIC_CPU_BASE  + 0x010)

/* ── PL011 UART ──────────────────────────────────────────── */
#define UART0_BASE              0x09000000UL
#define UART0_DR                (UART0_BASE + 0x000)
#define UART0_FR                (UART0_BASE + 0x018)
#define UART0_IBRD              (UART0_BASE + 0x024)
#define UART0_FBRD              (UART0_BASE + 0x028)
#define UART0_LCR_H             (UART0_BASE + 0x02C)
#define UART0_CR                (UART0_BASE + 0x030)
#define UART0_IMSC              (UART0_BASE + 0x038)
#define UART0_ICR               (UART0_BASE + 0x044)
#define UART0_IRQ               33

/* ── ARM Generic Timer ───────────────────────────────────── */
#define TIMER0_IRQ              27    /* PPI — virtual timer           */

/* ── VirtIO block device ─────────────────────────────────── */
#define VIRTIO_BASE             0x0A000000UL
#define VIRTIO_IRQ              48

/* ── AArch64 PSTATE mode bits ────────────────────────────── */
#define PSTATE_EL0h             0x00u
#define PSTATE_EL1t             0x04u
#define PSTATE_EL1h             0x05u
#define PSTATE_EL2h             0x09u
#define PSTATE_EL3h             0x0Du
#define PSTATE_I_BIT            (1u << 7)   /* IRQ mask               */
#define PSTATE_F_BIT            (1u << 6)   /* FIQ mask               */
#define PSTATE_A_BIT            (1u << 8)   /* SError mask            */
#define PSTATE_D_BIT            (1u << 9)   /* Debug mask             */

/* ── Compiler helpers ────────────────────────────────────── */
#define ARCH_INLINE             __attribute__((always_inline)) inline
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_SECTION(s)         __attribute__((section(s)))

/* ── Barrier macros ──────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("dmb sy\n\t"  ::: "memory")
#define arch_rmb()  __asm__ volatile("dmb ld\n\t"  ::: "memory")
#define arch_wmb()  __asm__ volatile("dmb st\n\t"  ::: "memory")
#define arch_isb()  __asm__ volatile("isb\n\t"     ::: "memory")
#define arch_dsb()  __asm__ volatile("dsb sy\n\t"  ::: "memory")
#define arch_nop()  __asm__ volatile("nop\n\t")
#define arch_wfi()  __asm__ volatile("wfi\n\t")
#define arch_wfe()  __asm__ volatile("wfe\n\t")
#define arch_sev()  __asm__ volatile("sev\n\t")

/* ── IRQ control ─────────────────────────────────────────── */
#define arch_irq_disable() \
    __asm__ volatile("msr daifset, #0xF\n\t" ::: "memory")

#define arch_irq_enable() \
    __asm__ volatile("msr daifclr, #0xF\n\t" ::: "memory")

#define arch_irq_save(flags) \
    __asm__ volatile("mrs %0, daif\n\t" : "=r"(flags) :: "memory")

#define arch_irq_restore(flags) \
    __asm__ volatile("msr daif, %0\n\t" :: "r"(flags) : "memory")

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

#endif /* UIOX_ARCH_DEFS_ARM64_H */
