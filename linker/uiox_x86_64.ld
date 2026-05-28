/*
 * linker/uiox_arm64.ld
 * UIOX AArch64 linker script.
 * Target: QEMU virt machine
 * DRAM base: 0x40000000  (1 GB physical)
 * Stack top: 0x44000000  (64 MB above DRAM base)
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

/* ── Memory layout ─────────────────────────────────────── */
MEMORY
{
    ROM   (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
    DRAM  (rwx) : ORIGIN = 0x40000000, LENGTH = 64M
    STACK (rw)  : ORIGIN = 0x43F00000, LENGTH = 1M
}

/* ── Symbols ────────────────────────────────────────────── */
_stack_top   = ORIGIN(STACK) + LENGTH(STACK);
_dram_start  = ORIGIN(DRAM);
_dram_end    = ORIGIN(DRAM) + LENGTH(DRAM);

SECTIONS
{
    /* ── Vector table must be at base of DRAM (VBAR_EL1) ── */
    .vectors ORIGIN(DRAM) :
    {
        KEEP(*(.vectors))
    } > DRAM

    /* ── Kernel text ───────────────────────────────────── */
    .text :
    {
        _text_start = .;
        KEEP(*(.text.boot))       /* boot / reset entry      */
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(16);
        _text_end = .;
    } > DRAM

    /* ── Read-only data ────────────────────────────────── */
    .rodata :
    {
        _rodata_start = .;
        *(.rodata .rodata.*)
        . = ALIGN(8);
        _rodata_end = .;
    } > DRAM

    /* ── Initialised data ──────────────────────────────── */
    .data :
    {
        _data_start = .;
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        _data_end = .;
    } > DRAM

    /* ── BSS (zero-initialised) ────────────────────────── */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DRAM

    /* ── Stack ──────────────────────────────────────────── */
    .stack (NOLOAD) :
    {
        _stack_bottom = .;
        . += LENGTH(STACK);
        _stack_top = .;
    } > STACK

    /* ── Debug sections (not loaded) ───────────────────── */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
