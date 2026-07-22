/*
 * 10_BSP/linker/bsp_dynamic.ld
 *
 * UIOX BSP — linker script for the dynamic / secondary-bootloader build.
 *
 * The BSP binary is loaded by the primary bootloader (01_uBoot) into SRAM
 * or early DRAM and executed before the kernel ELF is available.
 *
 * Memory map (example — tune BSP_LOAD_ADDR per SoC):
 *
 *   BSP_LOAD_ADDR  = 0x48000000   primary BL loads the BSP here
 *   KERNEL_LOAD    = 0x40000000   BSP will DMA the kernel ELF here
 *   STACK_SIZE     = 64K
 *
 * @version 1.0.0
 * @date    2026-07-21
 */

OUTPUT_FORMAT("elf64-littleaarch64")   /* overridden per-arch by ARCH_LDFLAGS */
OUTPUT_ARCH(aarch64)

ENTRY(uiox_bsp_entry)

BSP_LOAD_ADDR  = 0x48000000;
STACK_SIZE     = 0x10000;    /* 64 KiB */

MEMORY
{
    SRAM  (rwx) : ORIGIN = BSP_LOAD_ADDR, LENGTH = 4M
}

SECTIONS
{
    . = BSP_LOAD_ADDR;

    /*----------------------------------------------------------------
     * Text
     *----------------------------------------------------------------*/
    .text.bsp_entry : {
        KEEP(*(.text.bsp_entry))    /* assembly stub must be first  */
    } > SRAM

    .text : {
        *(.text .text.*)
    } > SRAM

    .rodata : {
        *(.rodata .rodata.*)
    } > SRAM

    /*----------------------------------------------------------------
     * Data
     *----------------------------------------------------------------*/
    .data : {
        *(.data .data.*)
    } > SRAM

    /*----------------------------------------------------------------
     * BSS  (zeroed by bsp_entry.S before entering C)
     *----------------------------------------------------------------*/
    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    } > SRAM

    /*----------------------------------------------------------------
     * Stack  (grows down from _stack_top)
     *----------------------------------------------------------------*/
    . = ALIGN(16);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        _stack_top = .;
    } > SRAM

    /*----------------------------------------------------------------
     * Discard debug / metadata
     *----------------------------------------------------------------*/
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}
