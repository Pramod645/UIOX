/*
 * uiox_boot_entry_arm64.S
 * AArch64 bootloader entry point.
 * Called by QEMU / firmware with:
 *   x0 = DTB physical address
 *   x1..x3 = 0
 * CPU is in EL2 or EL1, MMU off, caches off.
 */
    .arch armv8-a
    .section ".text.boot","ax"
    .global _start
    .type   _start, %function

_start:
    /* -- save DTB address before we clobber x0 -------------- */
    mov     x20, x0

    /* -- drop from EL2 to EL1 if needed --------------------- */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #2
    bne     .Lel1_already

    /* configure HCR_EL2: RW=1 (EL1 is AArch64) */
    mov     x0, #(1 << 31)
    msr     HCR_EL2, x0

    /* set SCTLR_EL1 to a known state */
    mov     x0, xzr
    msr     SCTLR_EL1, x0

    /* fake EL1h exception return */
    mov     x0, #0x3C5         /* DAIF masked, EL1h             */
    msr     SPSR_EL2, x0
    adr     x0, .Lel1_already
    msr     ELR_EL2, x0
    eret

.Lel1_already:
    /* -- invalidate I-cache + TLBs -------------------------- */
    ic      iallu
    tlbi    vmalle1
    dsb     sy
    isb

    /* -- set up stack --------------------------------------- */
    adrp    x0, _boot_stack_top
    add     x0, x0, :lo12:_boot_stack_top
    mov     sp, x0

    /* -- zero BSS ------------------------------------------ */
    adrp    x0, _bss_start
    add     x0, x0, :lo12:_bss_start
    adrp    x1, _bss_end
    add     x1, x1, :lo12:_bss_end
.Lzero_bss:
    cmp     x0, x1
    bge     .Lbss_done
    str     xzr, [x0], #8
    b       .Lzero_bss
.Lbss_done:

    /* -- call arch HW init ---------------------------------- */
    bl      uboot_hw_init_arm64

    /* -- call C bootloader main ----------------------------- */
    mov     x0, x20             /* DTB phys address             */
    mov     x1, xzr             /* boot flags = 0               */
    bl      uiox_boot_main

    /* -- should never return -------------------------------- */
.Lhang:
    wfi
    b       .Lhang

    .size _start, . - _start

    /* -- 64 KB boot stack ----------------------------------- */
    .section ".bss.boot_stack","aw"
    .align  4
    .space  65536
_boot_stack_top:
