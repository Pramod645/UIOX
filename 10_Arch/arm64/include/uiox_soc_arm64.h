/*
 * 10_Arch/arm64/include/uiox_soc_arm64.h
 * UIOX ARM64 SoC — Cortex-A76 / GIC-600 specific defines.
 *
 * Extends arch_defs.h with GIC-600 registers, cache topology,
 * Cortex-A76 CPU erratum workarounds, and DSU (DynamIQ Shared Unit)
 * configuration registers.
 */
#ifndef UIOX_SOC_ARM64_H
#define UIOX_SOC_ARM64_H

#include "arch_defs.h"
#include "../../../02_FwHal/include/uiox_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * GIC-600 register extensions (beyond GIC-400 base)
 * ====================================================================== */
#define GIC600_DIST_TYPER2      (SOC_GIC_DIST_BASE + 0x00Cu)
#define GIC600_REDIST_BASE      SOC_GIC_REDIST_BASE
#define GIC600_REDIST_STRIDE    0x20000u   /* 128 KB per redistributor     */
#define GIC600_REDIST_CTLR(n)   (GIC600_REDIST_BASE + (n)*GIC600_REDIST_STRIDE + 0x000u)
#define GIC600_REDIST_WAKER(n)  (GIC600_REDIST_BASE + (n)*GIC600_REDIST_STRIDE + 0x014u)
#define GIC600_REDIST_SGI(n)    (GIC600_REDIST_BASE + (n)*GIC600_REDIST_STRIDE + 0x10000u)

/* GIC-v3 CPU interface system registers (ICC_*) */
#define ICC_SRE_EL1_SRE         (1u << 0)
#define ICC_SRE_EL1_DFB         (1u << 1)
#define ICC_SRE_EL1_DIB         (1u << 2)
#define ICC_CTLR_EL1_EOImode    (1u << 1)
#define ICC_IGRPEN1_EL1_EN      (1u << 0)

/* =========================================================================
 * Cortex-A76 implementation-defined registers
 * ====================================================================== */
/* CPUECTLR_EL1 — CPU Extended Control */
#define A76_CPUECTLR_SMPEN      (1u << 6)   /* SMP enable                 */
#define A76_CPUECTLR_L1PCTL     (7u << 13)  /* L1 prefetch control        */

/* CPUACTLR_EL1 — CPU Auxiliary Control */
#define A76_CPUACTLR_DISABLE_L1_PREFETCH  (1u << 56)

/* =========================================================================
 * DynamIQ Shared Unit (DSU) cluster registers
 * ====================================================================== */
#define DSU_CLUSTERCFR_EL1_MASK 0xFFFFFFFFFFFFFFFFull
#define DSU_CLUSTERIDR_EL1_MASK 0xFFFFFFFFFFFFFFFFull

/* =========================================================================
 * Cortex-A76 cache geometry
 * ====================================================================== */
#define A76_L1_ICACHE_KB        64u
#define A76_L1_DCACHE_KB        64u
#define A76_L2_CACHE_KB         512u    /* Private per-core              */
#define A76_L3_CACHE_KB_MIN     1024u   /* DSU L3 varies by SoC          */

/* =========================================================================
 * ARM64 SoC topology helpers (called from uiox_soc_arm64_init.c)
 * ====================================================================== */

/** Read MPIDR_EL1 and return the CPU's cluster and core indices. */
static inline void arm64_mpidr_decode(uint64_t mpidr,
                                       uint32_t *cluster, uint32_t *core)
{
    *cluster = (uint32_t)((mpidr >> 8u)  & 0xFFu);
    *core    = (uint32_t)((mpidr >> 0u)  & 0xFFu);
}

/** Return true if the CPU is the primary boot CPU (MPIDR Aff0=0, Aff1=0). */
static inline bool arm64_is_primary_cpu(void)
{
    uint64_t mpidr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    return ((mpidr & 0x00FFFFFFull) == 0u);
}

/** Flush and invalidate all D-cache levels up to PoC. */
static inline void arm64_cache_flush_all(void)
{
    __asm__ volatile(
        "dsb  sy  \n\t"
        "ic   iallu\n\t"
        "dsb  sy  \n\t"
        "isb       \n\t"
        ::: "memory"
    );
}

int  uiox_soc_arm64_init(void);
void uiox_soc_arm64_fini(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_ARM64_H */
