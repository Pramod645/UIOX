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
 * @version 1.1.0  @date 2026-10-25
 *
 * FIX v1.1.0 — explicit PHDRS, same as kernel_arm64.ld / arm32.  Drop the
 * RWE merge; code R+X, rodata R, data/bss/stack RW.
 */

ENTRY(uiox_kernel_main)

KERNEL_LOAD_ADDR = 0x80200000;
STACK_SIZE       = 0x10000;     /* 64 KiB kernel stack */
MAXPAGESIZE      = 0x1000;      /* 4 KiB — segment alignment */

MEMORY
{
    DRAM (rwx) : ORIGIN = KERNEL_LOAD_ADDR, LENGTH = 64M
}

PHDRS
{
    text   PT_LOAD FLAGS(5);    /* R E — code only        */
    rodata PT_LOAD FLAGS(4);    /* R   — read-only data   */
    data   PT_LOAD FLAGS(6);    /* RW  — data, bss, stack */
}

SECTIONS
{
    . = KERNEL_LOAD_ADDR;

    /*------------------------------------------------------------------
     * Entry point first so the BSP jr branch lands on uiox_kernel_main.
     *------------------------------------------------------------------*/
    .text.entry : ALIGN(MAXPAGESIZE) {
        KEEP(*(.text.entry))
        KEEP(*(.text.uiox_kernel_main))
    } > DRAM :text

    .text : ALIGN(16) {
        *(.text .text.*)
    } > DRAM :text

    .rodata : ALIGN(MAXPAGESIZE) {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    } > DRAM :rodata

    _text_start = ADDR(.text.entry);
    _text_end   = .;

    .data : ALIGN(MAXPAGESIZE) {
        *(.data .data.*)
    } > DRAM :data

    .bss (NOLOAD) : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DRAM :data

    /*------------------------------------------------------------------
     * Kernel stack — grows downward from _stack_top.
     * 16-byte aligned per RV64 ABI.
     *------------------------------------------------------------------*/
    . = ALIGN(16);
    .stack (NOLOAD) : {
        . += STACK_SIZE;
        . = ALIGN(16);
        _stack_top = .;
    } > DRAM :data

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
    /DISCARD/ : { *(.eh_frame*) }
}
