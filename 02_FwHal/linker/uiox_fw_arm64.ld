/*
 * linker/uiox_fw_arm64.ld
 * UIOX Firmware — AArch64 linker script.
 * Firmware image loaded at 0x40000000 (DRAM base, QEMU virt).
 * Bootloader kernel loads at 0x40080000 — firmware stays below that.
 */
/*
 * linker/uiox_fw_arm64.ld
 * UIOX Firmware — AArch64 linker script.
 * Fixed: PHDRS declared correctly, sections use proper AT() syntax.
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

/* ── PHDRS must be declared BEFORE MEMORY and SECTIONS ────────── */
PHDRS
{
    fw_text  PT_LOAD FLAGS(0x5);   /* R + X  (read + execute)          */
    fw_data  PT_LOAD FLAGS(0x6);   /* R + W  (read + write)            */
}

MEMORY
{
    DRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 64M
}

SECTIONS
{
    . = 0x40000000;

    /* ── Entry stub (must be first — contains _start) ──────────── */
    .fw.entry ALIGN(4K) :
    {
        _fw_start = .;
        KEEP(*(.fw.entry))
    } > DRAM  :fw_text         /* assign to phdr fw_text */

    /* ── Kernel code ────────────────────────────────────────────── */
    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > DRAM  :fw_text

    /* ── Read-only data ─────────────────────────────────────────── */
    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > DRAM  :fw_text         /* RO still in the RX segment */

    /* ── Initialised data ───────────────────────────────────────── */
    .data ALIGN(4K) :
    {
        *(.data*)
        *(.gnu.linkonce.d*)
    } > DRAM  :fw_data         /* assign to phdr fw_data */

    /* ── BSS (zero-initialised) ─────────────────────────────────── */
    __fw_bss_start = ALIGN(8);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > DRAM  :fw_data

    .bss.fw_stack (NOLOAD) : { *(.bss.fw_stack) } > DRAM  :fw_data
    __fw_bss_end  = ALIGN(8);

    _fw_stack_top = __fw_bss_end;
    _fw_end       = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
