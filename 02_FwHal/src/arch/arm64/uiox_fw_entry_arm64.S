/*
 * uiox_fw_entry_arm64.S
 * UIOX Firmware — AArch64 entry point.
 *
 * Called by bootloader after Stage 7 hand-off.
 * Sets up stack, zeros BSS, then calls uiox_fw_main(dtb_pa).
 */

    .section ".fw.entry", "ax"
    .global  _start
    .type    _start, %function

_start:
    /* Mask all async exceptions */
    msr     daifset, #0xf
    isb

    /* Set up early stack */
    adrp    x10, _fw_stack_top
    add     x10, x10, :lo12:_fw_stack_top
    mov     sp,  x10

    /* Save DTB PA (x0 from bootloader) */
    mov     x20, x0

    /* Zero BSS */
    adrp    x10, __fw_bss_start
    add     x10, x10, :lo12:__fw_bss_start
    adrp    x11, __fw_bss_end
    add     x11, x11, :lo12:__fw_bss_end
.Lbss_zero:
    cmp     x10, x11
    bge     .Lbss_done
    str     xzr, [x10], #8
    b       .Lbss_zero
.Lbss_done:

    /* Call firmware main */
    mov     x0, x20
    bl      uiox_fw_main

.Lhang:
    wfe
    b       .Lhang

    .size _start, . - _start

    /* Stack (16 KB) */
    .section ".bss.fw_stack", "aw", %nobits
    .balign 16
    .global _fw_stack_base
_fw_stack_base:
    .space (16 * 1024)
    .global _fw_stack_top
_fw_stack_top:
