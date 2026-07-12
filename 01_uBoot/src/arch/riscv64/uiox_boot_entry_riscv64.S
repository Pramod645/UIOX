/*
 * uiox_boot_entry_riscv64.S
 * UIOX Bootloader — RISC-V RV64GC entry point.
 *
 * Target  : QEMU -machine virt (riscv64)
 * Loaded  : OpenSBI (M-mode firmware) jumps to us in S-mode at 0x80200000
 *           OR QEMU loads us directly at 0x80000000 in M-mode (no SBI).
 *
 * Register state on entry (OpenSBI FW_JUMP convention):
 *   a0 = hart ID   (uiox_cpu_hw.h: uiox_cpu_this_id = csr_read(mhartid))
 *   a1 = DTB PA    (physical address of the flattened device tree)
 *
 * Sequence:
 *   1. Park all secondary harts (only hart 0 proceeds)
 *   2. Disable all interrupts in mstatus / sstatus
 *   3. Set up early stack (8-byte aligned, grows down)
 *   4. Zero .bss
 *   5. Call uiox_boot_hw_riscv64_register()   [sets up UART, CLINT, PLIC]
 *   6. Call uiox_boot_main(dtb_pa)
 *   7. Spin on return (should never happen)
 *
 * @version 1.0.0
 * @date    2026-07-12
 */

    .option norvc          /* no compressed instructions in boot stub  */
    .section ".boot", "ax"
    .global _start
    .type   _start, @function

_start:
    /* ------------------------------------------------------------------ */
    /* 1. Park secondary harts — only hart 0 runs the bootloader          */
    /*    Mirrors arm64 secondary-spin pattern.                            */
    /* ------------------------------------------------------------------ */
    csrr    t0, mhartid          /* read hardware thread ID               */
    bnez    t0, .Lhang           /* hart != 0  → spin forever             */

    /* ------------------------------------------------------------------ */
    /* 2. Disable all machine-mode interrupts                              */
    /*    mstatus.MIE = 0  (bit 3)                                         */
    /* ------------------------------------------------------------------ */
    csrci   mstatus, (1 << 3)   /* clear MIE                             */
    csrwi   mie, 0              /* disable all interrupt enables         */

    /* ------------------------------------------------------------------ */
    /* 3. Set up early stack                                               */
    /*    Stack grows down from _boot_stack_top (defined in linker script) */
    /* ------------------------------------------------------------------ */
    la      sp, _boot_stack_top

    /* Save DTB PA — a1 holds it on entry; preserve across BSS zero loop  */
    mv      s0, a1              /* s0 = DTB PA                           */

    /* ------------------------------------------------------------------ */
    /* 4. Zero .bss                                                        */
    /*    Equivalent to the arm64 and arm32 BSS-zero loops.               */
    /* ------------------------------------------------------------------ */
    la      t0, __bss_start
    la      t1, __bss_end

.Lbss_zero:
    bgeu    t0, t1, .Lbss_done
    sd      zero, 0(t0)         /* store 8 bytes of zero                 */
    addi    t0, t0, 8
    j       .Lbss_zero

.Lbss_done:

    /* ------------------------------------------------------------------ */
    /* 5. Register RISC-V hw ops (SiFive UART16550, CLINT timer, PLIC)    */
    /*    Mirrors uiox_boot_hw_arm64_register() call pattern.             */
    /* ------------------------------------------------------------------ */
    call    uiox_boot_hw_riscv64_register

    /* ------------------------------------------------------------------ */
    /* 6. Call C bootloader main(dtb_pa)                                  */
    /*    arg0 (a0) = DTB physical address                                 */
    /* ------------------------------------------------------------------ */
    mv      a0, s0              /* restore DTB PA into a0                */
    call    uiox_boot_main

    /* ------------------------------------------------------------------ */
    /* 7. Should never return — spin (wfi saves power)                    */
    /*    Mirrors .Lhang in arm64 / arm32 entry files.                    */
    /* ------------------------------------------------------------------ */
.Lhang:
    wfi
    j       .Lhang

    .size _start, . - _start

/* ==========================================================================
 * Boot stack — 8 KiB, 16-byte aligned (RV64 ABI: sp must be 16-byte)
 * Same layout as arm64 / arm32 stacks.
 * ======================================================================= */
    .section ".bss.stack", "aw", @nobits
    .balign  16
    .global  _boot_stack_base
_boot_stack_base:
    .space   (8 * 1024)
    .global  _boot_stack_top
_boot_stack_top:
