OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
MEMORY { BOOT(rwx): ORIGIN=0x40000000, LENGTH=512K }
SECTIONS {
    .text.boot      : { KEEP(*(.text.boot))    } > BOOT
    .text           : { *(.text .text.*)
                        *(.rodata .rodata.*)    } > BOOT
    .data : ALIGN(16){ *(.data .data.*)        } > BOOT
    .bss.pgtable  : ALIGN(4096){ KEEP(*(.bss.pgtable))   } > BOOT
    .bss.boot_stack: ALIGN(16) { KEEP(*(.bss.boot_stack)) } > BOOT
    .bss : ALIGN(16){
        _bss_start=.; *(.bss .bss.*) *(COMMON) _bss_end=.;
    } > BOOT
    /DISCARD/: { *(.comment) *(.note*) *(.eh_frame*) }
}
PROVIDE(_bss_start=_bss_start); PROVIDE(_bss_end=_bss_end);
