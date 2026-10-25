/*
 * 30_KIX/linker/kernel_arm64.ld
 *
 * UIOX Kernel linker script — ARM64 (AArch64).
 *
 * @version 1.1.0  @date 2026-10-25
 *
 * FIX v1.1.0 — explicit PHDRS.  The script previously had none, so ld
 * chose the program headers and merged every output section into one
 * PT_LOAD with RWE.  That produced, on every link:
 *
 *     warning: uiox_kernel.elf has a LOAD segment with RWX permissions
 *
 * The warning is harmless for correctness — the image runs — but it means
 * W^X is not enforced: a stray write into a function pointer or a buffer
 * overrun can become executable code rather than a fault.  Bare-metal
 * does not have to live with that; the fix is to tell the linker which
 * sections are code and which are data.
 *
 * The split is by permission, not by section name:
 *     text    R + X   .text.entry, .text        (FLAGS 5 = R|X)
 *     rodata  R       .rodata                   (FLAGS 4 = R)
 *     data    RW      .data, .bss, .stack       (FLAGS 6 = R|W)
 *
 * .bss and .stack are NOLOAD: MemSiz > FileSiz is expected and correct,
 * since the loader zeroes them rather than reading them from the file.
 * Keep the three segments page-aligned (MAXPAGESIZE below) so each gets
 * its own mapping when a MMU is enabled.
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x40200000;
STACK_SIZE       = 0x10000;     /* 64 KiB kernel stack */
MAXPAGESIZE      = 0x1000;      /* 4 KiB — segment alignment */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

PHDRS
{
    text   PT_LOAD FLAGS(5);    /* R E — code only            */
    rodata PT_LOAD FLAGS(4);    /* R   — read-only data       */
    data   PT_LOAD FLAGS(6);    /* RW  — data, bss, stack     */
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point must be at the very start of the image so the BSP's
     * uiox_bsp_jump_to_kernel() branch lands on uiox_kernel_main.
     *------------------------------------------------------------------*/
    .text.entry : ALIGN(MAXPAGESIZE) {
        KEEP(*(.text.entry))
        KEEP(*(.text.uiox_kernel_main))
    } > DRAM :text

    .text : ALIGN(16) {
        *(.text .text.*)
    } > DRAM :text

    /*------------------------------------------------------------------
     * Read-only data ends the executable region.  The linker symbols are
     * taken AFTER .rodata so _text_end marks the end of everything that
     * may be mapped read-only/executable, not just code.
     *------------------------------------------------------------------*/
    .rodata : ALIGN(MAXPAGESIZE) {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    } > DRAM :rodata

    _text_start = ADDR(.text.entry);
    _text_end   = .;

    /*------------------------------------------------------------------
     * Writable region.  Page-aligned so it lands in its own PT_LOAD and
     * does not share a mapping with code under a MMU.
     *------------------------------------------------------------------*/
    .data : ALIGN(MAXPAGESIZE) {
        *(.data .data.*)
    } > DRAM :data

    .bss (NOLOAD) : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DRAM :data

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 16-byte aligned per AArch64 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(16);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(16);
        _stack_top = .;
    } > DRAM :data

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
    /DISCARD/ : { *(.eh_frame*) }
}
