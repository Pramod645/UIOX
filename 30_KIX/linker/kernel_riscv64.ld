/*
 * 30_KIX/linker/kernel_riscv64.ld
 *
 * UIOX Kernel linker script — RISC-V 64 (RV64IMAFDC).
 *
 * KERNEL_LOAD_ADDR must match uiox_bsp_config_t.kernel_load_pa in
 * 10_BSP/src/uiox_bsp_main.c (dynamic mode) and the address the
 * primary bootloader places the kernel image (static mode).
 *
 * Memory layout (QEMU virt RISC-V — DRAM starts at 0x80000000):
 *   0x80200000  kernel load base  (leaves room for OpenSBI at 0x80000000)
 *               .text.entry   — uiox_kernel_main (must be first)
 *               .text
 *               .rodata
 *               .data
 *               .bss          (NOLOAD, zeroed by bss_zero())
 *               .stack        (NOLOAD, grows down from _stack_top)
 *
 * @version 1.0.0  @date 2026-07-25
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x80200000;
STACK_SIZE       = 0x10000;     /* 64 KiB kernel stack */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point first so the BSP jr branch lands on uiox_kernel_main.
     *------------------------------------------------------------------*/
    .text.entry : {
        KEEP(*(.text.entry))
        KEEP(*(.text.uiox_kernel_main))
    } > DRAM

    .text : {
        *(.text .text.*)
    } > DRAM

    .rodata : {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    } > DRAM

    _text_start = ADDR(.text.entry);
    _text_end   = .;

    .data : {
        *(.data .data.*)
    } > DRAM

    .bss (NOLOAD) : {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DRAM

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 16-byte aligned per RV64 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(16);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(16);
        _stack_top = .;
    } > DRAM

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
