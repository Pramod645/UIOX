/*
 * uiox_boot_entry_arm32.S  —  ARMv7-A bootloader entry.
 * On entry: r0 = ATAG / DTB pointer.
 */
    .arch   armv7-a
    .arm
    .section ".text.boot","ax"
    .global _start
    .type   _start, %function

_start:
    mov     r10, r0
    cpsid   if, #0x13              /* SVC mode, IRQ/FIQ off      */

    /* disable MMU / caches */
    mrc     p15,0,r0,c1,c0,0
    bic     r0,r0,#0x1005
    mcr     p15,0,r0,c1,c0,0
    isb

    /* TLB + I-cache invalidate */
    mcr     p15,0,r0,c8,c7,0
    mcr     p15,0,r0,c7,c5,0
    dsb
    isb

    ldr     sp, =_boot_stack_top

    /* zero BSS */
    ldr     r0,=_bss_start
    ldr     r1,=_bss_end
    mov     r2,#0
.Lzb: cmp r0,r1; bge .Lzd; str r2,[r0],#4; b .Lzb
.Lzd:

    bl      uboot_hw_init_arm32

    mov     r0, r10
    mov     r1, #0
    bl      uiox_boot_main

.Lhang: wfi; b .Lhang
    .size _start,.-_start

    .section ".bss.boot_stack","aw"
    .align 3
    .space 32768
_boot_stack_top:
