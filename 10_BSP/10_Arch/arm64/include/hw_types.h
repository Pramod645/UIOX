/*
 * 10_BSP/10_Arch/arm64/include/hw_types.h
 *
 * ARM64 (AArch64) hardware context type.
 *
 * hw_context_t captures the CPU register state at EL1 exception entry.
 * Layout mirrors the save order in 10_Arch/arm64/src/vectors.S:
 *   x0–x30 (31 GPRs), sp_el0, elr_el1, spsr_el1.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_HW_TYPES_ARM64_H
#define UIOX_HW_TYPES_ARM64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hw_context {
    uint64_t  x[31];      /* x0–x30 general-purpose registers             */
    uint64_t  sp_el0;     /* EL0 stack pointer (saved at EL1 entry)        */
    uint64_t  elr_el1;    /* Exception Link Register — return address      */
    uint64_t  spsr_el1;   /* Saved Program Status Register                 */
    int       irq_num;    /* IRQ number — filled by irq_dispatch()          */
    uint32_t  reserved;
} hw_context_t;

typedef void (*hw_irq_handler_t)(int irq, hw_context_t *ctx, void *dev_id);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_HW_TYPES_ARM64_H */
