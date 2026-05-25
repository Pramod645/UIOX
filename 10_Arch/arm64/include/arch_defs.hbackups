#ifndef UIOX_ARCH_DEFS_ARM64_H
#define UIOX_ARCH_DEFS_ARM64_H

/*
 * arch/arm64/include/arch_defs.h
 *
 * AArch64 (ARMv8-A 64-bit) architecture definitions.
 * Included by every translation unit when ARCH=arm64.
 *
 * Ties together uiox_fs + uiox_dev + uiox_hw for a 64-bit ARM target.
 */

/* ── Identity ─────────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "arm64"
#define UIOX_ARCH_ARM64         1
#define UIOX_BITS               64
#define UIOX_ENDIAN_LITTLE      1

/* ── Word / pointer sizes ─────────────────────────────────── */
#define UIOX_PTR_SIZE           8       /* bytes                   */
#define UIOX_PAGE_SIZE          4096    /* 4 KB pages              */
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64      /* bytes                   */

/* ── Physical memory map (QEMU virt machine) ──────────────── */
#define PHYS_DRAM_BASE          0x40000000UL
#define PHYS_DRAM_SIZE          0x08000000UL    /* 128 MB          */
#define PHYS_MMIO_BASE          0x08000000UL
#define PHYS_MMIO_SIZE          0x01000000UL

/* ── GIC-400 interrupt controller ────────────────────────── */
#define GIC_DIST_BASE           0x08000000UL    /* GICD            */
#define GIC_CPU_BASE            0x08010000UL    /* GICC            */
#define GIC_DIST_CTLR           (GIC_DIST_BASE + 0x000)
#define GIC_DIST_ISENABLER0     (GIC_DIST_BASE + 0x100)
#define GIC_CPU_CTLR            (GIC_CPU_BASE  + 0x000)
#define GIC_CPU_IAR             (GIC_CPU_BASE  + 0x00C)
#define GIC_CPU_EOIR            (GIC_CPU_BASE  + 0x010)

/* ── PL011 UART ───────────────────────────────────────────── */
#define UART0_BASE              0x09000000UL
#define UART0_DR                (UART0_BASE + 0x000)   /* data        */
#define UART0_FR                (UART0_BASE + 0x018)   /* flag        */
#define UART0_IBRD              (UART0_BASE + 0x024)   /* baud int    */
#define UART0_FBRD              (UART0_BASE + 0x028)   /* baud frac   */
#define UART0_LCR_H             (UART0_BASE + 0x02C)   /* line ctrl   */
#define UART0_CR                (UART0_BASE + 0x030)   /* control     */
#define UART0_IMSC              (UART0_BASE + 0x038)   /* irq mask    */
#define UART0_ICR               (UART0_BASE + 0x044)   /* irq clear   */
#define UART0_IRQ               33

/* ── SP804 Timer ──────────────────────────────────────────── */
#define TIMER0_BASE             0x09010000UL
#define TIMER0_LOAD             (TIMER0_BASE + 0x000)
#define TIMER0_VALUE            (TIMER0_BASE + 0x004)
#define TIMER0_CTRL             (TIMER0_BASE + 0x008)
#define TIMER0_INTCLR           (TIMER0_BASE + 0x00C)
#define TIMER0_IRQ              34

/* ── VirtIO block device ──────────────────────────────────── */
#define VIRTIO_BLK_BASE         0x0A000000UL
#define VIRTIO_BLK_IRQ          48

/* ── ARM64 exception levels ──────────────────────────────── */
#define EL0                     0
#define EL1                     1       /* kernel runs here        */
#define EL2                     2       /* hypervisor              */
#define EL3                     3       /* secure monitor          */

/* ── DAIF mask bits ───────────────────────────────────────── */
#define DAIF_DBG                (1u << 9)
#define DAIF_SERR               (1u << 8)
#define DAIF_IRQ                (1u << 7)
#define DAIF_FIQ                (1u << 6)

/* ── SPSR_EL1 mode bits ───────────────────────────────────── */
#define SPSR_EL1_EL1H           0x05    /* EL1 with SP_EL1         */
#define SPSR_EL1_EL0T           0x00    /* EL0 with SP_EL0         */

/* ── Stack / vector table alignment ─────────────────────── */
#define STACK_ALIGN             16      /* ABI requires 16-byte    */
#define VECTOR_TABLE_ALIGN      2048    /* VBAR_EL1 must be 2KB-aligned */

/* ── Compiler / ABI helpers ──────────────────────────────── */
#define ARCH_INLINE             __attribute__((always_inline)) inline
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_SECTION(s)         __attribute__((section(s)))

/* ── Barrier macros ───────────────────────────────────────── */
#define arch_mb()   __asm__ volatile("dmb ish\n\t"   ::: "memory")
#define arch_rmb()  __asm__ volatile("dmb ishld\n\t" ::: "memory")
#define arch_wmb()  __asm__ volatile("dmb ishst\n\t" ::: "memory")
#define arch_isb()  __asm__ volatile("isb\n\t"       ::: "memory")
#define arch_dsb()  __asm__ volatile("dsb ish\n\t"   ::: "memory")
#define arch_nop()  __asm__ volatile("nop\n\t")
#define arch_wfi()  __asm__ volatile("wfi\n\t")
#define arch_wfe()  __asm__ volatile("wfe\n\t")
#define arch_sev()  __asm__ volatile("sev\n\t")

/* ── IRQ control ─────────────────────────────────────────── */
#define arch_irq_disable() \
    __asm__ volatile("msr daifset, #2\n\t" ::: "memory")

#define arch_irq_enable() \
    __asm__ volatile("msr daifclr, #2\n\t" ::: "memory")

#define arch_irq_save(flags) \
    __asm__ volatile("mrs %0, daif\n\t msr daifset, #2\n\t" \
                     : "=r"(flags) :: "memory")

#define arch_irq_restore(flags) \
    __asm__ volatile("msr daif, %0\n\t" :: "r"(flags) : "memory")

/* ── System register read helpers ────────────────────────── */
#define read_sysreg(r) \
    ({ uint64_t _v; __asm__ volatile("mrs %0, " #r : "=r"(_v)); _v; })

#define write_sysreg(v, r) \
    __asm__ volatile("msr " #r ", %0\n\t" :: "r"((uint64_t)(v)) : "memory")

/* ── Cross-layer UIOX config ─────────────────────────────── */
/* filesystem geometry — same across all arch targets        */
#define UIOX_BLOCK_SIZE         512
#define UIOX_MAX_BLOCKS         1024
#define UIOX_MAX_INODES         128

/* device layer */
#define UIOX_MAJOR_BLK_MAX      8
#define UIOX_MAJOR_CHR_MAX      8
#define UIOX_CBLOCK_POOL        256

/* hw layer */
#define UIOX_IRQ_MAX            64
#define UIOX_DMA_MAX_DESC       16
#define UIOX_MMIO_REGIONS       8

#endif /* UIOX_ARCH_DEFS_ARM64_H */
