/*
 *Extension Description
 *M         Integer multiplication and division
 *A         Atomic instructions
 *F         Single-precision floating-point
 *D         Double-precision floating-point
 *Q         Quad-precision floating-point
 *C         16-bit compressed instructions
 *B         Bit manipulation
 *J         Extension for dynamically translated languages
 *P         Packed Single instruction, multiple data (SIMD) instructions
 *V         Vector operations
 *H         Hypervisor extension
 *Zicsr     Control and status register instructions
 *Zifencei  Instruction fetch-fence
 *G         Abbreviation for IMAFD, Zicsr, and Zifencei extensions
 *
 *
 *Number of levels      Supported Modes     Intended Usage
 *1                     M                   Simple embedded systems
 *2                     M, U                Secure embedded systems
 *3                     M, S, U             Systems running Unix-like operating systems
 *
 *S-->supervisor mode
 *M-->machine-mode
 *U-->user mode
 *
 *RISC-V ISA is divided into two parts, non-privileged and privileged ISAs.
 *Non-privileged ISA defines the base integer instruction set, and most of the
 *CSRs and extensions. Privileged ISA has its own extensions such as Hypervisor
 *extension, and defines the CSRs needed for the different privilege modes.
 *
 *RISC-V privileged ISA is divided in three different levels as seen above.
 *Different levels have different supported privilege modes. M-mode stands for
 *machine-mode, S-mode for supervisor mode and U-mode for user mode.
 *M-mode has the highest privileges and is the only mandatory privilege mode
 *for a RISC-V processor. As it has the lowest level access, only trusted
 *code should be run in M-mode. U-mode is the least privileged mode. Every end
 *user has U-mode privileges, so no low-level code should be allowed to run in
 *U-mode. S-mode interacts between the user and lower level system resources.
 *----------------------------------------------------------------------------*
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
 *   5. Call uiox_boot_hw_riscv64_register()   [sets up UART, CLINT, PLIC], its diabled from here as called from boot main
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
    /* csrr instruction used to read the value of a Control and Status Register 
    (CSR) into a general-purpose integer register                         
    *The mhartid is a read-only Control and Status Register (CSR) in RISC-V that 
    holds the integer ID of the hardware thread (hart) currently running the code. */
    /* ------------------------------------------------------------------ */
    /* 2. Disable all machine-mode interrupts                              */
    /*    mstatus.MIE = 0  (bit 3)                                         */
    /* ------------------------------------------------------------------ */
    csrci   mstatus, (1 << 3)   /* clear MIE: Machine Interrupt Enable     */
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
    /*call    uiox_boot_hw_riscv64_register       disabled from here, called from boot main file */ 

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
