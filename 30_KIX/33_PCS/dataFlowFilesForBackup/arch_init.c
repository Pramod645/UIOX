/* Add to arch_init.c — after irq_enable(UART0_IRQ) */

#include "uiox_syscall.h"   /* uiox_syscall_dispatch, uiox_syscall_frame_t */

/*
 * arch_syscall_entry — called from the EL0 sync exception vector
 * when the instruction class in ESR_EL1 is SVC.
 *
 * On entry (set by vector table stub in assembly):
 *   x0–x5  = syscall arguments
 *   x8     = syscall number
 *   sp     = kernel stack (set up by vector table entry)
 *
 * Returns: value placed back in x0 before ERET to EL0.
 */
long arch_syscall_entry(unsigned long nr,
                        unsigned long a0, unsigned long a1,
                        unsigned long a2, unsigned long a3,
                        unsigned long a4, unsigned long a5)
{
    uiox_syscall_frame_t frame = {
        .nr = nr,
        .a0 = a0, .a1 = a1, .a2 = a2,
        .a3 = a3, .a4 = a4, .a5 = a5,
    };
    return (long)uiox_syscall_dispatch(&frame);
}
