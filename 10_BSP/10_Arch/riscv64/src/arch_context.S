/*
 * 10_Arch/riscv64/src/arch_context.S
 * RISC-V RV64 context save / restore.
 *
 * arch_context_t layout (offsets in bytes, 8 each):
 *   0..88   s0..s11     96  ra    104 sp
 *   112 tp  120 sstatus 128 sepc
 */
    .text
    .align 2

    .global arch_context_switch
arch_context_switch:
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

    /* ── CHANGE 2 (was: ret) ──────────────────────────────────────
     * WHY: `csrw sepc` and `csrw sstatus` above write the trap-return
     * registers, and `ret` (jalr x0, 0(ra)) never reads either — it
     * jumps to ra, which was loaded from offset 96.  So both writes were
     * inert and the switch stayed in S-mode.
     *
     * sret takes the return address from sepc and the target privilege
     * from sstatus.SPP (bit 8): SPP=0 → U-mode, SPP=1 → S-mode.  It also
     * restores sstatus.SPIE into the interrupt-enable bit.  That is the
     * instruction that makes a switch able to drop to U-mode.
     *
     * NOTE: ra/offset 96 is now irrelevant on this path, and sepc must
     * be set — arch_context_init (below) writes offset 128 for it. */
    sret


/* ── arch_context_init ────────────────────────────────────────────── */
    .global arch_context_init
arch_context_init:
    li      t0, 17
    mv      t1, a0
1:  sd      zero, 0(t1)
    addi    t1, t1, 8
    addi    t0, t0, -1
    bnez    t0, 1b

    /* ra = entry, kept for a `ret`-based caller.
     * sepc = entry — CHANGE 2's sret reads sepc, not ra. */
    sd      a1, 12*8(a0)   /* ra = entry                              */
    sd      a1, 16*8(a0)   /* sepc = entry  ← CHANGE 2 needs          */

    andi    a2, a2, -16
    sd      a2, 13*8(a0)   /* sp                                      */

    sd      a3, 0*8(a0)    /* s0 = arg                                */

    /* ── CHANGE 1 (was: SPP=1, S-mode) ────────────────────────────
     * WHY: bit 8 (SPP) = 1 means "return to S-mode".  With CHANGE 2 now
     * doing a real sret, that would drop the shell back into S-mode —
     * supervisor privilege, so its ecall would be indistinguishable from
     * a kernel-internal call and the trap path would never fire.
     *
     * SPP = 0 → U-mode.  SPIE (bit 5) = 1 → interrupts enabled on entry,
     * which is what a user program wants (it has no way to unmask them
     * itself until it runs). */
    li      t0, (0 << 8) | (1 << 5)  /* SPP=0 U-mode | SPIE=1           */
    sd      t0, 15*8(a0)             /* sstatus                         */

    ret
