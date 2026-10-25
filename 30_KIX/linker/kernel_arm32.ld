/*
 * 30_KIX/linker/kernel_arm32.ld
 *
 * UIOX Kernel linker script — ARM32 (ARMv7-A).
 *
 * KERNEL_LOAD_ADDR must match uiox_bsp_config_t.kernel_load_pa in
 * 10_BSP/src/uiox_bsp_main.c (dynamic mode) and the address the
 * primary bootloader places the kernel image (static mode).
 *
 * Memory layout:
 *   0x60200000  kernel load base  (ARM Versatile / QEMU virt typical DRAM)
 *               .text.entry   — uiox_kernel_main (must be first)
 *               .text
 *               .rodata
 *               .data
 *               .bss          (NOLOAD, zeroed by bss_zero())
 *               .stack        (NOLOAD, grows down from _stack_top)
 *
 * @version 1.1.0  @date 2026-10-25
 *
 * FIX v1.1.0 — explicit PHDRS.  Without them ld chose the program
 * headers and merged every output section into one PT_LOAD with RWE:
 *     warning: uiox_kernel.elf has a LOAD segment with RWX permissions
 * Harmless for correctness — the image runs — but it means W^X is not
 * enforced.  The split below is by permission: code R+X, rodata R,
 * data/bss/stack RW.  See kernel_arm64.ld for the fuller note.
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x60200000;
STACK_SIZE       = 0x8000;      /* 32 KiB kernel stack — conservative for arm32 */
MAXPAGESIZE      = 0x1000;      /* 4 KiB — segment alignment */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

PHDRS
{
    text   PT_LOAD FLAGS(5);    /* R E — code only        */
    rodata PT_LOAD FLAGS(4);    /* R   — read-only data   */
    data   PT_LOAD FLAGS(6);    /* RW  — data, bss, stack */
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point first so the BSP branch lands on uiox_kernel_main.
     *------------------------------------------------------------------*/
    .text.entry : ALIGN(MAXPAGESIZE) {
        KEEP(*(.text.entry))
        KEEP(*(.text.uiox_kernel_main))
    } > DRAM :text

    .text : ALIGN(16) {
        *(.text .text.*)
    } > DRAM :text

    .rodata : ALIGN(MAXPAGESIZE) {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    } > DRAM :rodata

    _text_start = ADDR(.text.entry);
    _text_end   = .;

    .data : ALIGN(MAXPAGESIZE) {
        *(.data .data.*)
    } > DRAM :data

    .bss (NOLOAD) : ALIGN(4) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > DRAM :data

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 8-byte aligned per ARM32 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(8);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(8);
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
