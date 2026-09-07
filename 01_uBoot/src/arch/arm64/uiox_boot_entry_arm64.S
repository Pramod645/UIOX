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

    /*
    *EL (Exception Level): The general architectural term for privilege tiers in AArch64.
    *EL0 (Unprivileged / User tier): Lowest privilege level. Used for normal user-space applications and tasks.
    *EL1 (Privileged / Kernel tier): Medium-high privilege level. Used by Rich Operating System kernels 
        like Linux or Windows.
    *EL2 (Hypervisor tier): High privilege level reserved for virtualization monitors (Hypervisors 
        like KVM, Xen, or Type-1 hypervisors) that manage virtual machines. EL2 is optional in hardware.
    *(Reference) EL3 (Firmware tier): Highest privilege level, used for Secure Monitors, 
        low-level platform firmware, and TrustZone security state switching.

    *https://krinkinmu.github.io/2021/01/04/aarch64-exception-levels.html  
    */

    /* Set SPSR_EL2 so eret drops to EL1h with IRQs/FIQs masked */
    mov     x10, #0x3C5             /* EL1h, DAIF all masked             */
    msr     spsr_el2, x10
    adr     x10, .Lat_el1
    msr     elr_el2, x10
    eret
    /* 
    https://support.arm.com/documentation/ihi0048/b/Interrupt-Handling-and-Prioritization/Example-GIC-usage-models/Using-IRQs-and-FIQs-to-provide-Non-secure-and-Secure-interrupts 
    */
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
    /*
    *SCTLR_EL1 (System Control Register EL1) is a 64-bit AArch64 system register that provides 
    top-level control of the system's core and memory system at Exception Level 1 (EL1) and 
    Exception Level 0 (EL0).

    *Enables or disables the MMU (Memory Management Unit) for EL1/EL0 (M and C bits).
    *Caches: Controls instruction and data caching behaviors.
    *Endianness: Sets execution endianness for data and instructions at EL1 (EE) and EL0 (E0E).
    *Fault Handling: Configures alignment fault checking (A bit).
    */

    /* Invalidate TLBs */
    tlbi    vmalle1
    dsb     sy
    isb
    /*
    *TLBI VMALLE1 in AArch64 is an Arm system instruction that invalidates cached Translation Lookaside 
    Buffer (TLB) entries for all stage 1 translations at Exception Level 1 (EL1) that match the current 
    Virtual Machine Identifier (VMID)

    *A dsb sy followed by an isb on AArch64 is a sequence of ARM barrier instructions used to guarantee 
    complete memory visibility and pipeline synchronization before program execution continues.
    *DSB (Data Synchronization Barrier - sy for Full System): Stops all instructions after the barrier 
    from executing until all explicit memory accesses (loads and stores) before the barrier are fully 
    complete across the system.
    *ISB (Instruction Synchronization Barrier): Flushes the processor pipeline, throwing away any 
    prefetched instructions so that subsequent instructions are fetched and executed using the newest 
    system context or configuration.
    */

    /* ------------------------------------------------------------------ */
    /* 3. Set up early stack (8-byte aligned, grows down from _boot_stack) */
    /* ------------------------------------------------------------------ */
    adrp    x10, _boot_stack_top
    add     x10, x10, :lo12:_boot_stack_top
    mov     sp,  x10

    /* Save DTB pointer (x0 = DTB PA from QEMU) */
    mov     x20, x0
    /*
    *The ADRP (Address Page Relative) instruction in AArch64 calculates the 4KB page address of a program 
    label relative to the current Program Counter (PC) and stores it in a destination register.
    */

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
    wfe /* wait for event */
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
