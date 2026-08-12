/*
 * syscall_stub.S — x86-64 SYSCALL entry stub
 *
 * On SYSCALL entry:
 *   rcx = saved RIP (return address)
 *   r11 = saved RFLAGS
 *   rsp = still user stack  ← must swap to kernel stack immediately
 *   rax = syscall number
 *   rdi,rsi,rdx,r10,r8,r9 = args 0-5
 */

    .text
    .global syscall_entry_stub
    .align  16
syscall_entry_stub:

    /* ── 1. Swap to kernel stack ───────────────────────────────── */
    /* Use per-CPU kernel stack pointer stored in scratch MSR or    */
    /* a fixed kernel stack for now (single-core boot stage).       */
    /* In a full SMP kernel: swapgs + load from gs:kernel_sp        */
    movq    %rsp, %gs:8         /* save user rsp in per-CPU scratch  */
    movq    %gs:0, %rsp         /* load kernel rsp from per-CPU data */

    /* ── 2. Save callee-saved + syscall-clobbered registers ───── */
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    pushq   %rcx                /* saved RIP (return address)        */
    pushq   %r11                /* saved RFLAGS                      */
    pushq   %gs:8               /* saved user rsp                    */

    /*
     * ── 3. Rearrange args for C calling convention ──────────────
     *
     * C signature: arch_syscall_entry(nr, a0, a1, a2, a3, a4, a5)
     * SysV ABI:    rdi,  rsi, rdx, rcx,  r8,  r9, stack
     *
     * On entry:
     *   rax = nr    → move to rdi (1st C arg)
     *   rdi = a0    → move to rsi (2nd C arg)
     *   rsi = a1    → move to rdx (3rd C arg)
     *   rdx = a2    → move to rcx (4th C arg)
     *   r10 = a3    → move to r8  (5th C arg) [r10 ≠ rcx for syscalls]
     *   r8  = a4    → move to r9  (6th C arg)
     *   r9  = a5    → push on stack (7th C arg)
     */
    pushq   %r9                 /* a5 → 7th arg on stack             */
    movq    %r8,  %r9           /* a4 → r9 (6th arg)                 */
    movq    %r10, %r8           /* a3 → r8 (5th arg)                 */
    movq    %rdx, %rcx          /* a2 → rcx (4th arg)                */
    movq    %rsi, %rdx          /* a1 → rdx (3rd arg)                */
    movq    %rdi, %rsi          /* a0 → rsi (2nd arg)                */
    movq    %rax, %rdi          /* nr → rdi (1st arg)                */

    /* ── 4. Call C handler ─────────────────────────────────────── */
    call    arch_syscall_entry  /* returns result in rax             */

    /* ── 5. Clean up stack arg ─────────────────────────────────── */
    addq    $8, %rsp            /* pop a5 pushed in step 3           */

    /* ── 6. Restore registers ──────────────────────────────────── */
    popq    %gs:8               /* discard saved user rsp slot       */
    popq    %r11                /* restore RFLAGS for SYSRETQ        */
    popq    %rcx                /* restore RIP for SYSRETQ           */
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp

    /* ── 7. Restore user stack and return to Ring 3 ────────────── */
    movq    %gs:8, %rsp         /* restore user rsp                  */
    sysretq                     /* rcx→rip, r11→rflags, Ring3        */
