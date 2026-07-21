/*
 * linker/bsp_arm64.ld
 * ARM64 BSP standalone ELF linker script.
 * BSP loads at DRAM base (0x40000000) and runs until it jumps
 * to the kernel image loaded at 0x40400000.
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
/*ENTRY(uiox_arch_main)*/
/* Change in all four linker scripts */
ENTRY(bsp_entry)           /* ← was: ENTRY(uiox_arch_main) */

MEMORY
{
    /* BSP occupies first 4 MB of DRAM */
    BSP_RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 4M
}

SECTIONS
{
    . = ORIGIN(BSP_RAM);

    .text ALIGN(4) : {
        KEEP(*(.text.bsp_entry))   /* ensure entry is first */
        *(.text*)
        *(.rodata*)
    } > BSP_RAM

    /* Exception vector table must be 2KB-aligned for VBAR_EL1 */
    .vectors ALIGN(2048) : { KEEP(*(.vectors)) } > BSP_RAM

    .text ALIGN(4) : {
        *(.text.uiox_arch_main)   /* entry first */
        *(.text*)
        *(.rodata*)
    } > BSP_RAM

    .data ALIGN(8) : {
        *(.data*)
    } > BSP_RAM

    .bss ALIGN(8) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > BSP_RAM

    /* BSP stack: 64 KB at top of BSP region */
    . = ALIGN(4096);
    __stack_top = ORIGIN(BSP_RAM) + LENGTH(BSP_RAM);
    _soc_stack_base = __stack_top - 65536;

    /* Discard unwanted sections */
    /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
}
