OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start32)
MEMORY { BOOT(rwx): ORIGIN=0x00100000, LENGTH=512K }
SECTIONS {
    .multiboot2  : ALIGN(8)   { KEEP(*(.multiboot2))     } > BOOT
    .text.boot   :             { KEEP(*(.text.boot))      } > BOOT
    .text        :             { *(.text .text.*)
                                 *(.rodata .rodata.*)     } > BOOT
    .data : ALIGN(16)          { *(.data .data.*)         } > BOOT
    .bss.pgtable  : ALIGN(4096){ KEEP(*(.bss.pgtable))   } > BOOT
    .bss.boot_stack: ALIGN(16) { KEEP(*(.bss.boot_stack)) } > BOOT
    .bss : ALIGN(16){
        _bss_start=.; *(.bss .bss.*) *(COMMON) _bss_end=.;
    } > BOOT
    /DISCARD/: { *(.comment) *(.note*) *(.eh_frame*) *(.got*) }
}
