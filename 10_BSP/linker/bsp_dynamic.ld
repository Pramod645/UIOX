/*
 * 10_BSP/linker/bsp_dynamic.ld
 *
 * UIOX BSP — linker script for the dynamic / secondary-bootloader build.
 *
 * OUTPUT_FORMAT and OUTPUT_ARCH are intentionally omitted so the linker
 * infers the correct ELF machine type from the input object files.
 * The -m / LD_EMUL flag in the Makefile (_do_arm32 → armelf,
 * _do_arm64 → aarch64elf, _do_riscv64 → elf64lriscv,
 * _do_x86_64 → elf_x86_64) sets the emulation explicitly — that is
 * the correct place to control the output format for a multi-arch build.
 *
 * Previously this file had:
 *   OUTPUT_FORMAT("elf64-littleaarch64")
 *   OUTPUT_ARCH(aarch64)
 * which caused arm-none-eabi-ld to error "cannot represent machine aarch64"
 * when building arm32, riscv64, or x86_64 dynamic BSP binaries.
 *
 * Memory map (tune BSP_LOAD_ADDR per SoC / arch):
 *   BSP_LOAD_ADDR  = 0x48000000   primary BL loads the BSP here
 *   STACK_SIZE     = 64 KiB
 *
 * @version 1.1.0
 * @date    2026-07-22
 */

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
     * Text — entry stub must be first so the primary BL jumps to it
     *----------------------------------------------------------------*/
    .text.bsp_entry : {
        KEEP(*(.text.bsp_entry))
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
