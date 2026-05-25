#ifndef ARM64_SYSREGS_H
#define ARM64_SYSREGS_H
/*
 * arm64_sysregs.h — AArch64 system register interface
 * Reference: ARM DDI 0487, Chapter D12
 *
 * System registers are accessed via MRS / MSR instructions.
 * Encoding: op0:op1:CRn:CRm:op2
 */
#include "arm64_types.h"

/* ── System register encodings (for MRS/MSR instr field) ─── */
/* EL1 control registers */
#define ARM64_SYSREG_SCTLR_EL1   0xC080u  /* System control EL1          */
#define ARM64_SYSREG_TTBR0_EL1   0xC100u  /* Trans. table base 0 EL1     */
#define ARM64_SYSREG_TTBR1_EL1   0xC101u  /* Trans. table base 1 EL1     */
#define ARM64_SYSREG_TCR_EL1     0xC102u  /* Trans. control register EL1 */
#define ARM64_SYSREG_MAIR_EL1    0xC510u  /* Mem. attr. indirection EL1  */
#define ARM64_SYSREG_VBAR_EL1    0xC600u  /* Vector base address EL1     */
#define ARM64_SYSREG_ELR_EL1     0xC201u  /* Exception link register EL1 */
#define ARM64_SYSREG_SPSR_EL1    0xC200u  /* Saved program status EL1    */
#define ARM64_SYSREG_SP_EL0      0x4208u  /* Stack pointer EL0           */
#define ARM64_SYSREG_SP_EL1      0xC208u  /* Stack pointer EL1           */
#define ARM64_SYSREG_ESR_EL1     0xC290u  /* Exception syndrome EL1      */
#define ARM64_SYSREG_FAR_EL1     0xC300u  /* Fault address EL1           */
#define ARM64_SYSREG_PAR_EL1     0xC3A0u  /* Phys. address register EL1  */
#define ARM64_SYSREG_DAIF        0xDA11u  /* Debug/IRQ/FIQ/SError mask   */
#define ARM64_SYSREG_NZCV        0xDA10u  /* Condition flags             */
#define ARM64_SYSREG_TPIDR_EL0   0xDE82u  /* Thread ID register EL0      */
#define ARM64_SYSREG_TPIDR_EL1   0xC684u  /* Thread ID register EL1      */
#define ARM64_SYSREG_MIDR_EL1    0xC000u  /* Main ID register            */
#define ARM64_SYSREG_MPIDR_EL1   0xC005u  /* Multiprocessor affinity     */
#define ARM64_SYSREG_CurrentEL   0xD212u  /* Current exception level     */
#define ARM64_SYSREG_CNTFRQ_EL0  0xDF00u  /* Counter frequency           */
#define ARM64_SYSREG_CNTVCT_EL0  0xDF02u  /* Virtual count register      */
#define ARM64_SYSREG_CNTV_CTL_EL0 0xDF19u /* Virtual timer control       */
#define ARM64_SYSREG_CNTV_CVAL_EL0 0xDF1Au/* Virtual timer compare value */

/* ── SCTLR_EL1 bits ──────────────────────────────────────── */
#define ARM64_SCTLR_M     (1u <<  0)   /* MMU enable                    */
#define ARM64_SCTLR_A     (1u <<  1)   /* Alignment check enable        */
#define ARM64_SCTLR_C     (1u <<  2)   /* Data cache enable             */
#define ARM64_SCTLR_SA    (1u <<  3)   /* Stack alignment check EL1     */
#define ARM64_SCTLR_SA0   (1u <<  4)   /* Stack alignment check EL0     */
#define ARM64_SCTLR_I     (1u << 12)   /* Instruction cache enable      */
#define ARM64_SCTLR_WXN   (1u << 19)   /* Write implies execute never   */
#define ARM64_SCTLR_SPAN  (1u << 23)   /* Set PAN on exception entry    */
#define ARM64_SCTLR_EIS   (1u << 22)   /* Exception entry DAIF mask     */

/* ── DAIF bits ───────────────────────────────────────────── */
#define ARM64_DAIF_D      (1u <<  9)   /* Debug exception mask          */
#define ARM64_DAIF_A      (1u <<  8)   /* SError mask                   */
#define ARM64_DAIF_I      (1u <<  7)   /* IRQ mask                      */
#define ARM64_DAIF_F      (1u <<  6)   /* FIQ mask                      */

/* ── System register struct ──────────────────────────────── */
typedef struct arm64_sysregs {
    arm64_word_t sctlr_el1;
    arm64_word_t ttbr0_el1;
    arm64_word_t ttbr1_el1;
    arm64_word_t tcr_el1;
    arm64_word_t mair_el1;
    arm64_word_t vbar_el1;
    arm64_word_t esr_el1;
    arm64_word_t far_el1;
    arm64_word_t tpidr_el0;
    arm64_word_t tpidr_el1;
    arm64_word_t daif;
    arm64_word_t nzcv;
    arm64_word_t cntfrq_el0;
    arm64_word_t cntvct_el0;
} arm64_sysregs_t;

/* ── Read/write system register helpers ──────────────────── */
arm64_word_t arm64_sysreg_read  (arm64_uint32_t reg_enc);
void         arm64_sysreg_write (arm64_uint32_t reg_enc, arm64_word_t val);

#endif /* ARM64_SYSREGS_H */
