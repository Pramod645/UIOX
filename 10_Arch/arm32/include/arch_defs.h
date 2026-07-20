#ifndef UIOX_ARCH_DEFS_ARM32_H
#define UIOX_ARCH_DEFS_ARM32_H
/*
 * 10_Arch/arm32/include/arch_defs.h
 * ARMv7-A 32-bit architecture definitions.
 */

/* ── Architecture identity ────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "arm32"
#define UIOX_ARCH_BITS          32
#define UIOX_ARCH_LITTLE_ENDIAN 1

/* ── Word / pointer sizes ─────────────────────────────────────────── */
#define UIOX_PTR_SIZE           4
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         32

/* ── Physical memory map (QEMU virt / versatilepb defaults) ──────────
 * Overridden per SoC by 03_SoC/include/uiox_soc_map.h.
 * ──────────────────────────────────────────────────────────────────── */
#define PHYS_DRAM_BASE          0x60000000UL
#define PHYS_DRAM_SIZE          0x04000000UL    /* 64 MB           */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x01000000UL

/* ── GIC-400 (same register layout as ARM64) ──────────────────────── */
#define GIC_DIST_BASE           0x08000000UL
#define GIC_CPU_BASE            0x08010000UL

#define GIC_DIST_CTLR           (GIC_DIST_BASE + 0x000U)
#define GIC_DIST_TYPER          (GIC_DIST_BASE + 0x004U)
#define GIC_DIST_ISENABLER0     (GIC_DIST_BASE + 0x100U)
#define GIC_DIST_IPRIORITYR0    (GIC_DIST_BASE + 0x400U)
#define GIC_DIST_ITARGETSR0     (GIC_DIST_BASE + 0x800U)
#define GIC_DIST_ICFGR0         (GIC_DIST_BASE + 0xC00U)

#define GIC_CPU_CTLR            (GIC_CPU_BASE  + 0x000U)
#define GIC_CPU_PMR             (GIC_CPU_BASE  + 0x004U)
#define GIC_CPU_IAR             (GIC_CPU_BASE  + 0x00CU)
#define GIC_CPU_EOIR            (GIC_CPU_BASE  + 0x010U)

/* ── ARMv7-A CPSR / SCTLR bit definitions ─────────────────────────── */

/* SCTLR bits */
#define SCTLR_M                 (1U <<  0)    /* MMU enable             */
#define SCTLR_C                 (1U <<  2)    /* D-cache enable         */
#define SCTLR_I                 (1U << 12)    /* I-cache enable         */
#define SCTLR_V                 (1U << 13)    /* High vectors           */
#define SCTLR_XP                (1U << 23)    /* Subpage AP bits disable*/

/* CPSR mode bits */
#define CPSR_MODE_USR           0x10U
#define CPSR_MODE_FIQ           0x11U
#define CPSR_MODE_IRQ           0x12U
#define CPSR_MODE_SVC           0x13U
#define CPSR_MODE_MON           0x16U
#define CPSR_MODE_ABT           0x17U
#define CPSR_MODE_HYP           0x1AU
#define CPSR_MODE_UND           0x1BU
#define CPSR_MODE_SYS           0x1FU

/* CPSR interrupt mask bits */
#define CPSR_I_BIT              (1U << 7)     /* IRQ disable            */
#define CPSR_F_BIT              (1U << 6)     /* FIQ disable            */
#define CPSR_A_BIT              (1U << 8)     /* Async abort disable    */
#define CPSR_T_BIT              (1U << 5)     /* Thumb state            */

/* ── Privilege levels ─────────────────────────────────────────────── */
#define ARCH_MODE_SVC           CPSR_MODE_SVC
#define ARCH_MODE_USR           CPSR_MODE_USR
#define ARCH_MODE_HYP           CPSR_MODE_HYP
#define ARCH_MODE_MON           CPSR_MODE_MON

/* ── Generic Timer (ARMv7-A with LPAE / Virtualization extension) ─── */
#define ARCH_TIMER_IRQ_PHYS     29U           /* PPI — physical timer   */
#define ARCH_TIMER_IRQ_VIRT     27U           /* PPI — virtual timer    */

/* ── Alignment helpers ────────────────────────────────────────────── */
#define ARCH_ALIGN_UP(v, a)     (((v) + ((a) - 1UL)) & ~((a) - 1UL))
#define ARCH_ALIGN_DOWN(v, a)   ((v) & ~((a) - 1UL))
#define ARCH_IS_ALIGNED(v, a)   (((v) & ((a) - 1UL)) == 0UL)
#define ARCH_PAGE_ALIGN(v)      ARCH_ALIGN_UP(v, UIOX_PAGE_SIZE)

/* ── Barrier macros ───────────────────────────────────────────────── */
#define arch_dsb_sy()   __asm__ volatile("dsb"   ::: "memory")
#define arch_dsb_st()   __asm__ volatile("dsb st"::: "memory")
#define arch_isb()      __asm__ volatile("isb"   ::: "memory")
#define arch_dmb_sy()   __asm__ volatile("dmb sy"::: "memory")
#define arch_wfi()      __asm__ volatile("wfi")
#define arch_wfe()      __asm__ volatile("wfe")
#define arch_sev()      __asm__ volatile("sev")
#define arch_nop()      __asm__ volatile("nop")

/* ── TLB invalidation (CP15) ─────────────────────────────────────── */
#define arch_tlbi_all()  \
    __asm__ volatile("mcr p15,0,%0,c8,c7,0" :: "r"(0U) : "memory")
#define arch_tlbi_mva(va) \
    __asm__ volatile("mcr p15,0,%0,c8,c7,1" :: "r"(va) : "memory")

/* ── Cache maintenance (CP15) ─────────────────────────────────────── */
#define arch_ic_iallu() \
    __asm__ volatile("mcr p15,0,%0,c7,c5,0" :: "r"(0U) : "memory")
#define arch_dc_cisw(sw) \
    __asm__ volatile("mcr p15,0,%0,c7,c14,2":: "r"(sw) : "memory")

/* ── Compiler helpers ─────────────────────────────────────────────── */
#define ARCH_INLINE             static inline __attribute__((always_inline))
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_USED               __attribute__((used))
#define ARCH_UNUSED(x)          ((void)(x))

#endif /* UIOX_ARCH_DEFS_ARM32_H */