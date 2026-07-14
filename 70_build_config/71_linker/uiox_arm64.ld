/*
 * linker/uiox_x86_64.ld
 * UIOX x86-64 linker script.
 * Target: QEMU q35 / bare-metal x86-64
 * Kernel loads at 1 MB (0x00100000) — classic x86 layout.
 * Higher-half mapped at -2 GB (0xFFFFFFFF80000000) for
 * future long-mode virtual memory mapping.
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* ── Memory layout (physical) ──────────────────────────── */
MEMORY
{
    LOW_MEM  (r)    : ORIGIN = 0x00000000, LENGTH = 1M
    KERNEL   (rwx)  : ORIGIN = 0x00100000, LENGTH = 63M
    STACK    (rw)   : ORIGIN = 0x03F00000, LENGTH = 1M
    DRAM     (rwx)  : ORIGIN = 0x40000000, LENGTH = 64M
}

/* ── Symbols ────────────────────────────────────────────── */
_kernel_phys_base = 0x00100000;
_stack_top        = ORIGIN(STACK) + LENGTH(STACK);
_dram_start       = ORIGIN(DRAM);

SECTIONS
{
    /* ── Real-mode IVT region (not loaded) ─────────────── */
    . = 0x00000000;

    /* ── Kernel entry at 1 MB ──────────────────────────── */
    . = _kernel_phys_base;

    /* ── Multiboot / boot header ───────────────────────── */
    .boot ALIGN(4K) :
    {
        _boot_start = .;
        KEEP(*(.multiboot))
        KEEP(*(.boot))
        KEEP(*(.text.boot))
        _boot_end = .;
    } > KERNEL

    /* ── Kernel text ───────────────────────────────────── */
    .text ALIGN(4K) :
    {
        _text_start = .;
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(16);
        _text_end = .;
    } > KERNEL

    /* ── Read-only data ────────────────────────────────── */
    .rodata ALIGN(4K) :
    {
        _rodata_start = .;
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(8);
        _rodata_end = .;
    } > KERNEL

    /* ── Initialised data ──────────────────────────────── */
    .data ALIGN(4K) :
    {
        _data_start = .;
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        _data_end = .;
    } > KERNEL

    /* ── BSS ────────────────────────────────────────────── */
    .bss ALIGN(4K) (NOLOAD) :
    {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > KERNEL

    /* ── Stack ──────────────────────────────────────────── */
    .stack ALIGN(4K) (NOLOAD) :
    {
        _stack_bottom = .;
        .
    . += LENGTH(STACK);
        _stack_top = .;
    } > STACK

    /* ── IDT (256 × 16-byte gates = 4096 bytes) ─────────── */
    .idt ALIGN(4K) (NOLOAD) :
    {
        KEEP(*(.idt))
        . = ALIGN(4K);
    } > KERNEL

    /* ── GDT ────────────────────────────────────────────── */
    .gdt ALIGN(4K) (NOLOAD) :
    {
        KEEP(*(.gdt))
        . = ALIGN(4K);
    } > KERNEL

    /* ── Page tables ────────────────────────────────────── */
    .pgtable ALIGN(4K) (NOLOAD) :
    {
        KEEP(*(.pgtable))
        . = ALIGN(4K);
    } > KERNEL

    /* ── Debug sections (stripped from image) ───────────── */
    .debug_info     0 : { *(.debug_info)    }
    .debug_abbrev   0 : { *(.debug_abbrev)  }
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_ranges   0 : { *(.debug_ranges)  }
    .debug_str      0 : { *(.debug_str)     }
    .debug_line     0 : { *(.debug_line)    }
    .debug_frame    0 : { *(.debug_frame)   }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.interp)
        *(.gnu.hash)
        *(.dynsym)
        *(.dynstr)
        *(.dynamic)
        *(.got*)
        *(.plt*)
        *(.rel*)
        *(.rela*)
    }
}

/* ── Exported symbols for C startup code ───────────────── */
PROVIDE(_start          = _kernel_phys_base);
PROVIDE(__text_start__  = _text_start);
PROVIDE(__text_end__    = _text_end);
PROVIDE(__bss_start__   = _bss_start);
PROVIDE(__bss_end__     = _bss_end);
PROVIDE(__data_start__  = _data_start);
PROVIDE(__data_end__    = _data_end);
PROVIDE(__stack_top__   = _stack_top);
PROVIDE(__stack_bottom__= _stack_bottom);
PROVIDE(__heap_start__  = _bss_end);
PROVIDE(__heap_end__    = ORIGIN(STACK));
