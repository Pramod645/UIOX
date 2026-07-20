/*
 * 10_Arch/x86_64/src/arch_irq.S
 * x86_64 IDT exception/IRQ entry stubs and vector table.
 *
 * The kernel installs the IDT in arch_init() (lidt).
 * Each vector saves the full register frame, calls a C dispatcher,
 * restores, and returns with IRETQ.
 */

    .text
    .align 4

/* ── Register frame layout (pushed onto stack) ───────────────────────
 * Low address (top of frame after sub rsp):
 *   rax rcx rdx rbx rbp rsi rdi r8 r9 r10 r11 r12 r13 r14 r15
 *   vector  error_code
 *   rip cs rflags rsp ss   ← hardware-pushed
 * ──────────────────────────────────────────────────────────────────── */
#define FRAME_GPR   120   /* 15 registers × 8                          */

.macro SAVE_REGS
    push    %r15
    push    %r14
    push    %r13
    push    %r12
    push    %r11
    push    %r10
    push    %r9
    push    %r8
    push    %rdi
    push    %rsi
    push    %rbp
    push    %rbx
    push    %rdx
    push    %rcx
    push    %rax
.endm

.macro RESTORE_REGS
    pop     %rax
    pop     %rcx
    pop     %rdx
    pop     %rbx
    pop     %rbp
    pop     %rsi
    pop     %rdi
    pop     %r8
    pop     %r9
    pop     %r10
    pop     %r11
    pop     %r12
    pop     %r13
    pop     %r14
    pop     %r15
.endm

/* ── Stubs without error code ────────────────────────────────────────── */
.macro ISR_NOERR num
    .global isr\num
    .align  4
    isr\num:
        push    $0              /* dummy error code                    */
        push    $\num           /* vector number                       */
        jmp     isr_common
.endm

/* ── Stubs with CPU-pushed error code ────────────────────────────────── */
.macro ISR_ERR num
    .global isr\num
    .align  4
    isr\num:
        push    $\num
        jmp     isr_common
.endm

/* Exceptions */
ISR_NOERR  0    /* #DE */
ISR_NOERR  1    /* #DB */
ISR_NOERR  2    /* NMI */
ISR_NOERR  3    /* #BP */
ISR_NOERR  4    /* #OF */
ISR_NOERR  5    /* #BR */
ISR_NOERR  6    /* #UD */
ISR_NOERR  7    /* #NM */
ISR_ERR    8    /* #DF */
ISR_NOERR  9
ISR_ERR   10    /* #TS */
ISR_ERR   11    /* #NP */
ISR_ERR   12    /* #SS */
ISR_ERR   13    /* #GP */
ISR_ERR   14    /* #PF */
ISR_NOERR 15
ISR_NOERR 16    /* #MF */
ISR_ERR   17    /* #AC */
ISR_NOERR 18    /* #MC */
ISR_NOERR 19    /* #XM */

/* Hardware IRQs (vectors 0x20-0x2F after PIC remap) */
.set i, 32
.rept 16
ISR_NOERR i
.set i, i+1
.endr

/* SYSCALL software interrupt vector (0x80) */
ISR_NOERR 128

/* ── Common ISR handler ──────────────────────────────────────────────── */
    .global isr_common
    .align  4
isr_common:
    SAVE_REGS

    /*
     * Stack layout at this point (high → low):
     *   ... SS RSP RFLAGS CS RIP  ← hardware pushed
     *   error_code vector          ← our stubs pushed
     *   r15 r14 ... rax            ← SAVE_REGS
     *
     * vector is at rsp + FRAME_GPR
     * error_code at rsp + FRAME_GPR + 8
     */
    mov     FRAME_GPR(%rsp),     %rdi   /* vector → arg1               */
    mov     FRAME_GPR+8(%rsp),   %rsi   /* error_code → arg2           */
    mov     %rsp,                %rdx   /* frame ptr → arg3            */

    /* Align stack to 16 bytes before call */
    and     $-16, %rsp
    sub     $8,   %rsp

    /* vector < 32 = exception, >= 32 = hardware IRQ, 128 = syscall    */
    cmp     $128, %rdi
    je      .Lsyscall_entry
    cmp     $32,  %rdi
    jl      .Lexception_entry

    /* Hardware IRQ: call C dispatcher */
    call    x86_irq_dispatch
    jmp     .Ldone

.Lexception_entry:
    call    x86_exception_dispatch
    jmp     .Ldone

.Lsyscall_entry:
    /*
     * x86_64 Linux-compatible syscall ABI via INT 0x80 path:
     * rax=nr, rdi=a0, rsi=a1, rdx=a2, r10=a3, r8=a4, r9=a5
     * We recover these from the saved frame.
     */
    mov     FRAME_GPR+112(%rsp), %rdi  /* saved rax = syscall nr       */
    mov     FRAME_GPR+ 56(%rsp), %rsi  /* saved rdi = a0               */
    mov     FRAME_GPR+ 48(%rsp), %rdx  /* saved rsi = a1               */
    mov     FRAME_GPR+ 16(%rsp), %rcx  /* saved rdx = a2               */
    mov     FRAME_GPR+ 72(%rsp), %r8   /* saved r10 = a3  (via r10)    */
    mov     FRAME_GPR+ 64(%rsp), %r9   /* saved r8  = a4               */
    push    FRAME_GPR+ 56(%rsp)        /* saved r9  = a5               */
    call    arch_syscall_dispatch
    add     $8, %rsp
    /* write return value back to saved rax slot */
    mov     %rax, FRAME_GPR+112(%rsp)

.Ldone:
    /* Restore original rsp (undo alignment) */
    lea     FRAME_GPR+8+8(%rsp), %rax  /* past vector+error on old rsp */
    /* Actually: just restore from the frame offset we know            */
    mov     %rdx, %rsp                  /* rdx still holds original rsp */

    RESTORE_REGS
    add     $16, %rsp       /* pop vector + error_code                 */
    iretq


/* ── SYSCALL fast path (MSR_LSTAR) ─────────────────────────────────────
 * More efficient than INT 0x80 for user→kernel transitions.
 * Kernel sets MSR_LSTAR = &x86_syscall_entry in arch_init().
 * rcx = user RIP (saved by SYSCALL), r11 = user RFLAGS.
 * ──────────────────────────────────────────────────────────────────── */
    .global x86_syscall_entry
    .align  4
x86_syscall_entry:
    /* Swap to kernel GS (if used for per-CPU data) */
    swapgs

    /* Save user stack, load kernel stack */
    mov     %rsp, %r10          /* temporarily save user rsp in r10   */
    /* Kernel stack pointer would come from per-CPU data via GS prefix  */
    /* For simplicity: kernel stack is pre-allocated at boot            */

    /* Push minimal frame for SYSRETQ */
    push    %r11                /* user RFLAGS                         */
    push    %rcx                /* user RIP                            */
    push    %rbp

    /* System V: rax=nr, rdi=a0, rsi=a1, rdx=a2, r10=a3, r8=a4, r9=a5 */
    mov     %rax, %rdi          /* nr                                  */
    /* a0-a5 already in rsi, rdx, r10, r8, r9; put a3 in rcx          */
    mov     %r10, %rcx
    call    arch_syscall_dispatch
    /* return value in rax, passed back to user                        */

    pop     %rbp
    pop     %rcx                /* user RIP                            */
    pop     %r11                /* user RFLAGS                         */

    swapgs
    sysretq


/* ── IDT vector table (256 entries, referenced in arch_init.c) ─────────
 * We expose a function table rather than a raw IDT so arch_init.c
 * can call idt_set_gate() for each entry.
 * ──────────────────────────────────────────────────────────────────── */
    .section .rodata
    .global isr_stub_table
    .align  8
isr_stub_table:
    .quad isr0,   isr1,   isr2,   isr3,   isr4,   isr5,   isr6,   isr7
    .quad isr8,   isr9,   isr10,  isr11,  isr12,  isr13,  isr14,  isr15
    .quad isr16,  isr17,  isr18,  isr19,  isr32,  isr33,  isr34,  isr35
    .quad isr36,  isr37,  isr38,  isr39,  isr40,  isr41,  isr42,  isr43
    .quad isr44,  isr45,  isr46,  isr47,  isr128, 0, 0, 0

