/*
 * 10_Arch/arm32/src/arch_context.S
 * ARMv7-A context save / restore.
 *
 * arch_context_t layout (offsets in bytes):
 *   0   r4    4   r5    8   r6    12  r7
 *   16  r8    20  r9    24  r10   28  r11
 *   32  lr    36  sp    40  cpsr  44  pc
 *
 * ── arm32 IS NOT LIKE THE OTHER THREE ───────────────────────────────
 * This is the only arch that applies the mode bits MID-FUNCTION:
 *
 *     msr  cpsr_c, r2      ← line 39, inside arch_context_switch
 *     bx   lr              ← line 42, AFTER the mode change
 *
 * On the other arches the mode field sits in a trap-return register
 * (spsr_el1 / sstatus.SPP / nothing) and is only consumed by the return
 * instruction.  Here `cpsr_c` takes effect immediately, so writing a
 * USER mode value means the very next instruction — the bx — executes
 * with user privilege, reading from a register-file the mode change may
 * have just remapped.
 *
 * If CPSR_M=User is set at line 39 and the bx at 42 is meant to jump to
 * the entry address, the sequence is a jump from user mode into
 * arbitrary kernel code.  So ARM32 NEEDS A DIFFERENT SHAPE, not a
 * constant swap: either
 *
 *   (a) keep msr cpsr_c as-is for the kernel path, and for the user
 *       path set cpsr from a saved frame and use `movs pc, lr` — which
 *       restores CPSR from SPSR and branches in one instruction, the
 *       ARM equivalent of eret; the mode change then happens ATOMICALLY
 *       with the branch, exactly as on the other arches; or
 *   (b) keep two switches.
 *
 * Do not just change 0x13 to 0x10.  It compiles and it is wrong.
 */

    .text
    .arch   armv7-a
    .syntax unified
    .thumb

    .align 2
    .global arch_context_switch
    .thumb_func
arch_context_switch:
    cmp     r0, #0
    beq     .Lload_new

    stm     r0!, {r4-r11, lr}
    str     sp,  [r0, #0]
    mrs     r2,  cpsr
    str     r2,  [r0, #4]
    str     lr,  [r0, #8]

.Lload_new:
    ldm     r1!, {r4-r11, lr}
    ldr     sp,  [r1, #0]

    /* ── CHANGE 2 (was: ldr r2,[r1,#4] / msr cpsr_c,r2 / bx lr) ───
     * WHY: the original restored cpsr_c and THEN branched with bx —
     * two steps, with the mode change taking effect between them.  That
     * is safe only when the target mode equals the current one (which is
     * the kernel-thread case).  For a user entry it is not safe: after
     * msr cpsr_c, the bx would execute in User mode.
     *
     * `movs pc, lr` is RISC-V's sret / AArch64's eret equivalent on
     * ARM: it writes CPSR from SPSR (restoring the mode and flags) and
     * branches to lr in ONE instruction, so the level change and the
     * jump are atomic.  Nothing executes in the new mode before control
     * arrives at the entry point.
     *
     * NOTE: this uses SPSR, so the mode value must be in SPSR at this
     * point — and SPSR is what `msr cpsr_c` did NOT write (cpsr_c writes
     * CPSR itself).  The saved-context field at offset 40 (cpsr) must
     * therefore be loaded into SPSR, or restored inside an exception
     * frame.  See the arch note above: arm32 needs the (a) shape, not a
     * constant swap. */
    ldr     r2,  [r1, #4]          /* cpsr                            */
    msr     spsr, r2               /* → SPSR, not CPSR                */
    ldr     r2,  [r1, #8]          /* pc                              */
    mov     lr,  r2
    movs    pc,  lr                /* return + restore mode atomically */

/*
 * void arch_context_init(...)
 */
    .align 2
    .global arch_context_init
    .thumb_func
arch_context_init:
    push    {r4-r7}
    mov     r4,  #0
    mov     r5,  #0
    mov     r6,  #0
    mov     r7,  #0
    stm     r0,  {r4-r7}
    add     r12, r0, #16
    stm     r12, {r4-r7}
    pop     {r4-r7}

    str     r1,  [r0, #32]         /* lr                              */

    bic     r2,  r2,  #7
    str     r2,  [r0, #36]         /* sp                              */

    /* ── CHANGE 1 (was: 0x13 SVC mode | 0x20 Thumb) ─────────────────
     * WHY: 0x13 = CPSR_M=0b10011 = SVC mode (supervisor).  Correct for a
     * kernel thread; with CHANGE 2 now doing a real mode-restoring
     * return, it would put the shell in SVC mode — privileged, ecall
     * indistinguishable from an internal call, trap path silent.
     *
     * 0x10 = CPSR_M=0b10000 = User.  T (0x20) is KEPT: the whole file
     * is assembled Thumb-2 (`.thumb` at the top), so the entry point is
     * Thumb code and the T bit must be set or the first fetch
     * misdecodes.  Dropping 0x20 along with 0x13 would be a second bug
     * wearing the first one's clothes. */
    mov     r12, #0x10             /* User mode                       */
    orr     r12, r12, #0x20        /* Thumb state — KEEP THIS         */
    str     r12, [r0, #40]         /* cpsr                            */

    str     r3,  [r0, #0]          /* r4 = arg                        */

    str     r1,  [r0, #44]         /* pc                              */

    bx      lr
