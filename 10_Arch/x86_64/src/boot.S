/* ============================================================
 * boot.S  —  x86_64 multiboot2 entry point and ISR stubs
 *
 * Structure:
 *   1. Multiboot2 header
 *   2. 32-bit protected mode entry (_start)
 *   3. Transition to 64-bit long mode
 *   4. 64-bit kernel entry (long_mode_start → calls boot_main)
 *   5. ISR stubs for all 256 IDT vectors
 *   6. Context-switch helper (cpu_context_switch_asm)
 * ============================================================ */

    .set MULTIBOOT2_MAGIC,       0xE85250D6
    .set MULTIBOOT2_ARCH_I386,   0
    .set MULTIBOOT2_HDR_LEN,     24
    .set MULTIBOOT2_CHECKSUM,    -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH_I386 + MULTIBOOT2_HDR_LEN)

/* ── Multiboot2 header (must be 8-byte aligned) ──────────── */
    .section .multiboot, "a"
    .align 8
multiboot2_header:
    .long  MULTIBOOT2_MAGIC
    .long  MULTIBOOT2_ARCH_I386
    .long  MULTIBOOT2_HDR_LEN
    .long  MULTIBOOT2_CHECKSUM
    /* end tag */
    .word  0        /* type  */
    .word  0        /* flags */
    .long  8        /* size  */

/* ── BSS: early stack and page tables ────────────────────── */
    .section .bss, "aw", @nobits
    .align 4096
pml4_table:
    .skip  4096
pdpt_table:
    .skip  4096
pd_table:
    .skip  4096
    .align 16
stack_bottom:
    .skip  16384        /* 16 KiB early kernel stack            */
stack_top:

/* ── 32-bit entry (_start) ───────────────────────────────── */
    .section .text
    .code32
    .global _start
    .type   _start, @function
_start:
    cli
    /* save multiboot info pointer (ebx) */
    movl    %ebx, %edi

    /* set up early stack */
    movl    $stack_top, %esp
    xorl    %ebp, %ebp

    /* check CPUID support (RFLAGS.ID bit) */
    pushfl
    popl    %eax
    movl    %eax, %ecx
    xorl    $(1 << 21), %eax
    pushl   %eax
    popfl
    pushfl
    popl    %eax
    xorl    %ecx, %eax
    testl   $(1 << 21), %eax
    jz      .no_cpuid

    /* check long mode support (CPUID leaf 0x80000001, EDX bit 29) */
    movl    $0x80000000, %eax
    cpuid
    cmpl    $0x80000001, %eax
    jl      .no_long_mode
    movl    $0x80000001, %eax
    cpuid
    testl   $(1 << 29), %edx
    jz      .no_long_mode

    /* ── build identity-map PML4 → PDPT → PD (2 MiB pages) */
    /* PML4[0] → pdpt_table */
    movl    $pdpt_table, %eax
    orl     $0x03, %eax         /* Present | Writable           */
    movl    %eax, pml4_table

    /* PDPT[0] → pd_table */
    movl    $pd_table, %eax
    orl     $0x03, %eax
    movl    %eax, pdpt_table

    /* PD: map first 2 GiB with 2 MiB huge pages */
    movl    $0, %ecx
.map_pd:
    movl    %ecx, %eax
    shll    $21, %eax           /* PA = index × 2 MiB           */
    orl     $0x83, %eax         /* Present | Writable | PS      */
    movl    %eax, pd_table(, %ecx, 8)
    incl    %ecx
    cmpl    $512, %ecx
    jl      .map_pd

    /* ── load CR3 */
    movl    $pml4_table, %eax
    movl    %eax, %cr3

    /* ── enable PAE (CR4.PAE) */
    movl    %cr4, %eax
    orl     $(1 << 5), %eax
    movl    %eax, %cr4

    /* ── set EFER.LME */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $(1 << 8), %eax
    wrmsr

    /* ── enable paging (CR0.PG | CR0.PE) */
    movl    %cr0, %eax
    orl     $(1 << 31 | 1 << 0), %eax
    movl    %eax, %cr0

    /* ── far jump to 64-bit code segment */
    ljmp    $0x08, $long_mode_start

.no_cpuid:
.no_long_mode:
    hlt
    jmp     .no_long_mode

/* ── 64-bit long mode entry ──────────────────────────────── */
    .code64
long_mode_start:
    /* reload all data segment registers */
    movw    $0x10, %ax    /* kernel data selector             */
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss

    /* set up proper 64-bit stack */
    movabsq $stack_top, %rsp
    xorq    %rbp, %rbp

    /* multiboot info pointer is still in rdi */
    /* call the C boot entry point */
    callq   boot_main

    /* should never return — halt */
1:  hlt
    jmp     1b

/* ============================================================
 * ISR stubs
 *
 * Exceptions without error code push a dummy 0 first.
 * The common handler saves all registers, calls
 * interrupt_dispatch(vector, error, frame), then restores.
 * ============================================================ */

/* macro: exception without error code */
.macro ISR_NOERR num
    .global isr\num
    isr\num:
    pushq   $0              /* dummy error code               */
    pushq   $\num           /* vector number                  */
    jmp     isr_common_stub
.endm

/* macro: exception with error code pushed by CPU */
.macro ISR_ERR num
    .global isr\num
    isr\num:
    pushq   $\num
    jmp     isr_common_stub
.endm

ISR_NOERR  0    /* #DE divide error                           */
ISR_NOERR  1    /* #DB debug                                  */
ISR_NOERR  2    /* NMI                                        */
ISR_NOERR  3    /* #BP breakpoint                             */
ISR_NOERR  4    /* #OF overflow                               */
ISR_NOERR  5    /* #BR bound range                            */
ISR_NOERR  6    /* #UD invalid opcode                         */
ISR_NOERR  7    /* #NM device not available                   */
ISR_ERR    8    /* #DF double fault            (error code)   */
ISR_NOERR  9    /* coprocessor segment overrun (obsolete)     */
ISR_ERR   10    /* #TS invalid TSS             (error code)   */
ISR_ERR   11    /* #NP segment not present     (error code)   */
ISR_ERR   12    /* #SS stack fault             (error code)   */
ISR_ERR   13    /* #GP general protection      (error code)   */
ISR_ERR   14    /* #PF page fault              (error code)   */
ISR_NOERR 15    /* reserved                                   */
ISR_NOERR 16    /* #MF x87 FP error                           */
ISR_ERR   17    /* #AC alignment check         (error code)   */
ISR_NOERR 18    /* #MC machine check                          */
ISR_NOERR 19    /* #XM SIMD FP                                */
ISR_NOERR 20    /* #VE virtualization                         */
/* 21–31 reserved */
.set i, 21
.rept 11
ISR_NOERR i
.set i, i+1
.endr
/* 32–255: IRQs */
.set i, 32
.rept 224
ISR_NOERR i
.set i, i+1
.endr

/* ── Common ISR stub ─────────────────────────────────────── */
    .global isr_common_stub
isr_common_stub:
    /* save all general-purpose registers */
    pushq   %rax
    pushq   %rcx
    pushq   %rdx
    pushq   %rbx
    pushq   %rbp
    pushq   %rsi
    pushq   %rdi
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    /*
     * Stack layout at this point:
     *   rsp+  0 : r15
     *   rsp+  8 : r14
     *   ...
     *   rsp+112 : rax
     *   rsp+120 : vector number
     *   rsp+128 : error code
     *   rsp+136 : rip   (hardware)
     *   rsp+144 : cs
     *   rsp+152 : rflags
     *   rsp+160 : rsp   (hardware, if privilege change)
     *   rsp+168 : ss
     *
     * C signature:
     *   void interrupt_dispatch(uint64_t vec,
     *                           uint64_t err,
     *                           void    *frame);
     */
    movq    120(%rsp), %rdi   /* vector → arg1                */
    movq    128(%rsp), %rsi   /* error code → arg2            */
    movq    %rsp, %rdx        /* frame pointer → arg3         */

    /* align stack to 16 bytes before call */
    andq    $-16, %rsp
    subq    $8, %rsp

    callq   interrupt_dispatch

    /* restore stack and registers */
    addq    $8, %rsp
    movq    136(%rsp), %rsp   /* restore original rsp value   */
    /* (unwind via iretq path) */

    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rdi
    popq    %rsi
    popq    %rbp
    popq    %rbx
    popq    %rdx
    popq    %rcx
    popq    %rax

    addq    $16, %rsp         /* pop vector + error code      */
    iretq

/* ============================================================
 * cpu_context_switch_asm(arch_regs_t *old, arch_regs_t *new)
 *
 * rdi = old context pointer
 * rsi = new context pointer
 *
 * Saves callee-saved registers into *old,
 * loads callee-saved registers from *new,
 * then returns into the new context's saved RIP.
 * ============================================================ */
    .global cpu_context_switch_asm
    .type   cpu_context_switch_asm, @function
cpu_context_switch_asm:
    /* ── save old context ────────────────────────────────── */
    movq    %rbx,  0(%rdi)
    movq    %rbp,  8(%rdi)
    movq    %r12, 16(%rdi)
    movq    %r13, 24(%rdi)
    movq    %r14, 32(%rdi)
    movq    %r15, 40(%rdi)
    /* save return address as RIP */
    movq    (%rsp), %rax
    movq    %rax,  48(%rdi)
    /* save stack pointer (after return address) */
    leaq    8(%rsp), %rax
    movq    %rax,  56(%rdi)

    /* ── load new context ────────────────────────────────── */
    movq   0(%rsi),  %rbx
    movq   8(%rsi),  %rbp
    movq  16(%rsi),  %r12
    movq  24(%rsi),  %r13
    movq  32(%rsi),  %r14
    movq  40(%rsi),  %r15
    /* restore stack pointer */
    movq  56(%rsi),  %rsp
    /* jump to new RIP */
    movq  48(%rsi),  %rax
    jmpq  *%rax
