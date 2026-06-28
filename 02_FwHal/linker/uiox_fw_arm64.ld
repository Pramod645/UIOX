/*
 * linker/uiox_fw_arm64.ld
 * UIOX Firmware — AArch64 linker script.
 * Firmware image loaded at 0x40000000 (DRAM base, QEMU virt).
 * Bootloader kernel loads at 0x40080000 — firmware stays below that.
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

MEMORY
{
    DRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 64M
}

SECTIONS
{
    . = 0x40000000;

    .fw.entry ALIGN(4K) :
    {
        _fw_start = .;
        KEEP(*(.fw.entry))
    } > DRAM

    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > DRAM

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > DRAM

    .data ALIGN(4K) :
    {
        *(.data*)
        *(.gnu.linkonce.d*)
    } > DRAM

    __fw_bss_start = ALIGN(8);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > DRAM
    .bss.fw_stack (NOLOAD) : { *(.bss.fw_stack) } > DRAM
    __fw_bss_end  = ALIGN(8);

    _fw_stack_top = __fw_bss_end;
    _fw_end       = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
