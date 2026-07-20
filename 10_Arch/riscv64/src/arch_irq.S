/*
 * 10_Arch/riscv64/src/arch_irq.S
 * RISC-V supervisor-mode trap vector (synchronous + async).
 * stvec points here (Direct mode).
 */
    .text
    .align 2
    .option norvc

#define FRAME  (34*8)

.macro SAVE_ALL
    addi    sp, sp, -FRAME
    sd      x1,  1*8(sp)
    sd      x3,  3*8(sp)
    sd      x4,  4*8(sp)
    sd      x5,  5*8(sp)
    sd      x6,  6*8(sp)
    sd      x7,  7*8(sp)
    sd      x8,  8*8(sp)
    sd      x9,  9*8(sp)
/*
 * 10_Arch/riscv64/src/arch_irq.S  — continued
 */
    sd      x10, 10*8(sp)
    sd      x11, 11*8(sp)
    sd      x12, 12*8(sp)
    sd      x13, 13*8(sp)
    sd      x14, 14*8(sp)
    sd      x15, 15*8(sp)
    sd      x16, 16*8(sp)
    sd      x17, 17*8(sp)
    sd      x18, 18*8(sp)
    sd      x19, 19*8(sp)
    sd      x20, 20*8(sp)
    sd      x21, 21*8(sp)
    sd      x22, 22*8(sp)
    sd      x23, 23*8(sp)
    sd      x24, 24*8(sp)
    sd      x25, 25*8(sp)
    sd      x26, 26*8(sp)
    sd      x27, 27*8(sp)
    sd      x28, 28*8(sp)
    sd      x29, 29*8(sp)
    sd      x30, 30*8(sp)
    sd      x31, 31*8(sp)
    csrr    t0,  sepc
    sd      t0,  32*8(sp)
    csrr    t0,  sstatus
    sd      t0,  33*8(sp)
.endm

.macro RESTORE_ALL
    ld      t0,  33*8(sp)
    csrw    sstatus, t0
    ld      t0,  32*8(sp)
    csrw    sepc, t0
    ld      x1,  1*8(sp)
    ld      x3,  3*8(sp)
    ld      x4,  4*8(sp)
    ld      x5,  5*8(sp)
    ld      x6,  6*8(sp)
    ld      x7,  7*8(sp)
    ld      x8,  8*8(sp)
    ld      x9,  9*8(sp)
    ld      x10, 10*8(sp)
    ld      x11, 11*8(sp)
    ld      x12, 12*8(sp)
    ld      x13, 13*8(sp)
    ld      x14, 14*8(sp)
    ld      x15, 15*8(sp)
    ld      x16, 16*8(sp)
    ld      x17, 17*8(sp)
    ld      x18, 18*8(sp)
    ld      x19, 19*8(sp)
    ld      x20, 20*8(sp)
    ld      x21, 21*8(sp)
    ld      x22, 22*8(sp)
    ld      x23, 23*8(sp)
    ld      x24, 24*8(sp)
    ld      x25, 25*8(sp)
    ld      x26, 26*8(sp)
    ld      x27, 27*8(sp)
    ld      x28, 28*8(sp)
    ld      x29, 29*8(sp)
    ld      x30, 30*8(sp)
    ld      x31, 31*8(sp)
    addi    sp,  sp, FRAME
.endm

/* ── _vector_table — stvec points here (Direct mode) ───────────────── */
    .global _vector_table
    .align  2
_vector_table:
    SAVE_ALL

    /* scause: bit 63 = interrupt, bits[62:0] = cause code */
    csrr    a0, scause
    csrr    a1, stval
    mv      a2, sp          /* pass frame pointer to dispatcher */

    bltz    a0, .Linterrupt /* bit 63 set = async interrupt */

    /* ── Synchronous trap (exception / ECALL) ───────────────────── */
    li      t0, 8           /* scause=8 = ECALL from U-mode */
    beq     a0, t0, .Lsyscall
    li      t0, 9           /* scause=9 = ECALL from S-mode */
    beq     a0, t0, .Lsyscall

    /* Other exception: pass to C handler */
    call    riscv_exception_handler
    j       .Ltrap_done

.Lsyscall:
    /* RISC-V syscall ABI: a7=nr, a0-a5=args
     * Advance sepc past ECALL (sepc += 4) */
    ld      t0, 32*8(sp)
    addi    t0, t0, 4
    sd      t0, 32*8(sp)
    /* Load syscall args from frame */
    ld      a0, 17*8(sp)    /* a7 = syscall nr */
    ld      a1, 10*8(sp)    /* a0 */
    ld      a2, 11*8(sp)    /* a1 */
    ld      a3, 12*8(sp)    /* a2 */
    ld      a4, 13*8(sp)    /* a3 */
    ld      a5, 14*8(sp)    /* a4 */
    ld      a6, 15*8(sp)    /* a5 */
    call    arch_syscall_dispatch
    sd      a0, 10*8(sp)    /* write return value to a0 in frame */
    j       .Ltrap_done

.Linterrupt:
    /* Async interrupt: call C dispatcher */
    andi    a0, a0, 0x7F    /* strip bit 63 to get cause code */
    call    riscv_irq_dispatch
    /* fall through to done */

.Ltrap_done:
    RESTORE_ALL
    sret
