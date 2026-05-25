#ifndef ARM64_EXCEPTIONS_H
#define ARM64_EXCEPTIONS_H
/*
 * arm64_exceptions.h — AArch64 exception vector table
 * Reference: ARM DDI 0487, Section D1.10
 *
 * AArch64 VBAR_ELn points to a 2 KB-aligned vector table.
 * Each of the 4 groups has 4 entries of 128 bytes (32 instructions):
 *
 *   Offset 0x000  Current EL, SP0  — Synchronous
 *   Offset 0x080  Current EL, SP0  — IRQ / vIRQ
 *   Offset 0x100  Current EL, SP0  — FIQ / vFIQ
 *   Offset 0x180  Current EL, SP0  — SError / vSError
 *
 *   Offset 0x200  Current EL, SPn  — Synchronous
 *   Offset 0x280  Current EL, SPn  — IRQ
 *   Offset 0x300  Current EL, SPn  — FIQ
 *   Offset 0x380  Current EL, SPn  — SError
 *
 *   Offset 0x400  Lower EL, AArch64 — Synchronous
 *   Offset 0x480  Lower EL, AArch64 — IRQ
 *   Offset 0x500  Lower EL, AArch64 — FIQ
 *   Offset 0x580  Lower EL, AArch64 — SError
 *
 *   Offset 0x600  Lower EL, AArch32 — Synchronous
 *   Offset 0x680  Lower EL, AArch32 — IRQ
 *   Offset 0x700  Lower EL, AArch32 — FIQ
 *   Offset 0x780  Lower EL, AArch32 — SError
 */
#include "arm64_types.h"

/* ── Exception vector offsets from VBAR_ELn ─────────────── */
#define ARM64_VEC_CUR_SP0_SYNC    0x000u
#define ARM64_VEC_CUR_SP0_IRQ     0x080u
#define ARM64_VEC_CUR_SP0_FIQ     0x100u
#define ARM64_VEC_CUR_SP0_SERR    0x180u

#define ARM64_VEC_CUR_SPN_SYNC    0x200u
#define ARM64_VEC_CUR_SPN_IRQ     0x280u
#define ARM64_VEC_CUR_SPN_FIQ     0x300u
#define ARM64_VEC_CUR_SPN_SERR    0x380u

#define ARM64_VEC_LOW_A64_SYNC    0x400u
#define ARM64_VEC_LOW_A64_IRQ     0x480u
#define ARM64_VEC_LOW_A64_FIQ     0x500u
#define ARM64_VEC_LOW_A64_SERR    0x580u

#define ARM64_VEC_LOW_A32_SYNC    0x600u
#define ARM64_VEC_LOW_A32_IRQ     0x680u
#define ARM64_VEC_LOW_A32_FIQ     0x700u
#define ARM64_VEC_LOW_A32_SERR    0x780u

#define ARM64_NUM_VECTORS         16u
#define ARM64_VECTOR_ENTRY_SIZE   0x80u   /* 128 bytes / 32 instructions */
#define ARM64_VECTOR_TABLE_SIZE   0x800u  /* 2 KB total                  */

/* ── Exception syndrome register (ESR_ELn) EC field ─────── */
#define ARM64_EC_UNKNOWN          0x00u   /* Unknown reason               */
#define ARM64_EC_TRAP_WF          0x01u   /* WFI/WFE trap                 */
#define ARM64_EC_TRAP_FP          0x07u   /* FP access trap               */
#define ARM64_EC_ILLEGAL          0x0Eu   /* Illegal execution state      */
#define ARM64_EC_SVC_A64          0x15u   /* SVC in AArch64               */
#define ARM64_EC_HVC_A64          0x16u   /* HVC in AArch64               */
#define ARM64_EC_SMC_A64          0x17u   /* SMC in AArch64               */
#define ARM64_EC_MRS_MSR          0x18u   /* MRS/MSR/System instruction   */
#define ARM64_EC_INST_ABORT_LOW   0x20u   /* Instruction abort, lower EL  */
#define ARM64_EC_INST_ABORT_CUR   0x21u   /* Instruction abort, current EL*/
#define ARM64_EC_PC_ALIGN         0x22u   /* PC alignment fault           */
#define ARM64_EC_DATA_ABORT_LOW   0x24u   /* Data abort, lower EL         */
#define ARM64_EC_DATA_ABORT_CUR   0x25u   /* Data abort, current EL       */
#define ARM64_EC_SP_ALIGN         0x26u   /* SP alignment fault           */
#define ARM64_EC_SERROR           0x2Fu   /* SError interrupt             */
#define ARM64_EC_BRK_A64          0x3Cu   /* BRK in AArch64               */

/* ── Exception types ─────────────────────────────────────── */
typedef enum arm64_exception {
    ARM64_EXC_SYNC      = 0,   /* Synchronous exception              */
    ARM64_EXC_IRQ       = 1,   /* IRQ interrupt                      */
    ARM64_EXC_FIQ       = 2,   /* FIQ interrupt                      */
    ARM64_EXC_SERROR    = 3,   /* System error (async abort)         */
} arm64_exception_t;

/* ── Exception frame saved on the stack by the handler ───── */
typedef struct arm64_exc_frame {
    arm64_word_t  x[31];       /* X0–X30 saved                       */
    arm64_word_t  sp;          /* SP at time of exception            */
    arm64_word_t  pc;          /* ELR_ELn — faulting PC              */
    arm64_word_t  pstate;      /* SPSR_ELn — saved PSTATE            */
    arm64_word_t  esr;         /* ESR_ELn  — syndrome register       */
    arm64_word_t  far;         /* FAR_ELn  — fault address register  */
} arm64_exc_frame_t;

/* ── Exception handler function pointer type ─────────────── */
typedef void (*arm64_exc_handler_t)(arm64_exception_t    exc,
                                    const arm64_exc_frame_t *frame);

/* ── Access macro for vector table ──────────────────────── */
#define ARM64_VECTOR_TABLE_AT(vbar, offset) \
    ((arm64_instr_t *)((arm64_addr_t)(vbar) + (offset)))

#endif /* ARM64_EXCEPTIONS_H */
