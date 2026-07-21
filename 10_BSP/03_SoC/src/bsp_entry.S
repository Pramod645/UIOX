/*
 * bsp_entry.S  —  BSP entry stub for standalone ELF.
 *
 * The SoC BootROM jumps here after loading the BSP ELF.
 * We set up the stack, zero BSS, then call uiox_arch_main().
 */

    .section .text.bsp_entry
    .global  bsp_entry
    .type    bsp_entry, @function

#if defined(ARCH_ARM64)
bsp_entry:
    /* Set stack pointer */
    adrp    x0, __stack_top
    add     x0, x0, :lo12:__stack_top
    mov     sp, x0

    /* Zero BSS */
    adrp    x0, __bss_start
    add     x0, x0, :lo12:__bss_start
    adrp    x1, __bss_end
    add     x1, x1, :lo12:__bss_end
1:  cmp     x0, x1
    bge     2f
    str     xzr, [x0], #8
    b       1b
2:
    /* Call uiox_arch_main(dtb_pa=0) — BootROM may pass DTB in x0 */
    /* If BootROM already put DTB address in x0, it is passed through */
    bl      uiox_arch_main
    /* Should never return */
1:  wfi
    b       1b

#elif defined(ARCH_ARM32)
bsp_entry:
    ldr     sp, =__stack_top
    /* Zero BSS */
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
1:  cmp     r0, r1
    bge     2f
    str     r2, [r0], #4
    b       1b
2:
    /* r0=0 (dtb_pa=0 default; BootROM may set r2=dtb) */
    mov     r0, #0
    bl      uiox_arch_main
1:  wfi
    b       1b

#elif defined(ARCH_X86_64)
bsp_entry:
    leaq    __stack_top(%rip), %rsp
    /* Zero BSS */
    leaq    __bss_start(%rip), %rdi
    leaq    __bss_end(%rip),   %rcx
    subq    %rdi, %rcx
    xorl    %eax, %eax
    rep stosb
    /* dtb_pa = 0 (rdi) */
    xorq    %rdi, %rdi
    callq   uiox_arch_main
1:  hlt
    jmp     1b

#elif defined(ARCH_RISCV64)
bsp_entry:
    la      sp, __stack_top
    /* Zero BSS */
    la      t0, __bss_start
    la      t1, __bss_end
1:  bgeu    t0, t1, 2f
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       1b
2:
    /* a0=hartid (from BootROM), a1=dtb_pa (from BootROM) */
    /* pass dtb_pa as first arg to uiox_arch_main */
    mv      a0, a1
    call    uiox_arch_main
1:  wfi
    j       1b
#endif
