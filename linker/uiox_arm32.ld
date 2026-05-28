/*
 * linker/uiox_arm32.ld
 * UIOX ARM32 linker script.
 * Target: QEMU versatilepb
 * ROM  base: 0x00000000
 * RAM  base: 0x00100000  (1 MB offset)
 * Stack top: 0x01000000  (16 MB)
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

/* ── Memory layout ─────────────────────────────────────── */
MEMORY
{
    ROM   (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
    RAM   (rwx) : ORIGIN = 0x00100000, LENGTH = 15M
    STACK (rw)  : ORIGIN = 0x00F00000, LENGTH = 1M
}

/* ── Symbols ────────────────────────────────────────────── */
_stack_top  = ORIGIN(STACK) + LENGTH(STACK);
_ram_start  = ORIGIN(RAM);
_ram_end    = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* ── Exception vectors (must be at 0x00000000) ──────── */
    .vectors ORIGIN(ROM) :
    {
        KEEP(*(.vectors))
    } > ROM

    /* ── Kernel text ───────────────────────────────────── */
    .text :
    {
        _text_start = .;
        KEEP(*(.text.boot))
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(4);
        _text_end = .;
    } > RAM

    /* ── Read-only data ────────────────────────────────── */
    .rodata :
    {
        _rodata_start = .;
        *(.rodata .rodata.*)
        . = ALIGN(4);
        _rodata_end = .;
    } > RAM

    /* ── Initialised data ──────────────────────────────── */
    .data :
    {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        _data_end = .;
    } > RAM

    /* ── BSS ────────────────────────────────────────────── */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    /* ── Stack ──────────────────────────────────────────── */
    .stack (NOLOAD) :
    {
        _stack_bottom = .;
        . += LENGTH(STACK);
        _stack_top = .;
    } > STACK

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.comment)
        *(.note*)
    }
}
