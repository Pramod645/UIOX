/*
 * 10_Arch/x86_64/src/arch_context.S
 * x86_64 context save / restore.
 *
 * arch_context_t layout (offsets in bytes):
 *   0   rbx    8   rbp    16  r12    24  r13
 *   32  r14    40  r15    48  rsp    56  rip
 *   64  rflags 72  fs_base 80  cs     88  ss
 */

    .text
    .align 4

    .global arch_context_switch
    .type   arch_context_switch, @function
arch_context_switch:
    /* ── Save old context (rdi) ───────────────────────────────── */
    test    %rdi, %rdi
    jz      .Lload_new

    mov     %rbx,  0(%rdi)
    mov     %rbp,  8(%rdi)
    mov     %r12, 16(%rdi)
    mov     %r13, 24(%rdi)
    mov     %r14, 32(%rdi)
    mov     %r15, 40(%rdi)
    mov     %rsp, 48(%rdi)
    mov     (%rsp), %rax
    mov     %rax,  56(%rdi)
    pushfq
    pop     %rax
    mov     %rax,  64(%rdi)
    mov     $0xC0000100, %ecx
    rdmsr
    shl     $32, %rdx
    or      %rdx, %rax
    mov     %rax, 72(%rdi)

.Lload_new:
    /* ── Load new context (rsi) ───────────────────────────────── */
    mov      0(%rsi), %rbx
    mov      8(%rsi), %rbp
    mov     16(%rsi), %r12
    mov     24(%rsi), %r13
    mov     32(%rsi), %r14
    mov     40(%rsi), %r15
    mov     48(%rsi), %rsp

    mov     72(%rsi), %rax
    mov     %eax, %eax
    mov     %rax, %rdx
    shr     $32, %rdx
    mov     $0xC0000100, %ecx
    wrmsr

    /* ── CHANGE 2 (was: push 64(%rsi) / popfq / jmp *56(%rsi)) ────
     * WHY: `jmp *56(%rsi)` is an ordinary near jump — it loads the target
     * RIP and nothing else.  RFLAGS was restored by popfq, but RFLAGS
     * carries no privilege information on x86_64: the current level is
     * CS.RPL, and a near jump never touches CS.  So the switch could
     * only ever stay at ring 0.
     *
     * iretq pops RIP, CS, RFLAGS, RSP, SS in that order and loads them
     * together — it is the only instruction that can change CPL.  The
     * stack must therefore hold a full 5-quadword frame at the moment
     * iretq runs, and CS must be a ring-3 selector for user entry.
     *
     * NOTE: this CONSUMES the stack.  `mov 48(%rsi), %rsp` above sets
     * rsp to the USER stack, and the five pushes build the frame there —
     * so SS:RSP in the frame names the same stack the pushes landed on.
     * The shell begins with a stack whose top 40 bytes the kernel just
     * wrote.  Moving the pushes onto the kernel stack is a later
     * refinement, not a correctness fix. */
    push    88(%rsi)               /* SS                              */
    push    48(%rsi)               /* RSP                             */
    push    64(%rsi)               /* RFLAGS                          */
    push    80(%rsi)               /* CS                              */
    push    56(%rsi)               /* RIP                             */
    iretq
    .size arch_context_switch, . - arch_context_switch


/*
 * void arch_context_init(arch_context_t *ctx,   // rdi
 *                         void (*entry)(void*),  // rsi
 *                         unsigned long sp_top,  // rdx
 *                         void *arg);            // rcx
 */
    .global arch_context_init
    .type   arch_context_init, @function
arch_context_init:
    /* Zero the struct (12 × 8 = 96 bytes: 0..72, then cs 80 and ss 88,
     * which CHANGE 1 writes below) */
    xor     %eax, %eax
    mov     $12,  %ecx
    mov     %rdi, %r8
1:  mov     %rax, (%r8)
    add     $8,   %r8
    dec     %ecx
    jnz     1b

    mov     %rsi, 56(%rdi)         /* rip = entry                     */

    and     $-16, %rdx
    mov     %rdx, 48(%rdi)         /* rsp = stack_top                 */

    mov     %rcx, 0(%rdi)          /* rbx = arg                       */

    mov     $0x202, %rax
    mov     %rax, 64(%rdi)         /* rflags: IF=1                    */

    /* ── CHANGE 1: the ring-3 selectors ──────────────────────────
     * On AArch64 and RISC-V the target privilege lives in a field this
     * routine already writes (spsr_el1 / sstatus).  x86_64 has no such
     * field: RFLAGS carries no CPL, and the level is named by two
     * SEPARATE segment registers that iretq pops from the stack.  So
     * CHANGE 2 cannot work without them, and this routine is where they
     * must come from.
     *
     * The values are this TREE's, from arch_defs.h:89-92:
     *
     *     SEG_KERN_CODE  0x08      SEG_KERN_DATA  0x10
     *     SEG_USER_CODE  0x18      SEG_USER_DATA  0x20
     *
     * Those are GDT INDICES with RPL 0.  iretq checks RPL == 3 on the
     * CS and SS it pops, so the runtime selector is the index OR'd
     * with 3:
     *
     *     SEG_USER_CODE | 3  =  0x18 | 3  =  0x1B   (cs)
     *     SEG_USER_DATA | 3  =  0x20 | 3  =  0x23   (ss)
     *
     * Writing SEG_USER_CODE directly (0x18) is a general-protection
     * fault, not a subtle bug — the CPU rejects a ring-3 return whose
     * CS.RPL is 0.
     *
     * RFLAGS keeps IF=1 (0x202): a user program cannot unmask
     * interrupts itself, so they must arrive unmasked. */
    mov     $0x1B, %rax            /* cs = SEG_USER_CODE | 3  (0x18|3) */
    mov     %rax, 80(%rdi)              /* cs = ring-3 code selector  */
    mov     $0x23, %rax            /* ss = SEG_USER_DATA | 3  (0x20|3) */
    mov     %rax, 88(%rdi)              /* ss = ring-3 stack selector */

    ret
    .size arch_context_init, . - arch_context_init
