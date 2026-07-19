#ifndef UIOX_ARCH_DEFS_ARM64_H
#define UIOX_ARCH_DEFS_ARM64_H
/*
 * 10_Arch/arm64/include/arch_defs.h
 * ARMv8-A (AArch64) architecture definitions.
 *
 * This file defines ONLY what the architecture layer owns:
 *   - ISA constants (instruction set, register widths, page geometry)
 *   - CPU system register bit-fields (SCTLR_EL1, DAIF, etc.)
 *   - Interrupt model (GIC CPU interface layout — same on all ARM64)
 *   - QEMU-default physical memory map (overridden per SoC in 03_SoC)
 *   - Barrier / cache / TLB macros
 *
 * What it does NOT define:
 *   - UART base address  (SoC-specific → 03_SoC/include/uiox_soc_map.h)
 *   - PLL / clock tree   (SoC-specific)
 *   - IRQ numbers        (SoC-specific)
 *   - Power domains      (SoC-specific)
 */

/* ── Architecture identity ────────────────────────────────────────── */
#define UIOX_ARCH_NAME          "arm64"
#define UIOX_ARCH_BITS          64
#define UIOX_ARCH_LITTLE_ENDIAN 1

/* ── Word / pointer sizes ─────────────────────────────────────────── */
#define UIOX_PTR_SIZE           8
#define UIOX_PAGE_SIZE          4096
#define UIOX_PAGE_SHIFT         12
#define UIOX_CACHE_LINE         64

/* ── Physical memory map (QEMU virt machine defaults) ────────────────
 * These are overridden per SoC by 03_SoC/include/uiox_soc_map.h.
 * ──────────────────────────────────────────────────────────────────── */
#define PHYS_DRAM_BASE          0x40000000UL
#define PHYS_DRAM_SIZE          0x04000000UL    /* 64 MB           */
#define PHYS_MMIO_BASE          0x10000000UL
#define PHYS_MMIO_SIZE          0x01000000UL

/* ── GIC-400 (common register layout — same on all ARM64 SoCs) ───────
 * Base addresses are the QEMU virt defaults; chip-specific SoC backends
 * in 03_SoC override these via SOC_GIC_DIST_BASE / SOC_GIC_CPU_BASE.
 * ──────────────────────────────────────────────────────────────────── */
#define GIC_DIST_BASE           0x08000000UL
#define GIC_CPU_BASE            0x08010000UL

/* GIC distributor register offsets (architecture-defined) */
#define GIC_DIST_CTLR           (GIC_DIST_BASE + 0x000U)
#define GIC_DIST_TYPER          (GIC_DIST_BASE + 0x004U)
#define GIC_DIST_ISENABLER0     (GIC_DIST_BASE + 0x100U)
#define GIC_DIST_IPRIORITYR0    (GIC_DIST_BASE + 0x400U)
#define GIC_DIST_ITARGETSR0     (GIC_DIST_BASE + 0x800U)
#define GIC_DIST_ICFGR0         (GIC_DIST_BASE + 0xC00U)

/* GIC CPU interface register offsets */
#define GIC_CPU_CTLR            (GIC_CPU_BASE  + 0x000U)
#define GIC_CPU_PMR             (GIC_CPU_BASE  + 0x004U)
#define GIC_CPU_IAR             (GIC_CPU_BASE  + 0x00CU)
#define GIC_CPU_EOIR            (GIC_CPU_BASE  + 0x010U)

/* ── ARMv8-A system register bit definitions ──────────────────────── */

/* SCTLR_EL1 bits */
#define SCTLR_EL1_M             (1UL <<  0)   /* MMU enable             */
#define SCTLR_EL1_C             (1UL <<  2)   /* D-cache enable         */
#define SCTLR_EL1_I             (1UL << 12)   /* I-cache enable         */
#define SCTLR_EL1_WXN           (1UL << 19)   /* Write implies XN       */
#define SCTLR_EL1_EE            (1UL << 25)   /* Exception endianness   */
#define SCTLR_EL1_UCI           (1UL << 26)   /* User cache instructions*/

/* DAIF bits */
#define DAIF_D_BIT              (1U << 3)     /* Debug mask             */
#define DAIF_A_BIT              (1U << 2)     /* SError mask            */
#define DAIF_I_BIT              (1U << 1)     /* IRQ mask               */
#define DAIF_F_BIT              (1U << 0)     /* FIQ mask               */

/* SPSR_EL1 / PSTATE */
#define PSTATE_EL1h             0x05U         /* EL1, use SP_EL1        */
#define PSTATE_EL0t             0x00U         /* EL0, use SP_EL0        */
#define PSTATE_I_BIT            (1U << 7)     /* IRQ mask in PSTATE     */
#define PSTATE_F_BIT            (1U << 6)     /* FIQ mask               */
#define PSTATE_A_BIT            (1U << 8)     /* SError mask            */
#define PSTATE_D_BIT            (1U << 9)     /* Debug mask             */

/* TCR_EL1 — Translation Control Register */
#define TCR_T0SZ(x)             ((unsigned long)(x))
#define TCR_T1SZ(x)             ((unsigned long)(x) << 16)
#define TCR_TG0_4K              (0UL << 14)
#define TCR_TG1_4K              (2UL << 30)
#define TCR_IPS_40BIT           (2UL << 32)
#define TCR_IRGN0_WBWA          (1UL <<  8)
#define TCR_ORGN0_WBWA          (1UL << 10)
#define TCR_SH0_INNER           (3UL << 12)

/* ── Privilege levels ─────────────────────────────────────────────── */
#define ARCH_EL0                0U
#define ARCH_EL1                1U
#define ARCH_EL2                2U
#define ARCH_EL3                3U

/* ── Generic Timer (architecture-defined — same on all ARMv8-A) ─────
 * Actual frequency read from CNTFRQ_EL0 at runtime.
 * ──────────────────────────────────────────────────────────────────── */
#define ARCH_TIMER_IRQ_PHYS     30U           /* EL1 physical timer PPI */
#define ARCH_TIMER_IRQ_VIRT     27U           /* EL1 virtual timer PPI  */

/* ── Memory attribute constants ───────────────────────────────────── */
#define MT_DEVICE_nGnRnE        0U
#define MT_DEVICE_nGnRE         1U
#define MT_NORMAL_NC            2U
#define MT_NORMAL               3U

/* ── Alignment helpers ────────────────────────────────────────────── */
#define ARCH_ALIGN_UP(v, a)     (((v) + ((a) - 1UL)) & ~((a) - 1UL))
#define ARCH_ALIGN_DOWN(v, a)   ((v) & ~((a) - 1UL))
#define ARCH_IS_ALIGNED(v, a)   (((v) & ((a) - 1UL)) == 0UL)
#define ARCH_PAGE_ALIGN(v)      ARCH_ALIGN_UP(v, UIOX_PAGE_SIZE)

/* ── Barrier macros (architecture instructions — not SoC registers) ─ */
#define arch_dsb_sy()   __asm__ volatile("dsb sy"   ::: "memory")
#define arch_dsb_st()   __asm__ volatile("dsb st"   ::: "memory")
#define arch_dsb_ld()   __asm__ volatile("dsb ld"   ::: "memory")
#define arch_isb()      __asm__ volatile("isb"      ::: "memory")
#define arch_dmb_sy()   __asm__ volatile("dmb sy"   ::: "memory")
#define arch_wfi()      __asm__ volatile("wfi")
#define arch_wfe()      __asm__ volatile("wfe")
#define arch_sev()      __asm__ volatile("sev")
#define arch_nop()      __asm__ volatile("nop")

/* ── TLB invalidation ─────────────────────────────────────────────── */
#define arch_tlbi_alle1()        __asm__ volatile("tlbi alle1"   ::: "memory")
#define arch_tlbi_vmalle1is()    __asm__ volatile("tlbi vmalle1is"::: "memory")
#define arch_tlbi_vaae1is(va)    __asm__ volatile("tlbi vaae1is, %0" :: "r"(va) : "memory")

/* ── Cache maintenance ────────────────────────────────────────────── */
#define arch_ic_iallu()          __asm__ volatile("ic iallu"     ::: "memory")
#define arch_dc_civac(va)        __asm__ volatile("dc civac, %0" :: "r"(va) : "memory")
#define arch_dc_cvac(va)         __asm__ volatile("dc cvac,  %0" :: "r"(va) : "memory")
#define arch_dc_ivac(va)         __asm__ volatile("dc ivac,  %0" :: "r"(va) : "memory")

/* ── Compiler helpers ─────────────────────────────────────────────── */
#define ARCH_INLINE             static inline __attribute__((always_inline))
#define ARCH_NORETURN           __attribute__((noreturn))
#define ARCH_PACKED             __attribute__((packed))
#define ARCH_ALIGNED(n)         __attribute__((aligned(n)))
#define ARCH_WEAK               __attribute__((weak))
#define ARCH_USED               __attribute__((used))
#define ARCH_UNUSED(x)          ((void)(x))

#endif /* UIOX_ARCH_DEFS_ARM64_H */
