/*
 * 10_BSP/src/bsp_entry.S
 *
 * UIOX BSP — assembly entry stub (dynamic / secondary-bootloader build only).
 *
 * The primary bootloader (uiox_boot_arch_jump) jumps to the symbol
 * uiox_bsp_entry with the same register convention it uses for the kernel:
 *
 *   ARM64:  x0 = dtb_pa,  x1 = args_pa
 *   ARM32:  r2 = dtb_pa,  r3 = args_pa
 *   x86-64: rdi = args_pa, rsi = dtb_pa   (SysV ABI)
 *   RV64:   a0 = dtb_pa,  a1 = args_pa
 *
 * This stub:
 *   1. Sets the stack pointer to _stack_top (defined by the BSP linker script)
 *   2. Zeros the BSS (loop: _bss_start … _bss_end)
 *   3. Calls uiox_bsp_entry_c(dtb_pa, args_pa)   — never returns
 *
 * @version 1.0.0
 * @date    2026-07-21
 */

/* ============================================================ ARM64 ======= */
#if defined(__aarch64__)

    .section ".text.bsp_entry", "ax", %progbits
    .global uiox_bsp_entry
    .type   uiox_bsp_entry, %function
    .align  3

uiox_bsp_entry:
    /* x0 = dtb_pa, x1 = args_pa — preserve across BSS zero */
    mov  x19, x0
    mov  x20, x1

    /* Set kernel stack (16-byte aligned) */
    adrp x9, _stack_top
    add  x9, x9, :lo12:_stack_top
    and  x9, x9, #~0xF
    mov  sp, x9

    /* Zero BSS */
    adrp x10, _bss_start
    add  x10, x10, :lo12:_bss_start
    adrp x11, _bss_end
    add  x11, x11, :lo12:_bss_end
.Lbss_loop_arm64:
    cmp  x10, x11
    bge  .Lbss_done_arm64
    strb wzr, [x10], #1
    b    .Lbss_loop_arm64
.Lbss_done_arm64:
    dsb  sy
    isb

    /* Call C entry */
    mov  x0, x19      /* dtb_pa  */
    mov  x1, x20      /* args_pa */
    bl   uiox_bsp_entry_c

    /* Should never return */
.Lhalt_arm64:
    wfi
    b    .Lhalt_arm64

    .size uiox_bsp_entry, . - uiox_bsp_entry

/* ============================================================ ARM32 ======= */
#elif defined(__arm__)

    .section ".text.bsp_entry", "ax", %progbits
    .global uiox_bsp_entry
    .type   uiox_bsp_entry, %function
    .arm
    .align  2

uiox_bsp_entry:
    /* r2 = dtb_pa, r3 = args_pa — preserve in callee-saved registers */
    mov  r4, r2
    mov  r5, r3

    /* Set stack (8-byte aligned) */
    ldr  sp, =_stack_top
    bic  sp, sp, #7

    /* Zero BSS */
    ldr  r6, =_bss_start
    ldr  r7, =_bss_end
    mov  r8, #0
.Lbss_loop_arm32:
    cmp  r6, r7
    bge  .Lbss_done_arm32
    strb r8, [r6], #1
    b    .Lbss_loop_arm32
.Lbss_done_arm32:
    dsb
    isb

    /* Call C entry: uiox_bsp_entry_c(dtb_pa, args_pa) */
    mov  r0, r4
    mov  r1, r5
    bl   uiox_bsp_entry_c

.Lhalt_arm32:
    wfi
    b    .Lhalt_arm32

    .size uiox_bsp_entry, . - uiox_bsp_entry

/* ============================================================ x86-64 ====== */
#elif defined(__x86_64__)

    .section ".text.bsp_entry", "ax", @progbits
    .global uiox_bsp_entry
    .type   uiox_bsp_entry, @function
    .align  4

uiox_bsp_entry:
    /*
     * SysV ABI on entry (from uiox_boot_arch_jump x86 variant):
     *   rdi = args_pa,  rsi = dtb_pa
     * We need to call uiox_bsp_entry_c(dtb_pa, args_pa)
     * so swap: rdi=dtb_pa(rsi), rsi=args_pa(rdi)
     */
    movq %rsi, %rax      /* rax = dtb_pa  */
    movq %rdi, %rsi      /* rsi = args_pa */
    movq %rax, %rdi      /* rdi = dtb_pa  */

    /* Set stack (16-byte aligned) */
    leaq _stack_top(%rip), %rsp
    andq $~0xF, %rsp

    /* Zero BSS */
    leaq _bss_start(%rip), %r8
    leaq _bss_end(%rip),   %r9
.Lbss_loop_x86:
    cmpq %r9, %r8
    jge  .Lbss_done_x86
    movb $0, (%r8)
    incq %r8
    jmp  .Lbss_loop_x86
.Lbss_done_x86:
    mfence

    /* rdi = dtb_pa, rsi = args_pa — already set above */
    callq uiox_bsp_entry_c

.Lhalt_x86:
    hlt
    jmp  .Lhalt_x86

    .size uiox_bsp_entry, . - uiox_bsp_entry

/* ============================================================ RISC-V 64 === */
#elif defined(__riscv)

    .section ".text.bsp_entry", "ax", @progbits
    .global uiox_bsp_entry
    .type   uiox_bsp_entry, @function
    .align  3

uiox_bsp_entry:
    /* a0 = dtb_pa, a1 = args_pa */
    mv   s0, a0
    mv   s1, a1

    /* Set stack (16-byte aligned) */
    la   sp, _stack_top
    andi sp, sp, ~0xF

    /* Zero BSS */
    la   t0, _bss_start
    la   t1, _bss_end
.Lbss_loop_riscv:
    bge  t0, t1, .Lbss_done_riscv
    sb   zero, 0(t0)
    addi t0, t0, 1
    j    .Lbss_loop_riscv
.Lbss_done_riscv:
    fence

    /* Call C entry */
    mv   a0, s0      /* dtb_pa  */
    mv   a1, s1      /* args_pa */
    call uiox_bsp_entry_c

.Lhalt_riscv:
    wfi
    j    .Lhalt_riscv

    .size uiox_bsp_entry, . - uiox_bsp_entry

#else
#  error "bsp_entry.S: unsupported architecture"
#endif
