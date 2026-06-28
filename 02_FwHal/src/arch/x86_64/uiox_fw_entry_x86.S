/*
 * uiox_fw_entry_x86.S
 * UIOX Firmware — x86_64 entry point (Multiboot2, loaded at 0x100000).
 *
 * Note: For firmware loaded after the UIOX bootloader the CPU is
 * already in 64-bit long mode.  If loaded stand-alone via GRUB,
 * Multiboot2 places us in 32-bit protected mode first (same as
 * uiox_boot_entry_x86.S in 90_uBoot).  This stub handles both.
 */

    /* ─── Multiboot2 header ─────────────────────────────────────── */
    .set MB2_MAGIC,  0xE85250D6
    .set MB2_ARCH,   0
    .set MB2_LEN,    (.mb2_end - .mb2_hdr)
    .set MB2_CSUM,   -(MB2_MAGIC + MB2_ARCH + MB2_LEN)

    .section ".fw.multiboot", "a"
    .balign 8
.mb2_hdr:
    .long  MB2_MAGIC
    .long  MB2_ARCH
    .long  MB2_LEN
    .long  MB2_CSUM
    .short 0
    .short 0
    .long  8
.mb2_end:

    /* ─── 32-bit entry (Multiboot2 leaves us here) ──────────────── */
    .section ".fw.entry", "ax"
    .code32
    .global _start

_start:
    cli

    /* Already in long mode (loaded by uiox bootloader)?
     * Check CS: if bit 8 of EFER.LMA is set we arrived in 64-bit. */

    /* Build minimal page tables: PML4 → PDPT → PD (2 MB identity pages) */
    movl    $0x1000, %edi
    xorl    %eax,  %eax
    movl    $3072,  %ecx
    rep stosl

    movl    $(0x2000 | 3), 0x1000   /* PML4[0] → PDPT */
    movl    $(0x3000 | 3), 0x2000   /* PDPT[0] → PD   */

    movl    $0x3000, %edi
    movl    $0x0083, %eax
    movl    $512,    %ecx
.Lpd:
    movl    %eax, (%edi)
    movl    $0,   4(%edi)
    addl    $0x200000, %eax
    addl    $8, %edi
    loop    .Lpd

    /* Enable PAE */
    movl    %cr4, %eax
    orl     $(1 << 5), %eax
    movl    %eax, %cr4

    movl    $0x1000, %eax
    movl    %eax, %cr3

    /* EFER.LME */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $(1 << 8), %eax
    wrmsr

    /* PG + PE */
    movl    %cr0, %eax
    orl     $(1 << 31 | 1), %eax
    movl    %eax, %cr0

    lgdtl   gdt64_ptr
    ljmpl   $0x08, $_start64

    .balign 8
gdt64:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF   /* 64-bit code */
    .quad 0x00CF92000000FFFF   /* 64-bit data */
gdt64_end:
gdt64_ptr:
    .word (gdt64_end - gdt64 - 1)
    .long gdt64

    /* ─── 64-bit entry ──────────────────────────────────────────── */
    .code64
_start64:
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    xorw    %ax, %ax
    movw    %ax, %fs
    movw    %ax, %gs

    leaq    _fw_stack_top(%rip), %rsp

    /* Zero BSS */
    leaq    __fw_bss_start(%rip), %rdi
    leaq    __fw_bss_end(%rip),   %rcx
    subq    %rdi, %rcx
    shrq    $3, %rcx
    xorq    %rax, %rax
    rep stosq

    /* dtb_pa = 0 on x86 (no DTB) */
    xorq    %rdi, %rdi
    callq   uiox_fw_main

.Lhang64:
    hlt
    jmp     .Lhang64

    .section ".bss.fw_stack", "aw", @nobits
    .balign 16
    .global _fw_stack_base
_fw_stack_base:
    .space (16 * 1024)
    .global _fw_stack_top
_fw_stack_top:
