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
 * @version 1.0.0  @date 2026-07-25
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x60200000;
STACK_SIZE       = 0x8000;      /* 32 KiB kernel stack — conservative for arm32 */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point first so the BSP branch lands on uiox_kernel_main.
     *------------------------------------------------------------------*/
    .text.entry : {
        KEEP(*(.text.entry))
        KEEP(*(.text.uiox_kernel_main))
    } > DRAM

    .text : {
        *(.text .text.*)
    } > DRAM

    .rodata : {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    } > DRAM

    _text_start = ADDR(.text.entry);
    _text_end   = .;

    .data : {
        *(.data .data.*)
    } > DRAM

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > DRAM

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 8-byte aligned per ARM32 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(8);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(8);
        _stack_top = .;
    } > DRAM

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}
