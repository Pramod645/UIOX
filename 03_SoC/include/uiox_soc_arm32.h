/*
 * 10_Arch/arm32/include/uiox_soc_arm32.h
 * UIOX ARM32 SoC — Cortex-A7/A9/A15 specific defines.
 *
 * Extends arch_defs.h with SCU (Snoop Control Unit) registers,
 * Cortex-A9 per-CPU timer (private timer), L2C-310 cache
 * controller offsets, and CP15 system register accessors.
 */
#ifndef UIOX_SOC_ARM32_H
#define UIOX_SOC_ARM32_H

#include "arch_defs.h"
#include "../../../02_FwHal/include/uiox_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Cortex-A9 MPCore — SCU (Snoop Control Unit)
 * Base address for QEMU versatilepb / virt: 0x1E000000
 * ====================================================================== */
#define SCU_BASE                0x1E000000UL
#define SCU_CTRL                (SCU_BASE + 0x000u)  /**< SCU Control      */
#define SCU_CFG                 (SCU_BASE + 0x004u)  /**< SCU Config       */
#define SCU_CPU_PWR_STATUS      (SCU_BASE + 0x008u)  /**< CPU power status */
#define SCU_INVALIDATE          (SCU_BASE + 0x00Cu)  /**< Invalidate all   */
#define SCU_FILTSTART           (SCU_BASE + 0x040u)  /**< Filter start     */
#define SCU_FILTEND             (SCU_BASE + 0x044u)  /**< Filter end       */
#define SCU_CTRL_ENABLE         (1u << 0)
#define SCU_CTRL_ADDR_FILTER    (1u << 1)
#define SCU_CTRL_SCU_SPECFILL   (1u << 3)

/* =========================================================================
 * Cortex-A9 Private Timer (per-CPU, at SCU_BASE + 0x200)
 * ====================================================================== */
#define PTIMER_BASE             (SCU_BASE + 0x200u)
#define PTIMER_LOAD             (PTIMER_BASE + 0x00u)
#define PTIMER_COUNTER          (PTIMER_BASE + 0x04u)
#define PTIMER_CTRL             (PTIMER_BASE + 0x08u)
#define PTIMER_INT_STATUS       (PTIMER_BASE + 0x0Cu)
#define PTIMER_CTRL_ENABLE      (1u << 0)
#define PTIMER_CTRL_AUTO_RELOAD (1u << 1)
#define PTIMER_CTRL_IRQ_EN      (1u << 2)
#define PTIMER_IRQ              29  /* PPI on Cortex-A9                    */

/* =========================================================================
 * L2C-310 PL310 (ARM L2 Cache Controller) — optional on Cortex-A9
 * ====================================================================== */
#define L2C310_BASE             0x1E00A000UL
#define L2C310_CTRL             (L2C310_BASE + 0x100u)
#define L2C310_AUX_CTRL         (L2C310_BASE + 0x104u)
#define L2C310_INV_WAY          (L2C310_BASE + 0x77Cu)
#define L2C310_CLEAN_WAY        (L2C310_BASE + 0x7BCu)
#define L2C310_CLEAN_INV_WAY    (L2C310_BASE + 0x7FCu)
#define L2C310_CACHE_SYNC       (L2C310_BASE + 0x730u)
#define L2C310_CTRL_ENABLE      (1u << 0)

/* =========================================================================
 * Cortex-A7 / A9 cache geometry (QEMU defaults)
 * ====================================================================== */
#define A9_L1_ICACHE_KB         32u
#define A9_L1_DCACHE_KB         32u
#define A9_L2_CACHE_KB          256u   /* PL310 shared L2                 */

#define A7_L1_ICACHE_KB         32u
#define A7_L1_DCACHE_KB         32u
#define A7_L2_CACHE_KB          512u

/* =========================================================================
 * CP15 register accessor macros (ARM32 coprocessor read/write)
 * ====================================================================== */
#define CP15_READ(reg)  ({ uint32_t _v; \
    __asm__ volatile("mrc " reg : "=r"(_v) :: "memory"); _v; })
#define CP15_WRITE(reg, v) \
    __asm__ volatile("mcr " reg :: "r"((uint32_t)(v)) : "memory")

/* MIDR read */
#define ARM32_READ_MIDR()   CP15_READ("p15, 0, %0, c0, c0, 0")
/* MPIDR read */
#define ARM32_READ_MPIDR()  CP15_READ("p15, 0, %0, c0, c0, 5")
/* SCTLR: system control register */
#define ARM32_READ_SCTLR()  CP15_READ("p15, 0, %0, c1, c0, 0")
#define ARM32_WRITE_SCTLR(v) CP15_WRITE("p15, 0, %0, c1, c0, 0", v)
/* ACTLR: auxiliary control (SMP bit) */
#define ARM32_READ_ACTLR()  CP15_READ("p15, 0, %0, c1, c0, 1")
#define ARM32_WRITE_ACTLR(v) CP15_WRITE("p15, 0, %0, c1, c0, 1", v)
/* ACTLR SMP bit for Cortex-A9 */
#define ACTLR_SMP_BIT       (1u << 6)

/* =========================================================================
 * ARM32 SoC init entry points
 * ====================================================================== */
int  uiox_soc_arm32_init(void);
void uiox_soc_arm32_fini(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_ARM32_H */
