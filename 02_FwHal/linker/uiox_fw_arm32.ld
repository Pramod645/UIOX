/*
 * linker/uiox_fw_arm32.ld
 * UIOX Firmware — ARMv7-A linker script (QEMU versatilepb).
 * Loaded at 0x00100000 (1 MB RAM base).
 */
/*
 * linker/uiox_fw_arm32.ld
 * UIOX Firmware — ARMv7-A linker script.
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
    fw_text  PT_LOAD FLAGS(0x5);
    fw_data  PT_LOAD FLAGS(0x6);
}

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00100000, LENGTH = 15M
}

SECTIONS
{
    . = 0x00100000;

    .fw.entry :
    {
        _fw_start = .;
        KEEP(*(.fw.entry))
    } > RAM  :fw_text

    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > RAM  :fw_text

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } > RAM  :fw_text

    .data ALIGN(4K) :
    {
        *(.data*)
    } > RAM  :fw_data

    __fw_bss_start = ALIGN(4);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > RAM  :fw_data

    .bss.fw_stack (NOLOAD) : { *(.bss.fw_stack) } > RAM  :fw_data
    __fw_bss_end  = ALIGN(4);

    _fw_stack_top = ORIGIN(RAM) + LENGTH(RAM) - 4;
    _fw_end       = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
