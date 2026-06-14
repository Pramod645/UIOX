/*
 * uiox_boot_entry_arm64.S  —  AArch64 bootloader entry.
 * On entry: x0 = DTB physical address, CPU in EL2 or EL1,
 *           MMU off, caches off, IRQ/FIQ masked.
 */
    .arch armv8-a
    .section ".text.boot","ax"
    .global _start
    .type   _start, %function

_start:
    mov     x20, x0                  /* save DTB pointer          */

    /* ── drop from EL2 → EL1 if needed ────────────────────── */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #2
    bne     .Lel1

    mov     x0, #(1 << 31)           /* HCR_EL2.RW = 1           */
    msr     HCR_EL2, x0
    mov     x0, xzr
    msr     SCTLR_EL1, x0
    mov     x0, #0x3C5               /* EL1h, DAIF masked         */
    msr     SPSR_EL2, x0
    adr     x0, .Lel1
    msr     ELR_EL2, x0
    eret

.Lel1:
    /* ── invalidate caches / TLBs ──────────────────────────── */
    ic      iallu
    tlbi    vmalle1
    dsb     sy
    isb

    /* ── set stack ──────────────────────────────────────────── */
    adrp    x0, _boot_stack_top
    add     x0, x0, :lo12:_boot_stack_top
    mov     sp, x0

    /* ── zero BSS ───────────────────────────────────────────── */
    adrp    x0, _bss_start
    add     x0, x0, :lo12:_bss_start
    adrp    x1, _bss_end
    add     x1, x1, :lo12:_bss_end
.Lzbss:
    cmp     x0, x1
    bge     .Lzdone
    str     xzr, [x0], #8
    b       .Lzbss
.Lzdone:

    /* ── arch HW init ───────────────────────────────────────── */
    bl      uboot_hw_init_arm64

    /* ── call C main ────────────────────────────────────────── */
    mov     x0, x20
    mov     x1, xzr
    bl      uiox_boot_main

.Lhang:
    wfi
    b       .Lhang

    .size _start, . - _start

    .section ".bss.boot_stack","aw"
    .align  4
    .space  65536
_boot_stack_top:
