/*
 * 10_Arch/arm64/src/arch_context.S
 * ARMv8-A context save / restore.
 *
 * Two callers, and they need different things:
 *   33_PCS scheduler        kernel thread → kernel thread, EL1 → EL1
 *   scpcs_exec (SYS_EXECV)  kernel → userland, EL1 → EL0
 *
 * arch_context_t layout (offsets in bytes):
 *   0   x19   8   x20   16  x21   24  x22
 *   32  x23   40  x24   48  x25   56  x26
 *   64  x27   72  x28   80  x29   88  x30
 *   96  sp    104 spsr  112 elr   120 tpidr
 */

    .text
    .align 3

    .global arch_context_switch
    .type   arch_context_switch, @function
arch_context_switch:
    /* ── Save old context ─────────────────────────────────────── */
    cbz     x0, .Lload_new          /* NULL old_ctx = first switch     */

    stp     x19, x20, [x0, #0]
    stp     x21, x22, [x0, #16]
    stp     x23, x24, [x0, #32]
    stp     x25, x26, [x0, #48]
    stp     x27, x28, [x0, #64]
    stp     x29, x30, [x0, #80]     /* fp, lr                          */
    mov     x2,  sp
    str     x2,       [x0, #96]     /* sp                              */
    mrs     x2,  spsr_el1
    str     x2,       [x0, #104]    /* spsr                            */
    mrs     x2,  elr_el1
    str     x2,       [x0, #112]    /* elr                             */
    mrs     x2,  tpidr_el0
    str     x2,       [x0, #120]    /* tpidr                           */

.Lload_new:
    /* ── Load new context ─────────────────────────────────────── */
    ldp     x19, x20, [x1, #0]
    ldp     x21, x22, [x1, #16]
    ldp     x23, x24, [x1, #32]
    ldp     x25, x26, [x1, #48]
    ldp     x27, x28, [x1, #64]
    ldp     x29, x30, [x1, #80]     /* fp, lr                          */
    ldr     x2,       [x1, #96]
    mov     sp,  x2                  /* sp                              */
    ldr     x2,       [x1, #104]
    msr     spsr_el1, x2             /* spsr                            */
    ldr     x2,       [x1, #112]
    msr     elr_el1,  x2             /* elr                             */
    ldr     x2,       [x1, #120]
    msr     tpidr_el0,x2             /* tpidr                           */

    /* ── CHANGE 2 (was: ret) ──────────────────────────────────────
     * WHY: the four lines above write spsr_el1 and elr_el1, and `ret`
     * never reads either — it branches to x30, which was loaded from
     * offset 88.  So the mode field and the return address were pure
     * decoration: the "switch" was a plain jump that stayed at EL1.
     *
     * eret consumes BOTH: it takes the address from elr_el1 and the
     * target level (and DAIF state) from spsr_el1's M[3:0]/[9:6].
     * That is what makes a switch ABLE to change privilege level — and
     * it is the only instruction on AArch64 that can go EL1 → EL0.
     *
     * NOTE: this now depends on x30/offset 88 being irrelevant, and on
     * elr_el1 being set.  Both hold: the load above writes elr_el1, and
     * arch_context_init (below) writes offset 104 for the mode. */
    eret
    .size arch_context_switch, . - arch_context_switch


/* ── arch_context_init ─────────────────────────────────────────────── */
    .global arch_context_init
    .type   arch_context_init, @function
arch_context_init:
    mov     x4,  #0
    stp     x4,  x4,  [x0, #0]
    stp     x4,  x4,  [x0, #16]
    stp     x4,  x4,  [x0, #32]
    stp     x4,  x4,  [x0, #48]
    stp     x4,  x4,  [x0, #64]

    /* elr_el1 = entry.  With CHANGE 2 the return is eret, so the entry
     * address must be on elr_el1, not on x30.  Offset 88 is written too
     * so a `ret`-based caller still works — belt and braces. */
    str     x1,       [x0, #88]     /* x30 = entry                     */
    str     x1,       [x0, #112]    /* elr = entry  ← CHANGE 2 needs   */

    and     x2,  x2,  #~0xFUL
    str     x2,       [x0, #96]     /* sp                              */

    str     x3,       [x0, #0]      /* x19 = arg                       */

    /* ── CHANGE 1 (was: mov x4, #0x05 — EL1h) ─────────────────────
     * WHY: 0x05 is M[3:0]=0b0101 = EL1h, DAIF = 0000 (all interrupts
     * enabled).  That is correct for a KERNEL THREAD resuming into the
     * scheduler's own context, and wrong for a user entry: with CHANGE 2
     * now doing a real eret, spsr=EL1h would return the shell to EL1
     * with kernel privilege.  It would print a prompt, issue no trapping
     * svc, and be indistinguishable from a correct build.
     *
     * 0x3C0 = bits [9:6] = 0b1111 (D, A, I, F all masked on entry) with
     * M[3:0] = 0b0000 = EL0t.  User mode, interrupts off for the first
     * instructions.
     *
     * THIS IS THE ONE VALUE THAT DECIDES EL0 vs EL1.  Every other field
     * in this struct can be right and the shell still runs privileged if
     * this is left at 0x05. */
    mov     x4,  #0x3C0             /* PSTATE_EL0t, DAIF masked        */
    str     x4,       [x0, #104]    /* spsr                            */

    ret
    .size arch_context_init, . - arch_context_init
