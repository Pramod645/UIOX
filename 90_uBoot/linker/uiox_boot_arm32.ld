OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY { BOOT (rwx) : ORIGIN = 0x00100000, LENGTH = 256K }
SECTIONS {
    . = 0x00100000;
    .text.boot : { KEEP(*(.text.boot)) } > BOOT
    .text : { *(.text .text.*) *(.rodata .rodata.*) } > BOOT
    .data : ALIGN(8) { *(.data .data.*) } > BOOT
    .bss.boot_stack : ALIGN(8) { KEEP(*(.bss.boot_stack)) } > BOOT
    .bss : ALIGN(8) {
        _bss_start = .; *(.bss .bss.*) *(COMMON) _bss_end = .;
    } > BOOT
    /DISCARD/ : { *(.ARM.exidx*) *(.comment) *(.note*) }
}
