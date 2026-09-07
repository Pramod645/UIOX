/*
 * uiox_boot_entry_arm32.S
 * UIOX Bootloader — ARMv7-A (Thumb-interworking) entry point.
 *
 * QEMU versatilepb: ROM at 0x00000000, RAM at 0x00100000.
 * QEMU passes:  r0 = 0 (machine type), r1 = ATAG base, r2 = DTB PA.
 */

    .syntax unified
    .arch   armv7-a
    .section ".boot", "ax", %progbits
    .global _start
    .type   _start, %function
    .thumb_func

_start:
    /* ------------------------------------------------------------------ */
    /* 1. Enter SVC mode, disable IRQ/FIQ, clear CPSR thumb flag          */
    /* ------------------------------------------------------------------ */
    cpsid   if
    mrs     r4, cpsr
    bic     r4, r4, #0x1F        /* clear mode bits                      */
    orr     r4, r4, #0x13        /* SVC mode                             */
    msr     cpsr_c, r4
    /*
    *The CPSID (Change Processor State IDentifier/Disable) instruction does not exist 
    in the AArch64 (64-bit ARM) instruction set.

    *AArch32 (A32/T32): Instructions like cpsid i or cpsid f are used to change processor state and 
    disable specific interrupt mask bits (like IRQ or FIQ) in the old CPSR/PSTATE.
    *AArch64: The CPSID and CPSIE mnemonics are invalid. AArch64 uses explicit system register 
    access instructions to modify the DAIF (Debug, A, I, F) interrupt mask flags.

    *AArch64 does not have a single Current Program Status Register (CPSR) like 32-bit ARM architectures; 
    instead, it uses an abstraction called PSTATE (Process State).
    */

    /* ------------------------------------------------------------------ */
    /* 2. Disable MMU / D-cache / I-cache in SCTLR                        */
    /* ------------------------------------------------------------------ */
    mrc     p15, 0, r4, c1, c0, 0
    bic     r4, r4, #(1 << 0)   /* M = 0 disable MMU                    */
    bic     r4, r4, #(1 << 2)   /* C = 0 disable D-cache                */
    bic     r4, r4, #(1 << 12)  /* I = 0 disable I-cache                */
    mcr     p15, 0, r4, c1, c0, 0
    isb

    /* Invalidate TLB */
    mcr     p15, 0, r4, c8, c7, 0
    dsb
    isb
    /*
    *MRC (Move to ARM Register from Coprocessor) becomes MRS (Move System Register to Register) 
    to read a system value into a general-purpose register.
    *MCR (Move to Coprocessor from ARM Register) becomes MSR (Move Register to System Register) to 
    write a general-purpose register value into a system register.
    */
    /* ------------------------------------------------------------------ */
    /* 3. Set up early stack (grows down from _boot_stack_top)            */
    /* ------------------------------------------------------------------ */
    ldr     sp, =_boot_stack_top

    /* Save r2 = DTB PA */
    mov     r9, r2

    /* ------------------------------------------------------------------ */
    /* 4. Zero BSS                                                        */
    /* ------------------------------------------------------------------ */
    ldr     r4, =__bss_start
    ldr     r5, =__bss_end
    mov     r6, #0
.Lbss_zero:
    cmp     r4, r5
    bge     .Lbss_done
    str     r6, [r4], #4
    b       .Lbss_zero
.Lbss_done:

    /* ------------------------------------------------------------------ */
    /* 5. Call C bootloader main(dtb_pa)                                  */
    /* ------------------------------------------------------------------ */
    mov     r0, r9
    bl      uiox_boot_main

.Lhang:
    wfe
    b       .Lhang

    .size _start, . - _start


    .section ".bss.stack", "aw", %nobits
    .balign 8
    .global _boot_stack_base
_boot_stack_base:
    .space  (8 * 1024)
    .global _boot_stack_top
_boot_stack_top:
