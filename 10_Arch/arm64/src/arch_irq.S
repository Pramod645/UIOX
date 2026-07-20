/*
 * 10_Arch/arm64/src/arch_irq.S
 * ARMv8-A exception vector table and IRQ entry/exit stubs.
 *
 * The kernel installs this table via VBAR_EL1 in arch_init().
 * On every IRQ/exception the CPU jumps here, we save the full
 * register frame, call the C dispatcher, then restore and ERET.
 */

    .text

/* ── Register frame offsets ─────────────────────────────────────────── */
#define FRAME_SIZE    272   /* 34 × 8-byte slots                        */
#define OFF_X0        0
#define OFF_X30       240
#define OFF_SP_EL0    248
#define OFF_ELR       256
#define OFF_SPSR      264

/* ── Save full register frame ──────────────────────────────────────── */
.macro SAVE_REGS
    sub     sp,  sp,  #FRAME_SIZE
    stp     x0,  x1,  [sp, #16*0]
    stp     x2,  x3,  [sp, #16*1]
    stp     x4,  x5,  [sp, #16*2]
    stp     x6,  x7,  [sp, #16*3]
    stp     x8,  x9,  [sp, #16*4]
    stp     x10, x11, [sp, #16*5]
    stp     x12, x13, [sp, #16*6]
    stp     x14, x15, [sp, #16*7]
    stp     x16, x17, [sp, #16*8]
    stp     x18, x19, [sp, #16*9]
    stp     x20, x21, [sp, #16*10]
    stp     x22, x23, [sp, #16*11]
    stp     x24, x25, [sp, #16*12]
    stp     x26, x27, [sp, #16*13]
    stp     x28, x29, [sp, #16*14]
    str     x30,      [sp, #OFF_X30]
    mrs     x0,  sp_el0
    str     x0,       [sp, #OFF_SP_EL0]
    mrs     x0,  elr_el1
    str     x0,       [sp, #OFF_ELR]
    mrs     x0,  spsr_el1
    str     x0,       [sp, #OFF_SPSR]
.endm

/* ── Restore full register frame ───────────────────────────────────── */
.macro RESTORE_REGS
    ldr     x0,       [sp, #OFF_SPSR]
    msr     spsr_el1, x0
    ldr     x0,       [sp, #OFF_ELR]
    msr     elr_el1,  x0
    ldr     x0,       [sp, #OFF_SP_EL0]
    msr     sp_el0,   x0
    ldr     x30,      [sp, #OFF_X30]
    ldp     x28, x29, [sp, #16*14]
    ldp     x26, x27, [sp, #16*13]
    ldp     x24, x25, [sp, #16*12]
    ldp     x22, x23, [sp, #16*11]
    ldp     x20, x21, [sp, #16*10]
    ldp     x18, x19, [sp, #16*9]
    ldp     x16, x17, [sp, #16*8]
    ldp     x14, x15, [sp, #16*7]
    ldp     x12, x13, [sp, #16*6]
    ldp     x10, x11, [sp, #16*5]
    ldp     x8,  x9,  [sp, #16*4]
    ldp     x6,  x7,  [sp, #16*3]
    ldp     x4,  x5,  [sp, #16*2]
    ldp     x2,  x3,  [sp, #16*1]
    ldp     x0,  x1,  [sp, #16*0]
    add     sp,  sp,  #FRAME_SIZE
.endm

/* ── Exception vector entry stubs ──────────────────────────────────── */
    .macro VENTRY label
    .align 7                        /* each vector slot is 128 bytes   */
\label:
    SAVE_REGS
    bl      \label\()_handler
    RESTORE_REGS
    eret
    .endm

/* ── IRQ handler stub ───────────────────────────────────────────────── */
/* Called from the vector table; reads IAR, calls C dispatcher, EOI.   */
    .global arm64_irq_entry
    .align  7
arm64_irq_entry:
    SAVE_REGS

    /* Read Interrupt Acknowledge Register → IRQ number in x0 */
    ldr     x1,  =0x08010000        /* GIC_CPU_IAR                     */
    ldr     w0,  [x1]
    and     x19, x0, #0x3FFU        /* save IRQ number                 */

    /* Call C dispatcher: void arch_irq_dispatch(unsigned int irq) */
    mov     x0,  x19
    bl      arch_irq_dispatch

    /* Write End-of-Interrupt register */
    ldr     x1,  =0x08010010        /* GIC_CPU_EOIR                    */
    str     w19, [x1]

    RESTORE_REGS
    eret

/* ── SVC (system call) entry ────────────────────────────────────────── */
    .global arm64_svc_entry
    .align  7
arm64_svc_entry:
    SAVE_REGS

    /* ARM64 syscall ABI: x8=nr, x0-x5=args */
    mov     x6,  x5
    mov     x5,  x4
    mov     x4,  x3
    mov     x3,  x2
    mov     x2,  x1
    mov     x1,  x0
    mov     x0,  x8    /* syscall number */
    bl      arch_syscall_dispatch
    str     x0,  [sp, #OFF_X0]   /* write return value back to frame  */

    RESTORE_REGS
    eret

/* ── Exception vector table (VBAR_EL1 points here) ─────────────────── */
    .global _vector_table
    .align  11                      /* 2KB aligned                     */
_vector_table:
    /* Current EL with SP0 */
    .align 7; b arm64_irq_entry     /* Synchronous  */
    .align 7; b arm64_irq_entry     /* IRQ          */
    .align 7; b arm64_irq_entry     /* FIQ          */
    .align 7; b arm64_irq_entry     /* SError       */
    /* Current EL with SPx */
    .align 7; b arm64_svc_entry     /* Synchronous (SVC) */
    .align 7; b arm64_irq_entry     /* IRQ          */
    .align 7; b arm64_irq_entry     /* FIQ          */
    .align 7; b arm64_irq_entry     /* SError       */
    /* Lower EL AArch64 */
    .align 7; b arm64_svc_entry     /* Synchronous (SVC from EL0) */
    .align 7; b arm64_irq_entry     /* IRQ          */
    .align 7; b arm64_irq_entry     /* FIQ          */
    .align 7; b arm64_irq_entry     /* SError       */
    /* Lower EL AArch32 */
    .align 7; b arm64_irq_entry
    .align 7; b arm64_irq_entry
    .align 7; b arm64_irq_entry
    .align 7; b arm64_irq_entry
