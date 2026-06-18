/*
 * linker/uiox_boot_arm64.ld
 * UIOX Bootloader — AArch64 linker script.
 *
 * Target   : QEMU virt (ARM64)
 * Boot base: 0x40000000  (DRAM start — QEMU loads kernel/bootloader here)
 * Kernel   : 0x40080000  (512 KB offset — avoids DTB + boot-args region)
 * Args     : 0x40070000  (uiox_boot_args_t)
 * Stack    : grows down from 0x40060000
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

_boot_load_base = 0x40000000;
_kern_load_base = 0x40080000;
_args_base      = 0x40070000;

MEMORY
{
    DRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 64M
}

SECTIONS
{
    . = _boot_load_base;

    /* ── Boot entry (first section loaded) ────────────────────────── */
    .boot ALIGN(4K) :
    {
        _boot_start = .;
        KEEP(*(.boot))
        KEEP(*(.boot.text))
        _boot_end = .;
    } > DRAM

    /* ── Read-only: text + rodata ─────────────────────────────────── */
    .text ALIGN(4K) :
    {
        *(.text.unlikely)
        *(.text*)
        *(.gnu.linkonce.t*)
    } > DRAM

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > DRAM

    /* ── Read-write: data ─────────────────────────────────────────── */
    .data ALIGN(4K) :
    {
        *(.data*)
        *(.gnu.linkonce.d*)
    } > DRAM

    /* ── BSS (zeroed by entry stub) ──────────────────────────────── */
    . = ALIGN(8);
    __bss_start = .;
    .bss :
    {
        *(.bss*)
        *(.gnu.linkonce.b*)
        *(COMMON)
    } > DRAM
    .bss.pagetables (NOLOAD) : { *(.bss.pagetables) } > DRAM
    .bss.stack      (NOLOAD) : { *(.bss.stack)      } > DRAM
    __bss_end = ALIGN(8);

    /* ── Symbols used by C code ───────────────────────────────────── */
    _boot_stack_top  = __bss_end;
    _boot_image_end  = .;

    /* ── Discard unwanted sections ────────────────────────────────── */
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}
