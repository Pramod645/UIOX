/*
 * linker/uiox_fw_arm32.ld
 * UIOX Firmware — ARMv7-A linker script (QEMU versatilepb).
 * Loaded at 0x00100000 (1 MB RAM base).
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

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
    } > RAM

    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > RAM

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } > RAM

    .data ALIGN(4K) :
    {
        *(.data*)
    } > RAM

    __fw_bss_start = ALIGN(4);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > RAM
    .bss.fw_stack (NOLOAD) : { *(.bss.fw_stack) } > RAM
    __fw_bss_end  = ALIGN(4);

    _fw_stack_top = ORIGIN(RAM) + LENGTH(RAM) - 4;
    _fw_end       = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
