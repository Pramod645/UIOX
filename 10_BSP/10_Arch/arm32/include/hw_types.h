/*
 * 10_BSP/10_Arch/arm32/include/hw_types.h
 *
 * ARM32 (ARMv7-A) hardware context type.
 *
 * hw_context_t captures the CPU register state at IRQ entry.
 * Layout mirrors the save order in 10_Arch/arm32/src/vectors.S:
 *   r0–r15 (16 GPRs), cpsr.
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_HW_TYPES_ARM32_H
#define UIOX_HW_TYPES_ARM32_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hw_context {
    uiox_uint32_t  cpsr;       /* Saved CPSR at IRQ entry                       */
    uiox_uint32_t  spsr;       /* SPSR of the interrupted mode                  */
    int       irq_num;    /* IRQ number — filled by irq_dispatch()          */
    uiox_uint32_t  reserved;
} hw_context_t;

typedef void (*hw_irq_handler_t)(int irq, hw_context_t *ctx, void *dev_id);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_HW_TYPES_ARM32_H */
