/*
 * linker/uiox_fw_x86_64.ld
 * UIOX Firmware — x86_64 linker script (QEMU q35 / Multiboot2).
 * Loaded at 0x00100000 (1 MB, Multiboot2 standard).
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

MEMORY
{
    KERN (rwx) : ORIGIN = 0x00100000, LENGTH = 63M
}

SECTIONS
{
    . = 0x00100000;

    .fw.multiboot ALIGN(8) :
    {
        KEEP(*(.fw.multiboot))
    } > KERN

    .fw.entry :
    {
        _fw_start = .;
        KEEP(*(.fw.entry))
    } > KERN

    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > KERN

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } > KERN

    .data ALIGN(4K) :
    {
        *(.data*)
    } > KERN

    __fw_bss_start = ALIGN(8);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > KERN
    .bss.fw_stack (NOLOAD) : { *(.bss.fw_stack) } > KERN
    __fw_bss_end  = ALIGN(8);

    _fw_stack_top = __fw_bss_end;
    _fw_end       = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
