/* 
 * linker/uiox_boot_x86_64.ld 
 * uiox bootloader — x86_64 linker script. 
 * 
 * target : qemu q35 / bare-metal x86_64 via multiboot2 
 * load addr : 0x00100000 (1 mb — standard multiboot2 load address) 
 * stack : 0x00090000 (below 1 mb, set up in 64-bit entry) 
 * 
 * mirrors uiox_x86_64.ld kernel region. 
 */ 

OUTPUT_FORMAT("elf64-x86-64") 
OUTPUT_ARCH(i386:x86-64) 
ENTRY(_start) 

MEMORY 
{ 
    low (r) : ORIGIN = 0x00000000, LENGTH = 1M 
    kern (rwx) : ORIGIN = 0x00100000, LENGTH = 63M 
} 

SECTIONS 
{ 
    . = 0x00100000; 

    /* ── multiboot2 header (must be within first 32 kb) ──────────── */ 
    .multiboot ALIGN(8) : 
    { 
        KEEP(*(.multiboot)) 
    } > kern 

    /* ── boot entry / 32→64 trampoline ───────────────────────────── */ 
    .boot : 
    { 
        _boot_start = .; 
        KEEP(*(.boot)) 
        _boot_end = .; 
    } > kern 

    /* ── 64-bit code and data ─────────────────────────────────────── */ 
    .text ALIGN(4096) : 
    { 
        *(.text*) 
        *(.gnu.linkonce.t*) 
    } > kern 

    .rodata ALIGN(4096) : 
    { 
        *(.rodata*) 
        *(.gnu.linkonce.r*) 
    } > kern 

    .data ALIGN(4096) : 
    { 
        *(.data*) 
        *(.gnu.linkonce.d*) 
    } > kern 

    . = ALIGN(8);
    __bss_start = .; 

    .bss (NOLOAD) : 
    { 
        *(.bss*) 
        *(COMMON) 
    } > kern 

    .bss.pagetables (NOLOAD) : 
    { 
        *(.bss.pagetables) 
        . = ALIGN(4096); 
    } > kern 

    .bss.stack (NOLOAD) : 
    { 
        *(.bss.stack) 
    } > kern 

    . = ALIGN(8);
    __bss_end = .; 
    _boot_stack_top = __bss_end; 
    _boot_image_end = .; 

    /DISCARD/ : 
    { 
        *(.comment) 
        *(.note*) 
        *(.eh_frame*) 
    } 
}
