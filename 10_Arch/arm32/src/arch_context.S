/*
 * 10_Arch/arm32/src/arch_context.S
 * ARMv7-A context save / restore for the UIOX scheduler.
 *
 * void arch_context_switch(arch_context_t *old_ctx,  @ r0
 *                           arch_context_t *new_ctx); @ r1
 *
 * arch_context_t layout (offsets in bytes):
 *   0   r4    4   r5    8   r6    12  r7
 *   16  r8    20  r9    24  r10   28  r11
 *   32  lr    36  sp    40  cpsr  44  pc
 */

    .text
    .arch   armv7-a
    .syntax unified
    .thumb                     /* Thumb-2 throughout */

    .align 2
    .global arch_context_switch
    .thumb_func
arch_context_switch:
    /* ── Save old context ─────────────────────────────────────── */
    cmp     r0, #0
    beq     .Lload_new              /* NULL old_ctx = first switch    */

    stm     r0!, {r4-r11, lr}      /* r4-r11, lr → offsets 0-32       */
    str     sp,  [r0, #0]          /* sp → offset 36                  */
    mrs     r2,  cpsr
    str     r2,  [r0, #4]          /* cpsr → offset 40                */
    /* pc is "where we return to" = current lr (already saved above)   */
    str     lr,  [r0, #8]          /* pc → offset 44                  */

.Lload_new:
    /* ── Load new context ─────────────────────────────────────── */
    ldm     r1!, {r4-r11, lr}      /* r4-r11, lr                      */
    ldr     sp,  [r1, #0]          /* sp                              */
    ldr     r2,  [r1, #4]          /* cpsr                            */
    msr     cpsr_c, r2             /* restore mode / interrupt bits   */
    ldr     r2,  [r1, #8]          /* pc                              */
    /* Return into the new thread — jump to saved pc                   */
    bx      lr                     /* lr was loaded from old context  */

/*
 * void arch_context_init(arch_context_t *ctx,  @ r0
 *                         void (*entry)(void*), @ r1
 *                         unsigned int sp_top,  @ r2
 *                         void *arg);           @ r3
 */
    .align 2
    .global arch_context_init
    .thumb_func
arch_context_init:
    /* Zero the whole struct (48 bytes = 12 words) */
    mov     r12, #0
    stm     r0,  {r12}
    add     r12, r0,  #4
    stm     r12, {r12}             /* lazy zero — loop below is cleaner */
    push    {r4-r7}
    mov     r4,  #0
    mov     r5,  #0
    mov     r6,  #0
    mov     r7,  #0
    stm     r0,  {r4-r7}           /* r4-r7 = 0  (offsets 0-12)       */
    add     r12, r0, #16
    stm     r12, {r4-r7}           /* r8-r11 = 0 (offsets 16-28)      */
    pop     {r4-r7}

    /* lr = entry (when switch does bx lr it jumps to entry)           */
    str     r1,  [r0, #32]         /* lr                              */

    /* sp = stack_top & ~7  (8-byte AAPCS alignment)                   */
    bic     r2,  r2,  #7
    str     r2,  [r0, #36]         /* sp                              */

    /* cpsr: SVC mode, Thumb, IRQ enabled (CPSR_MODE_SVC=0x13, T=0x20) */
    mov     r12, #0x13             /* SVC mode                        */
    orr     r12, r12, #0x20        /* Thumb state                     */
    str     r12, [r0, #40]         /* cpsr                            */

    /* r4 = arg — entry receives this as r0 via our trampoline          */
    str     r3,  [r0, #0]          /* r4 = arg                        */

    /* pc = entry */
    str     r1,  [r0, #44]         /* pc                              */

    bx      lr
