/*
 * 10_Arch/arm64/src/arch_context.S
 * ARMv8-A context save / restore for the UIOX scheduler.
 *
 * Called by 33_ProcessControlSubsystem when switching threads.
 *
 * void arch_context_switch(arch_context_t *old_ctx,   // x0
 *                           arch_context_t *new_ctx);  // x1
 */

    .text
    .align 3

/* ── arch_context_switch ─────────────────────────────────────────────
 * Save callee-saved registers into *old_ctx then load from *new_ctx.
 * x0 = old_ctx pointer (may be NULL for first switch from idle)
 * x1 = new_ctx pointer
 * ──────────────────────────────────────────────────────────────────── */
    .global arch_context_switch
    .type   arch_context_switch, @function
arch_context_switch:
    /* ── Save old context ─────────────────────────── */
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
    /* ── Load new context ─────────────────────────── */
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

    /* Return into the new thread — lr was loaded above */
    ret
    .size arch_context_switch, . - arch_context_switch


/* ── arch_context_init ───────────────────────────────────────────────
 * Initialise a fresh context for a new thread so that when
 * arch_context_switch loads it, execution starts at @entry(arg).
 *
 * void arch_context_init(arch_context_t *ctx,   // x0
 *                         void (*entry)(void*),  // x1
 *                         unsigned long sp_top,  // x2
 *                         void *arg);            // x3
 * ──────────────────────────────────────────────────────────────────── */
    .global arch_context_init
    .type   arch_context_init, @function
arch_context_init:
    /* Zero the whole context */
    mov     x4,  #0
    stp     x4,  x4,  [x0, #0]
    stp     x4,  x4,  [x0, #16]
    stp     x4,  x4,  [x0, #32]
    stp     x4,  x4,  [x0, #48]
    stp     x4,  x4,  [x0, #64]

    /* lr = entry  → when switch loads x30 and does ret, it jumps here */
    str     x1,       [x0, #88]     /* x30 (lr) = entry                */

    /* sp = stack_top aligned to 16 bytes */
    and     x2,  x2,  #~0xFUL
    str     x2,       [x0, #96]     /* sp                              */

    /* x19 = arg  → entry receives this as x0 via our trampoline        */
    str     x3,       [x0, #0]      /* x19 = arg (trampoline picks up) */

    /* SPSR_EL1: EL1h, DAIF = 0000 (all interrupts enabled) */
    mov     x4,  #0x05              /* PSTATE_EL1h                     */
    str     x4,       [x0, #104]

    ret
    .size arch_context_init, . - arch_context_init
