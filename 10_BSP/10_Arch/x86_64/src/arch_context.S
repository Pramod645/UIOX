/*
 * 10_Arch/x86_64/src/arch_context.S
 * x86_64 context save / restore for the UIOX scheduler.
 *
 * void arch_context_switch(arch_context_t *old_ctx,  // rdi
 *                           arch_context_t *new_ctx); // rsi
 *
 * arch_context_t layout (offsets in bytes):
 *   0   rbx    8   rbp    16  r12    24  r13
 *   32  r14    40  r15    48  rsp    56  rip
 *   64  rflags 72  fs_base
 */

    .text
    .align 4

    .global arch_context_switch
    .type   arch_context_switch, @function
arch_context_switch:
    /* ── Save old context (rdi) ───────────────────────────────── */
    test    %rdi, %rdi
    jz      .Lload_new              /* NULL old_ctx = first switch     */

    mov     %rbx,  0(%rdi)
    mov     %rbp,  8(%rdi)
    mov     %r12, 16(%rdi)
    mov     %r13, 24(%rdi)
    mov     %r14, 32(%rdi)
    mov     %r15, 40(%rdi)
    mov     %rsp, 48(%rdi)          /* rsp                             */
    /* rip = return address sitting on the stack */
    mov     (%rsp), %rax
    mov     %rax,  56(%rdi)         /* rip                             */
    /* rflags */
    pushfq
    pop     %rax
    mov     %rax,  64(%rdi)
    /* FS.base (thread-local storage) */
    mov     $0xC0000100, %ecx       /* MSR_FS_BASE                     */
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
    mov     48(%rsi), %rsp          /* restore stack pointer           */
    /* Restore FS.base */
    mov     72(%rsi), %rax
    mov     %eax, %eax              /* zero-extend low 32             */
    mov     %rax, %rdx
    shr     $32, %rdx
    mov     $0xC0000100, %ecx
    wrmsr
    /* Restore rflags */
    push    64(%rsi)
    popfq
    /* Jump to saved rip */
    jmp     *56(%rsi)
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
    /* Zero the whole struct (10 × 8 = 80 bytes) */
    xor     %eax, %eax
    mov     $10,  %ecx
    mov     %rdi, %r8
1:  mov     %rax, (%r8)
    add     $8,   %r8
    dec     %ecx
    jnz     1b

    /* rip = entry — when switch does jmp *rip it enters here          */
    mov     %rsi, 56(%rdi)

    /* rsp = stack_top aligned to 16 bytes                             */
    and     $-16, %rdx
    mov     %rdx, 48(%rdi)

    /* rbx = arg — trampoline passes this as rdi to entry              */
    mov     %rcx, 0(%rdi)           /* rbx = arg                       */

    /* rflags: IF=1 (interrupts enabled), no other special bits        */
    mov     $0x202, %rax            /* RFLAGS: IF=1                    */
    mov     %rax, 64(%rdi)

    ret
    .size arch_context_init, . - arch_context_init
