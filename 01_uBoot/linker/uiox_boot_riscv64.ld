/*
 * linker/uiox_boot_riscv64.ld
 * UIOX Bootloader — RISC-V RV64GC linker script.
 *
 * Fixed (2026-07-12):
 *   W8: "LOAD segment with RWX permissions"
 *       Added explicit PHDRS to split code (RX) from data/BSS/stack (RW).
 *       This gives two LOAD segments:
 *         PT_LOAD [RX]: .boot + .text + .rodata  (no W → not writable)
 *         PT_LOAD [RW]: .data + .bss + .stack    (no X → not executable)
 *       The linker no longer needs to merge all permissions into one RWX
 *       segment.
 *
 * Target    : QEMU -machine virt (riscv64), OpenSBI FW_JUMP
 * Boot base : 0x80200000  (OpenSBI jumps here in S-mode)
 * Kernel    : 0x80280000  (512 KiB offset)
 * Boot args : 0x80270000
 */

OUTPUT_FORMAT("elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

_boot_load_base = 0x80200000;
_kern_load_base = 0x80280000;
_args_base      = 0x80270000;

/* ── Explicit program headers ──────────────────────────────────────────
 * text_seg: covers boot entry + code + read-only data  → R + X, no W
 * data_seg: covers writable data + BSS + stack          → R + W, no X
 * This is what eliminates the RWX warning.
 * ─────────────────────────────────────────────────────────────────────*/
PHDRS
{
    text_seg PT_LOAD FLAGS(5);   /* PF_R | PF_X = 0x4 | 0x1 = 5  */
    data_seg PT_LOAD FLAGS(6);   /* PF_R | PF_W = 0x4 | 0x2 = 6  */
}

MEMORY
{
    DRAM (rwx) : ORIGIN = 0x80200000, LENGTH = 64M
}

SECTIONS
{
    . = _boot_load_base;

    /* ── Boot entry (must be first — _start at load address) ──────── */
    .boot ALIGN(4K) :
    {
        _boot_start = .;
        KEEP(*(.boot))
        KEEP(*(.boot.text))
        _boot_end = .;
    } > DRAM :text_seg

    /* ── Code ─────────────────────────────────────────────────────── */
    .text ALIGN(4K) :
    {
        *(.text.unlikely)
        *(.text*)
        *(.gnu.linkonce.t*)
    } > DRAM :text_seg

    /* ── Read-only data ───────────────────────────────────────────── */
    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > DRAM :text_seg

    /* ── Writable data — switch to data_seg (RW, no X) ───────────── */
    .data ALIGN(4K) :
    {
        *(.data*)
        *(.gnu.linkonce.d*)
        *(.sdata*)           /* RISC-V GP-relative small data section */
        *(.gnu.linkonce.s*)
    } > DRAM :data_seg

    /* ── BSS ──────────────────────────────────────────────────────── */
    . = ALIGN(8);
    __bss_start = .;

    .bss (NOLOAD) :
    {
        *(.bss*)
        *(.sbss*)            /* RISC-V GP-relative small BSS          */
        *(COMMON)
        *(.gnu.linkonce.b*)
    } > DRAM :data_seg

    /* ── Boot stack ───────────────────────────────────────────────── */
    .bss.stack (NOLOAD) :
    {
        *(.bss.stack)
    } > DRAM :data_seg

    __bss_end = ALIGN(8);

    /* ── Symbols used by entry .S and C code ─────────────────────── */
    _boot_stack_top  = __bss_end;
    _boot_image_end  = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.riscv.attributes)
    }
}
