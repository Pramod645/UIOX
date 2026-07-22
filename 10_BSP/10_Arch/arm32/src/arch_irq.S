/*
 * 10_Arch/arm32/src/arch_irq.S
 * ARMv7-A exception vector table and IRQ entry/exit stubs.
 *
 * The kernel installs the vector table via arch_init() (VBAR CP15).
 * On every IRQ the CPU jumps here, saves the full context,
 * calls the C dispatcher, restores, and returns.
 */

    .text
    .arch   armv7-a
    .syntax unified

/* ── Frame size: r0-r12, lr, spsr = 15 × 4 = 60 bytes ──────────────── */
#define FRAME_SIZE  60

.macro SAVE_REGS
    /* In IRQ mode: lr_irq = PC+4 of interrupted instruction            */
    sub     lr,  lr, #4             /* adjust lr → interrupted PC      */
    srsdb   sp!, #0x13              /* push lr + spsr onto SVC stack   */
    cpsid   i,   #0x13             /* switch to SVC mode, mask IRQ    */
    push    {r0-r12, lr}            /* save all GPRs + lr_svc          */
.endm

.macro RESTORE_REGS
    pop     {r0-r12, lr}            /* restore GPRs + lr               */
    rfeia   sp!                     /* restore PC and CPSR from srsdb  */
.endm

/* ── IRQ handler stub ────────────────────────────────────────────────── */
    .global arm32_irq_entry
arm32_irq_entry:
    SAVE_REGS

    /* Read GIC CPU IAR to get IRQ number */
    ldr     r4,  =0x0801000C        /* GIC_CPU_IAR                     */
    ldr     r0,  [r4]
    /*and     r5,  r0, #0x3FF */        /* save IRQ number, strip source   */
    movw    r6,  #0x3FF             /* load 10-bit mask into r6        */
    and     r5,  r0, r6             /* save IRQ number, strip source   */

    /* Call C dispatcher */
    mov     r0,  r5
    bl      arch_irq_dispatch

    /* Write GIC CPU EOIR */
    ldr     r4,  =0x08010010        /* GIC_CPU_EOIR                    */
    str     r5,  [r4]

    RESTORE_REGS

/* ── SVC (system call) entry ─────────────────────────────────────────── */
    .global arm32_svc_entry
arm32_svc_entry:
    /* SVC mode: lr_svc = PC of next instruction after SVC              */
    srsdb   sp!, #0x13              /* push lr + spsr → SVC stack      */
    cpsid   i,   #0x13
    push    {r0-r12, lr}

    /* AAPCS32 syscall: r7=nr, r0-r5=args */
    mov     r6,  r5
    mov     r5,  r4
    mov     r4,  r3
    mov     r3,  r2
    mov     r2,  r1
    mov     r1,  r0
    mov     r0,  r7                 /* syscall number from r7          */
    bl      arch_syscall_dispatch
    str     r0,  [sp, #0]          /* write return value to r0 slot   */

    pop     {r0-r12, lr}
    rfeia   sp!                     /* return to user, restore CPSR    */

/* ── Exception vector table (VBAR points here) ───────────────────────── */
    .global _vector_table
    .align  5                       /* 32-byte aligned                 */
_vector_table:
    ldr     pc, =arm32_reset_handler    /* 0x00 Reset                  */
    ldr     pc, =arm32_undef_handler    /* 0x04 Undefined instruction  */
    ldr     pc, =arm32_svc_entry        /* 0x08 SVC                    */
    ldr     pc, =arm32_prefetch_handler /* 0x0C Prefetch abort         */
    ldr     pc, =arm32_data_handler     /* 0x10 Data abort             */
    nop                                 /* 0x14 Reserved               */
    ldr     pc, =arm32_irq_entry        /* 0x18 IRQ                    */
    ldr     pc, =arm32_fiq_entry        /* 0x1C FIQ                    */

/* ── Minimal exception stubs ─────────────────────────────────────────── */
    .global arm32_reset_handler
arm32_reset_handler:
    b       arm32_reset_handler     /* spin — kernel handles reset     */

    .global arm32_undef_handler
arm32_undef_handler:
    srsdb   sp!, #0x13
    cpsid   i,   #0x13
    push    {r0-r12, lr}
    mov     r0,  #1                 /* cause = undefined instruction   */
    bl      arm32_exception_dispatch
    pop     {r0-r12, lr}
    rfeia   sp!

    .global arm32_prefetch_handler
arm32_prefetch_handler:
    sub     lr,  lr, #4
    srsdb   sp!, #0x13
    cpsid   i,   #0x13
    push    {r0-r12, lr}
    mov     r0,  #3                 /* cause = prefetch abort          */
    bl      arm32_exception_dispatch
    pop     {r0-r12, lr}
    rfeia   sp!

    .global arm32_data_handler
arm32_data_handler:
    sub     lr,  lr, #8
    srsdb   sp!, #0x13
    cpsid   i,   #0x13
    push    {r0-r12, lr}
    mov     r0,  #4                 /* cause = data abort              */
    bl      arm32_exception_dispatch
    pop     {r0-r12, lr}
    rfeia   sp!

    .global arm32_fiq_entry
arm32_fiq_entry:
    sub     lr,  lr, #4
    srsdb   sp!, #0x11             /* FIQ mode                        */
    cpsid   if,  #0x13
    push    {r0-r12, lr}
    mov     r0,  #7                 /* cause = FIQ                     */
    bl      arm32_exception_dispatch
    pop     {r0-r12, lr}
    rfeia   sp!

    
