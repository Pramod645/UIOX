/*
 * 10_BSP/src/uiox_bsp_stubs.c
 *
 * Weak stub implementations for symbols referenced by BSP arch/SoC
 * code but implemented in the kernel or driver layer.
 *
 * Allows the dynamic BSP binary (uiox_bsp.elf) to link standalone.
 * All stubs are __attribute__((weak)) so the kernel real symbols
 * override them at final link time (static build).
 *
 * @version 1.0.0  @date 2026-07-22
 */

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"

struct hw_context;   /* forward-declare; stubs do not dereference it */

/* uiox_soc_init ------------------------------------------------------- */
/* Called by uiox_bsp_init() and uiox_bsp_entry_c().                      */
/* Real impl: 03_SoC/src/uiox_soc_main.c (compiled into libbsp.a         */
/* for static builds; not present in standalone dynamic ELF).             */
__attribute__((weak))
int uiox_soc_init(void)
{
    return 0;   /* UIOX_SOC_OK */
}

/* IRQ handler stubs (ARM64 / ARM32 / RV64 naming) --------------------- */
/* arch_init.c registers these via irq_request().                         */
/* Real impls live in the kernel driver layer.                            */
__attribute__((weak))
void timer0_irq_handler(int irq, struct hw_context *ctx, void *dev_id)
{ (void)irq; (void)ctx; (void)dev_id; }

__attribute__((weak))
void uart0_irq_handler(int irq, struct hw_context *ctx, void *dev_id)
{ (void)irq; (void)ctx; (void)dev_id; }

/* IRQ handler stubs (x86-64 naming) ----------------------------------- */
__attribute__((weak))
void timer_irq_handler(int irq, struct hw_context *ctx, void *dev_id)
{ (void)irq; (void)ctx; (void)dev_id; }

__attribute__((weak))
void uart_irq_handler(int irq, struct hw_context *ctx, void *dev_id)
{ (void)irq; (void)ctx; (void)dev_id; }

/* syscall_dispatch ----------------------------------------------------- */
/* Called by arch_syscall_dispatch() in arch_runtime.c.                   */
/* Real impl: kernel process-control subsystem.                          */
__attribute__((weak))
long syscall_dispatch(long nr, long a0, long a1, long a2,
                      long a3, long a4, long a5)
{
    (void)nr; (void)a0; (void)a1; (void)a2;
    (void)a3; (void)a4; (void)a5;
    return -1L;
}

/* exception_dispatch --------------------------------------------------- */
/* Called by arm32_exception_dispatch() in arch_runtime.c.                */
/* Real impl: kernel exception handler in process-control subsystem.      */
__attribute__((weak))
void exception_dispatch(unsigned int type, struct hw_context *ctx)
{
    (void)type; (void)ctx;
}
