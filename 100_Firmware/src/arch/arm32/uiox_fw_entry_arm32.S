/*
 * uiox_fw_entry_arm32.S
 * UIOX Firmware — ARMv7-A entry point (QEMU versatilepb).
 */

    .syntax unified
    .arch   armv7-a
    .section ".fw.entry", "ax", %progbits
    .global _start
    .thumb_func

_start:
    /* SVC mode, IRQ/FIQ disabled */
    cpsid   if
    mrs     r4, cpsr
    bic     r4, r4, #0x1F
    orr     r4, r4, #0x13
    msr     cpsr_c, r4

    /* Stack */
    ldr     sp, =_fw_stack_top

    /* Save DTB/ATAG pointer (r0 from bootloader) */
    mov     r9, r0

    /* Zero BSS */
    ldr     r4, =__fw_bss_start
    ldr     r5, =__fw_bss_end
    mov     r6, #0
.Lbss_zero:
    cmp     r4, r5
    bge     .Lbss_done
    str     r6, [r4], #4
    b       .Lbss_zero
.Lbss_done:

    /* Call firmware main */
    mov     r0, r9
    bl      uiox_fw_main

.Lhang:
    wfe
    b       .Lhang

    .section ".bss.fw_stack", "aw", %nobits
    .balign 8
    .global _fw_stack_base
_fw_stack_base:
    .space  (8 * 1024)
    .global _fw_stack_top
_fw_stack_top:
