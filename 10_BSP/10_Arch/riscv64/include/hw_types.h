/*
 * 10_BSP/10_Arch/riscv64/include/hw_types.h
 *
 * RISC-V 64 hardware context type.
 *
 * hw_context_t captures the CPU register state at S-mode trap entry.
 * Layout mirrors 10_Arch/riscv64/src/vectors.S save order:
 *   x0–x31 (32 GPRs, x0 always 0), sepc, sstatus, scause, stval.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_HW_TYPES_RISCV64_H
#define UIOX_HW_TYPES_RISCV64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hw_context {
    uint64_t  x[32];      /* x0–x31 (x0 always 0 but saved for uniformity) */
    uint64_t  sepc;       /* Supervisor Exception Program Counter           */
    uint64_t  sstatus;    /* Supervisor Status Register at trap entry        */
    uint64_t  scause;     /* Supervisor Cause Register                       */
    uint64_t  stval;      /* Supervisor Trap Value                           */
    int       irq_num;    /* PLIC interrupt source — filled by irq_dispatch()*/
    uint32_t  reserved;
} hw_context_t;

typedef void (*hw_irq_handler_t)(int irq, hw_context_t *ctx, void *dev_id);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_HW_TYPES_RISCV64_H */
