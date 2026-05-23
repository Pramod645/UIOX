#ifndef ARM_EXCEPTIONS_H
#define ARM_EXCEPTIONS_H

/*
 * arm_exceptions.h — ARM exception vector table
 * Reference: ARM Instruction Set, Section 2.6
 *
 * Exception vector addresses:
 *   0x00000000  Reset
 *   0x00000004  Undefined Instruction
 *   0x00000008  Software Interrupt (SWI)
 *   0x0000000C  Prefetch Abort
 *   0x00000010  Data Abort
 *   0x00000014  Reserved
 *   0x00000018  IRQ
 *   0x0000001C  FIQ
 */

#include "arm_types.h"

/* ── Exception vector addresses ─────────────────────────── */
#define ARM_VEC_RESET       0x00000000u
#define ARM_VEC_UNDEF       0x00000004u
#define ARM_VEC_SWI         0x00000008u
#define ARM_VEC_PREFETCH    0x0000000Cu
#define ARM_VEC_DATA_ABORT  0x00000010u
#define ARM_VEC_RESERVED    0x00000014u
#define ARM_VEC_IRQ         0x00000018u
#define ARM_VEC_FIQ         0x0000001Cu

/* Number of exception vectors */
#define ARM_NUM_VECTORS     8u

/* ── Exception types ─────────────────────────────────────── */
typedef enum arm_exception {
    ARM_EXC_RESET      = 0,
    ARM_EXC_UNDEF      = 1,
    ARM_EXC_SWI        = 2,
    ARM_EXC_PREFETCH   = 3,
    ARM_EXC_DATA_ABORT = 4,
    ARM_EXC_RESERVED   = 5,
    ARM_EXC_IRQ        = 6,
    ARM_EXC_FIQ        = 7,
} arm_exception_t;

/* ── Exception handler function pointer type ─────────────── */
typedef void (*arm_exc_handler_t)(arm_exception_t exc,
                                   arm_word_t      cpsr,
                                   arm_word_t      pc);

/* ── Exception vector table struct ──────────────────────── */
typedef struct arm_vector_table {
    arm_instr_t vec[ARM_NUM_VECTORS]; /* B/LDR instruction at each */
} arm_vector_table_t;

/* ── Access macro for vector table ──────────────────────── */
#define ARM_VECTOR_TABLE_AT(base) \
    ((arm_vector_table_t *)(arm_addr_t)(base))

/* ── Macro to write a B instruction into a vector slot ───── */
#define ARM_SET_VECTOR(base, vec_idx, handler_addr) \
    do { \
        arm_addr_t _vpc = (arm_addr_t)(base) + (vec_idx)*4u; \
        arm_addr_t _tgt = (arm_addr_t)(handler_addr); \
        arm_int32_t _off = (arm_int32_t)((_tgt - _vpc - 8u) / 4u); \
        *((arm_instr_t *)_vpc) = ARM_BUILD_BAL(_off); \
    } while(0)

/* ── Exception handler prototypes ───────────────────────── */
void arm_exc_reset     (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_undef     (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_swi       (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_prefetch  (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_data_abort(arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_irq       (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);
void arm_exc_fiq       (arm_exception_t e, arm_word_t cpsr, arm_word_t pc);

#endif /* ARM_EXCEPTIONS_H */
