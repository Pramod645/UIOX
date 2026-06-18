/*
 * linker/uiox_boot_arm32.ld
 * UIOX Bootloader — ARMv7-A linker script.
 *
 * Target   : QEMU versatilepb
 * ROM base : 0x00000000 (QEMU loads image here from -kernel flag)
 * RAM base : 0x00100000 (1 MB — mirrors uiox_arm32.ld KERNEL region)
 * Stack    : 0x00F00000 (top of 15 MB RAM region)
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
    RAM (rwx) : ORIGIN = 0x00100000, LENGTH = 15M
}

SECTIONS
{
    . = 0x00000000;

    /* ── Boot entry (ROM) ─────────────────────────────────────────── */
    .boot :
    {
        _boot_start = .;
        KEEP(*(.boot))
        KEEP(*(.boot.text))
        _boot_end = .;
    } > ROM

    /* ── Kernel text ──────────────────────────────────────────────── */
    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > ROM

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > ROM

    /* ── Data copied to RAM ───────────────────────────────────────── */
    .data ALIGN(4K) : AT(LOADADDR(.rodata) + SIZEOF(.rodata))
    {
        *(.data*)
        *(.gnu.linkonce.d*)
    } > RAM

    __bss_start = ALIGN(4);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > RAM
    .bss.stack (NOLOAD) : { *(.bss.stack) } > RAM
    __bss_end = ALIGN(4);

    _boot_stack_top = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
    _boot_image_end = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
