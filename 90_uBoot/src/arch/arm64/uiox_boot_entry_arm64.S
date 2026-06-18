/*
 * uiox_boot_entry_arm64.S
 * UIOX Bootloader — AArch64 entry point.
 *
 * Reset vector for QEMU virt machine (ARM64).
 * Loaded by QEMU firmware at 0x40000000 (DRAM base).
 *
 * Sequence:
 *   1. Drop from EL2 → EL1 if running at EL2
 *   2. Disable MMU / D-cache / I-cache in SCTLR_EL1
 *   3. Set up early stack
 *   4. Zero BSS
 *   5. Call uiox_boot_main(dtb_pa)   [x0 = DTB phys addr from QEMU]
 */

    .section ".boot", "ax"
    .global _start
    .type   _start, %function

_start:
    /* ------------------------------------------------------------------ */
    /* 1. Determine current EL and drop to EL1 if at EL2                  */
    /* ------------------------------------------------------------------ */
    mrs     x9, CurrentEL
    lsr     x9, x9, #2
    cmp     x9, #2
    bne     .Lat_el1

    /* We are at EL2 — configure HCR_EL2 to route to EL1 AArch64 */
    mov     x10, #(1 << 31)         /* HCR_EL2.RW = 1 (EL1 is AArch64) */
    msr     hcr_el2, x10

    /* Set SPSR_EL2 so eret drops to EL1h with IRQs/FIQs masked */
    mov     x10, #0x3C5             /* EL1h, DAIF all masked             */
    msr     spsr_el2, x10
    adr     x10, .Lat_el1
    msr     elr_el2, x10
    eret

.Lat_el1:
    /* ------------------------------------------------------------------ */
    /* 2. Disable MMU, D-cache, I-cache via SCTLR_EL1                     */
    /* ------------------------------------------------------------------ */
    mrs     x10, sctlr_el1
    bic     x10, x10, #(1 << 0)    /* M   = 0 disable MMU               */
    bic     x10, x10, #(1 << 2)    /* C   = 0 disable D-cache           */
    bic     x10, x10, #(1 << 12)   /* I   = 0 disable I-cache           */
    msr     sctlr_el1, x10
    isb

    /* Invalidate TLBs */
    tlbi    vmalle1
    dsb     sy
    isb

    /* ------------------------------------------------------------------ */
    /* 3. Set up early stack (8-byte aligned, grows down from _boot_stack) */
    /* ------------------------------------------------------------------ */
    adrp    x10, _boot_stack_top
    add     x10, x10, :lo12:_boot_stack_top
    mov     sp,  x10

    /* Save DTB pointer (x0 = DTB PA from QEMU) */
    mov     x20, x0

    /* ------------------------------------------------------------------ */
    /* 4. Zero .bss                                                        */
    /* ------------------------------------------------------------------ */
    adrp    x10, __bss_start
    add     x10, x10, :lo12:__bss_start
    adrp    x11, __bss_end
    add     x11, x11, :lo12:__bss_end
.Lbss_zero:
    cmp     x10, x11
    bge     .Lbss_done
    str     xzr, [x10], #8
    b       .Lbss_zero
.Lbss_done:

    /* ------------------------------------------------------------------ */
    /* 5. Call C bootloader main                                           */
    /* ------------------------------------------------------------------ */
    mov     x0, x20                 /* arg0 = dtb_pa                     */
    bl      uiox_boot_main

    /* Should never return — if it does, spin */
.Lhang:
    wfe
    b       .Lhang

    .size _start, . - _start


/* =========================================================================
 * Early boot stack (16 KB, placed in .bss.stack section)
 * ====================================================================== */

    .section ".bss.stack", "aw", %nobits
    .balign 16
    .global _boot_stack_base
_boot_stack_base:
    .space  (16 * 1024)
    .global _boot_stack_top
_boot_stack_top:
