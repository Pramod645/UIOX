/*
 * 30_KIX/linker/kernel_arm64.ld
 *
 * UIOX Kernel linker script — ARM64 (AArch64).
 *
 * KERNEL_LOAD_ADDR must match uiox_bsp_config_t.kernel_load_pa in
 * 10_BSP/src/uiox_bsp_main.c (dynamic mode) and the address the
 * primary bootloader places the kernel image (static mode).
 *
 * Memory layout:
 *   0x40200000  kernel load base
 *               .text.entry   — uiox_kernel_main (must be first)
 *               .text         — all other code
 *               .rodata
 *               .data
 *               .bss          (NOLOAD, zeroed by bss_zero())
 *               .stack        (NOLOAD, grows down from _stack_top)
 *
 * @version 1.0.0  @date 2026-07-25
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x40200000;
STACK_SIZE       = 0x10000;     /* 64 KiB kernel stack */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point must be at the very start of the image so the BSP's
     * uiox_bsp_jump_to_kernel() branch lands on uiox_kernel_main.
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

    /*------------------------------------------------------------------
     * Linker symbols used by uiox_kernel_main.c and uiox_ks_boot_entry
     *------------------------------------------------------------------*/
    _text_start = ADDR(.text.entry);
    _text_end   = .;

    .data : {
        *(.data .data.*)
    } > DRAM

    .bss (NOLOAD) : {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DRAM

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 16-byte aligned per AArch64 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(16);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(16);
        _stack_top = .;
    } > DRAM

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}
