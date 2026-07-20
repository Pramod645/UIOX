/*
 * 10_Arch/riscv64/src/arch_context.S
 * RISC-V RV64 context save / restore.
 *
 * void arch_context_switch(arch_context_t *old_ctx,  // a0
 *                           arch_context_t *new_ctx); // a1
 */
    .text
    .align 2

    .global arch_context_switch
arch_context_switch:
    /* Save callee-saved regs into *old_ctx (a0) */
    beqz    a0, .Lload
    sd      s0,  0*8(a0)
    sd      s1,  1*8(a0)
    sd      s2,  2*8(a0)
    sd      s3,  3*8(a0)
    sd      s4,  4*8(a0)
    sd      s5,  5*8(a0)
    sd      s6,  6*8(a0)
    sd      s7,  7*8(a0)
    sd      s8,  8*8(a0)
    sd      s9,  9*8(a0)
    sd      s10, 10*8(a0)
    sd      s11, 11*8(a0)
    sd      ra,  12*8(a0)
    sd      sp,  13*8(a0)
    sd      tp,  14*8(a0)
    csrr    t0,  sstatus
    sd      t0,  15*8(a0)
    csrr    t0,  sepc
    sd      t0,  16*8(a0)

.Lload:
    /* Load callee-saved regs from *new_ctx (a1) */
    ld      s0,  0*8(a1)
    ld      s1,  1*8(a1)
    ld      s2,  2*8(a1)
    ld      s3,  3*8(a1)
    ld      s4,  4*8(a1)
    ld      s5,  5*8(a1)
    ld      s6,  6*8(a1)
    ld      s7,  7*8(a1)
    ld      s8,  8*8(a1)
    ld      s9,  9*8(a1)
    ld      s10, 10*8(a1)
    ld      s11, 11*8(a1)
    ld      ra,  12*8(a1)
    ld      sp,  13*8(a1)
    ld      tp,  14*8(a1)
    ld      t0,  15*8(a1)
    csrw    sstatus, t0
    ld      t0,  16*8(a1)
    csrw    sepc, t0
    ret


/* ── arch_context_init ──────────────────────────────────────────────
 * void arch_context_init(arch_context_t *ctx,  // a0
 *                         void (*entry)(void*),  // a1
 *                         unsigned long sp_top,  // a2
 *                         void *arg);            // a3
 * ──────────────────────────────────────────────────────────────────── */
    .global arch_context_init
arch_context_init:
    /* Zero the whole struct */
    li      t0, 17
    mv      t1, a0
1:  sd      zero, 0(t1)
    addi    t1, t1, 8
    addi    t0, t0, -1
    bnez    t0, 1b

    /* ra = entry (context_switch does ret → jumps to entry) */
    sd      a1, 12*8(a0)   /* ra = entry */

    /* sp = stack_top & ~0xF  (16-byte aligned) */
    andi    a2, a2, -16
    sd      a2, 13*8(a0)   /* sp */

    /* s0 = arg (trampoline passes this as a0 to entry) */
    sd      a3, 0*8(a0)    /* s0 = arg */

    /* sstatus = SPP=1 (S-mode), SPIE=1 (interrupts enabled on entry) */
    li      t0, (1 << 8) | (1 << 5)   /* SPP | SPIE */
    sd      t0, 15*8(a0)

    ret
