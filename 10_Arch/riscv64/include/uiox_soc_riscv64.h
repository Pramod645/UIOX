/*
 * 10_Arch/riscv64/include/uiox_soc_riscv64.h
 * UIOX RISC-V 64-bit SoC — CLINT, PLIC, SiFive-specific defines.
 *
 * Provides SBI call wrappers, PLIC context helpers, and
 * SiFive-specific cache management registers.
 */
#ifndef UIOX_SOC_RISCV64_H
#define UIOX_SOC_RISCV64_H

#include "arch_defs.h"
#include "../../../02_FwHal/include/uiox_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * PLIC context assignments (QEMU virt convention)
 *   context 0  = hart 0 M-mode
 *   context 1  = hart 0 S-mode   ← UIOX kernel uses this
 *   context 2  = hart 1 M-mode
 *   context 3  = hart 1 S-mode
 * ====================================================================== */
#define PLIC_CTX_SMODE(hart)    ((uint32_t)(1u + (hart)*2u))
#define PLIC_CTX_MMODE(hart)    ((uint32_t)(0u + (hart)*2u))

/* =========================================================================
 * SBI inline wrappers
 * ====================================================================== */
typedef struct { long error; long value; } sbi_ret_t;

static inline sbi_ret_t sbi_call(uint64_t eid, uint64_t fid,
                                  uint64_t a0, uint64_t a1,
                                  uint64_t a2, uint64_t a3)
{
    register uint64_t _a0 __asm__("a0") = a0;
    register uint64_t _a1 __asm__("a1") = a1;
    register uint64_t _a2 __asm__("a2") = a2;
    register uint64_t _a3 __asm__("a3") = a3;
    register uint64_t _a6 __asm__("a6") = fid;
    register uint64_t _a7 __asm__("a7") = eid;
    __asm__ volatile("ecall"
        : "+r"(_a0), "+r"(_a1)
        : "r"(_a2), "r"(_a3), "r"(_a6), "r"(_a7)
        : "memory");
    return (sbi_ret_t){ .error = (long)_a0, .value = (long)_a1 };
}

/* SBI convenience macros */
#define SBI_PUTCHAR(c)    sbi_call(0x01u, 0u, (uint64_t)(c), 0,0,0)
#define SBI_SET_TIMER(t)  sbi_call(0x54494D45u, 0u, (uint64_t)(t), 0,0,0)
#define SBI_SEND_IPI(m)   sbi_call(0x735049u, 0u, (uint64_t)(m), 0,0,0)
#define SBI_FENCE_I()     sbi_call(0x52464E43u, 0u, 0,0,0,0)
#define SBI_HART_START(h,e,p) sbi_call(0x48534Du, 0u, (h),(e),(p),0)
#define SBI_HART_STOP()   sbi_call(0x48534Du, 1u, 0,0,0,0)
#define SBI_POWEROFF()    sbi_call(0x53525354u, 0u, 0,0,0,0)
#define SBI_REBOOT()      sbi_call(0x53525354u, 0u, 1,0,0,0)

/* =========================================================================
 * SiFive U74 — L2 cache controller (if present)
 * ====================================================================== */
#define SIFIVE_L2CC_BASE        0x02010000UL
#define SIFIVE_L2CC_CONFIG      (SIFIVE_L2CC_BASE + 0x000u)
#define SIFIVE_L2CC_WAYS        (SIFIVE_L2CC_BASE + 0x008u)
#define SIFIVE_L2CC_FLUSH64     (SIFIVE_L2CC_BASE + 0x200u)
#define SIFIVE_L2CC_FLUSH32     (SIFIVE_L2CC_BASE + 0x240u)

/* =========================================================================
 * RISC-V 64 SoC init entry points
 * ====================================================================== */
int  uiox_soc_riscv64_init(void);
void uiox_soc_riscv64_fini(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SOC_RISCV64_H */
