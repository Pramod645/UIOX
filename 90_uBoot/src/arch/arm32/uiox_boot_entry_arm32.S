/*
 * uiox_boot_entry_arm32.S
 * ARMv7-A bootloader entry point.
 * x0 / r0 = ATAG or DTB pointer from firmware.
 */
    .arch   armv7-a
    .arm
    .section ".text.boot","ax"
    .global _start
    .type   _start, %function

_start:
    mov     r10, r0             /* save ATAG/DTB pointer         */

    /* switch to SVC mode, disable IRQ/FIQ */
    cpsid   if, #0x13

    /* disable MMU + caches in SCTLR */
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #0x1005     /* M, C, Z bits                  */
    mcr     p15, 0, r0, c1, c0, 0
    isb

    /* invalidate TLBs and I-cache */
    mcr     p15, 0, r0, c8, c7, 0   /* TLBIALL                  */
    mcr     p15, 0, r0, c7, c5, 0   /* ICIALLU                  */
    dsb
    isb

    /* set up stack */
    ldr     sp, =_boot_stack_top

    /* zero BSS */
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0
.Lzero_bss:
    cmp     r0, r1
    bge     .Lbss_done
    str     r2, [r0], #4
    b       .Lzero_bss
.Lbss_done:

    bl      uboot_hw_init_arm32

    mov     r0, r10             /* ATAG/DTB phys                 */
    mov     r1, #0              /* boot flags                    */
    bl      uiox_boot_main

.Lhang:
    wfi
    b       .Lhang

    .size _start, . - _start

    .section ".bss.boot_stack","aw"
    .align  3
    .space  32768
_boot_stack_top:
