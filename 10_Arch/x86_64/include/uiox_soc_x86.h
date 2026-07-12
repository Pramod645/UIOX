/*
 * 10_Arch/x86_64/include/uiox_soc_x86.h
 * UIOX x86-64 SoC — APIC, IOAPIC, HPET, PIT, and MSR defines.
 *
 * Extends arch_defs.h with all register offsets and bit definitions
 * needed by uiox_soc_x86_init.c.
 */
#ifndef UIOX_SOC_X86_H
#define UIOX_SOC_X86_H

#include "arch_defs.h"
#include "../../../02_FwHal/include/uiox_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Local APIC register offsets  (MMIO at SOC_LAPIC_BASE)
 * ====================================================================== */
#define LAPIC_ID            (SOC_LAPIC_BASE + 0x020u)
#define LAPIC_VERSION       (SOC_LAPIC_BASE + 0x030u)
#define LAPIC_TPR           (SOC_LAPIC_BASE + 0x080u)  /**< Task Priority  */
#define LAPIC_EOI           (SOC_LAPIC_BASE + 0x0B0u)  /**< End Of Interrupt */
#define LAPIC_SVR           (SOC_LAPIC_BASE + 0x0F0u)  /**< Spurious IRQ   */
#define LAPIC_ICR_LO        (SOC_LAPIC_BASE + 0x300u)  /**< IPI command lo */
#define LAPIC_ICR_HI        (SOC_LAPIC_BASE + 0x310u)  /**< IPI command hi */
#define LAPIC_LVT_TIMER     (SOC_LAPIC_BASE + 0x320u)  /**< LVT Timer      */
#define LAPIC_LVT_LINT0     (SOC_LAPIC_BASE + 0x350u)
#define LAPIC_LVT_LINT1     (SOC_LAPIC_BASE + 0x360u)
#define LAPIC_LVT_ERROR     (SOC_LAPIC_BASE + 0x370u)
#define LAPIC_TIMER_ICR     (SOC_LAPIC_BASE + 0x380u)  /**< Initial count  */
#define LAPIC_TIMER_CCR     (SOC_LAPIC_BASE + 0x390u)  /**< Current count  */
#define LAPIC_TIMER_DCR     (SOC_LAPIC_BASE + 0x3E0u)  /**< Divide config  */

/* LAPIC SVR bits */
#define LAPIC_SVR_ENABLE    (1u << 8)
#define LAPIC_SVR_SPURIOUS  0xFFu

/* LAPIC LVT mask bit */
#define LAPIC_LVT_MASKED    (1u << 16)

/* LAPIC timer modes */
#define LAPIC_TIMER_ONESHOT     0x00000u
#define LAPIC_TIMER_PERIODIC    0x20000u
#define LAPIC_TIMER_TSC_DEADLINE 0x40000u

/* LAPIC IPI delivery modes */
#define LAPIC_ICR_FIXED     0x00000u
#define LAPIC_ICR_NMI       0x00400u
#define LAPIC_ICR_INIT      0x00500u
#define LAPIC_ICR_STARTUP   0x00600u
#define LAPIC_ICR_LEVEL_ASSERT (1u << 14)

/* =========================================================================
 * I/O APIC register offsets  (MMIO at SOC_IOAPIC_BASE)
 * ====================================================================== */
#define IOAPIC_IOREGSEL     (SOC_IOAPIC_BASE + 0x000u)
#define IOAPIC_IOWIN        (SOC_IOAPIC_BASE + 0x010u)

/* IOAPIC indirect registers */
#define IOAPIC_REG_ID       0x00u
#define IOAPIC_REG_VER      0x01u
#define IOAPIC_REG_REDTBL(n) (0x10u + (n)*2u)  /**< Redirection table lo  */

/* IOAPIC redirection entry bits */
#define IOAPIC_RED_MASKED   (1u << 16)
#define IOAPIC_RED_LEVEL    (1u << 15)
#define IOAPIC_RED_ACTIVELO (1u << 13)
#define IOAPIC_RED_FIXED    0x000u

/* =========================================================================
 * HPET register offsets  (MMIO at SOC_HPET_BASE)
 * ====================================================================== */
#define HPET_GCAP_ID        (SOC_HPET_BASE + 0x000u)  /**< Caps + ID      */
#define HPET_GEN_CFG        (SOC_HPET_BASE + 0x010u)  /**< General Config */
#define HPET_GEN_ISR        (SOC_HPET_BASE + 0x020u)  /**< Interrupt Status */
#define HPET_MAIN_CTR       (SOC_HPET_BASE + 0x0F0u)  /**< Main counter   */
#define HPET_T0_CFG         (SOC_HPET_BASE + 0x100u)  /**< Timer 0 config */
#define HPET_T0_CMP         (SOC_HPET_BASE + 0x108u)  /**< Timer 0 comp   */
#define HPET_T1_CFG         (SOC_HPET_BASE + 0x120u)
#define HPET_T1_CMP         (SOC_HPET_BASE + 0x128u)

#define HPET_CFG_ENABLE     (1u << 0)   /**< Overall enable               */
#define HPET_CFG_LEG_RT     (1u << 1)   /**< Legacy replacement route     */
#define HPET_Tn_INT_EN      (1u << 2)   /**< Timer n interrupt enable     */
#define HPET_Tn_PERIODIC    (1u << 3)   /**< Periodic timer mode          */
#define HPET_Tn_VAL_SET     (1u << 6)   /**< Set accumulator              */

/* =========================================================================
 * MSR numbers used in UIOX x86 SoC layer
 * ====================================================================== */
#define MSR_IA32_APIC_BASE      0x0000001Bu
#define MSR_IA32_MISC_ENABLE    0x000001A0u
#define MSR_IA32_TSC_DEADLINE   0x000006E0u
#define MSR_IA32_EFER           0xC0000080u
#define MSR_IA32_STAR           0xC0000081u
#define MSR_IA32_LSTAR          0xC0000082u
#define MSR_IA32_FMASK          0xC0000084u
#define MSR_IA32_KERNEL_GS_BASE 0xC0000102u
#define MSR_IA32_ENERGY_PERF_BIAS 0x000001B0u

/* =========================================================================
 * MSR read/write helpers (Ring 0 only)
 * ====================================================================== */
static inline uint64_t x86_rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32u) | (uint64_t)lo;
}

static inline void x86_wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t lo = (uint32_t)(val & 0xFFFFFFFFu);
    uint32_t hi = (uint32_t)(val >> 32u);
    __asm__ volatile("wrmsr" :: "c"(msr), "a"(lo), "d"(hi) : "memory");
}

/* =========================================================================
 * IOAPIC indirect access helpers
 * ====================================================================== */
static inline uint32_t ioapic_read(uint8_t reg)
{
    mmio_write32(IOAPIC_IOREGSEL, (uint32_t)reg);
    return mmio_read32(IOAPIC_IOWIN);
}

static inline void ioapic_write(uint8_t reg, uint32_t val)
{
    mmio_write32(IOAPIC_IOREGSEL, (uint32_t)reg);
    mmio_write32(IOAPIC_IOWIN, val);
}

/* =========================================================================
 * x86-64 SoC init entry points
 * ====================================================================== */
int  uiox_soc_x86_init(void);
void uiox_soc_x86_fini(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_X86_H */
