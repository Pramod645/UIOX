/*
 * linker/uiox_boot_x86_64.ld
 * UIOX Bootloader — x86_64 linker script.
 *
 * Target    : QEMU q35 / bare-metal x86_64 via Multiboot2
 * Load addr : 0x00100000 (1 MB — standard Multiboot2 load address)
 * Stack     : 0x00090000 (below 1 MB, set up in 64-bit entry)
 *
 * Mirrors uiox_x86_64.ld KERNEL region.
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

MEMORY
{
    LOW  (r)   : ORIGIN = 0x00000000, LENGTH = 1M
    KERN (rwx) : ORIGIN = 0x00100000, LENGTH = 63M
}

SECTIONS
{
    . = 0x00100000;

    /* ── Multiboot2 header (must be within first 32 KB) ──────────── */
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    } > KERN

    /* ── Boot entry / 32→64 trampoline ───────────────────────────── */
    .boot :
    {
        _boot_start = .;
        KEEP(*(.boot))
        _boot_end = .;
    } > KERN

    /* ── 64-bit code and data ─────────────────────────────────────── */
    .text ALIGN(4K) :
    {
        *(.text*)
        *(.gnu.linkonce.t*)
    } > KERN

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    } > KERN

    .data ALIGN(4K) :
    {
        *(.data*)
        *(.gnu.linkonce.d*)
    } > KERN

    __bss_start = ALIGN(8);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    } > KERN
    .bss.pagetables (NOLOAD) : { *(.bss.pagetables) ALIGN(4096) } > KERN
    .bss.stack      (NOLOAD) : { *(.bss.stack)                  } > KERN
    __bss_end = ALIGN(8);

    _boot_stack_top = __bss_end;
    _boot_image_end = .;

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
