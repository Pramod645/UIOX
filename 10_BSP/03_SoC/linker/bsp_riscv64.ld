OUTPUT_ARCH(riscv)
/*ENTRY(uiox_arch_main)*/
/* Change in all four linker scripts */
ENTRY(bsp_entry)           /* ← was: ENTRY(uiox_arch_main) */

MEMORY
{
    BSP_RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 2M
}

SECTIONS
{
    . = ORIGIN(BSP_RAM);

    .text ALIGN(4) : {
        KEEP(*(.text.bsp_entry))   /* ensure entry is first */
        *(.text*)
        *(.rodata*)
    } > BSP_RAM

    .text   ALIGN(4) : { *(.text*) *(.rodata*) } > BSP_RAM
    .data   ALIGN(8) : { *(.data*) } > BSP_RAM
    .bss    ALIGN(8) : {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
    } > BSP_RAM

    __stack_top = ORIGIN(BSP_RAM) + LENGTH(BSP_RAM);
    _soc_stack_base = __stack_top - 65536;

    /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
}
