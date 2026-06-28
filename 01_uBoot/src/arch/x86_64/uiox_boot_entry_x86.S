/*
 * uiox_boot_entry_x86.S
 * UIOX Bootloader — x86_64 entry point with Multiboot2 header.
 *
 * QEMU q35: GRUB/Multiboot2 loads us at 1 MB (0x100000) in 32-bit mode.
 * We then set up a minimal identity page table and jump to long mode.
 *
 * Register state on Multiboot2 entry:
 *   eax = 0x36D76289  (Multiboot2 loader magic)
 *   ebx = physical address of Multiboot2 info structure
 */

    .set MB2_MAGIC,         0xE85250D6
    .set MB2_ARCH_I386,     0
    .set MB2_HDR_LEN,       (.mb2_end - .mb2_hdr)
    .set MB2_CHECKSUM,      -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HDR_LEN)

/* =========================================================================
 * Multiboot2 header (must be in first 32 KB of image)
 * ====================================================================== */

    .section ".multiboot", "a"
    .balign 8
.mb2_hdr:
    .long   MB2_MAGIC
    .long   MB2_ARCH_I386
    .long   MB2_HDR_LEN
    .long   MB2_CHECKSUM
    /* End tag */
    .short  0
    .short  0
    .long   8
.mb2_end:


/* =========================================================================
 * 32-bit protected-mode entry
 * ====================================================================== */

    .section ".boot", "ax"
    .code32
    .global _start
_start:
    /* Save Multiboot2 info pointer (ebx) */
    movl    %ebx, %edi

    /* ------------------------------------------------------------------ */
    /* 1. Build a minimal identity-mapped 4-level page table in BSS       */
    /*    PML4[0] → PDPT → PD → 2 MB pages (0–4 GB identity)             */
    /* ------------------------------------------------------------------ */
    movl    $pml4_table, %eax
    /* Zero all four tables (4 × 4096 bytes) */
    movl    $pml4_table, %edi
    /*movl    $(4 * 4096 / 4), %ecx*/
    movl  $4096, %ecx
    xorl    %eax, %eax
    rep stosl
    movl    %ebx, %edi          /* restore Multiboot info ptr */

    /* PML4[0] → PDPT */
    movl    $(pdpt_table + 0x3), pml4_table

    /* PDPT[0] → PD */
    movl    $(pd_table + 0x3), pdpt_table

    /* PD: 512 × 2 MB pages, flags = PS | RW | P */
    movl    $pd_table, %eax
    movl    $512, %ecx
    movl    $0x0083, %edx       /* addr=0, 2MB, RW, P */
.Lpd_fill:
    movl    %edx, (%eax)
    movl    $0x00, 4(%eax)
    addl    $0x200000, %edx     /* next 2 MB page */
    addl    $8, %eax
    loopl   .Lpd_fill

    /* ------------------------------------------------------------------ */
    /* 2. Enable PAE + load CR3                                           */
    /* ------------------------------------------------------------------ */
    movl    $pml4_table, %eax
    movl    %eax, %cr3

    movl    %cr4, %eax
    orl     $(1 << 5), %eax     /* CR4.PAE */
    movl    %eax, %cr4

    /* ------------------------------------------------------------------ */
    /* 3. Set IA32_EFER.LME (Long Mode Enable)                           */
    /* ------------------------------------------------------------------ */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $(1 << 8), %eax     /* LME */
    wrmsr

    /* ------------------------------------------------------------------ */
    /* 4. Enable paging → activates long mode (CR0.PG | CR0.PE)          */
    /* ------------------------------------------------------------------ */
    movl    %cr0, %eax
    orl     $(1 << 31 | 1 << 0), %eax
    movl    %eax, %cr0

    /* Far jump to 64-bit code segment (GDT set up below) */
    ljmp    $0x08, $long_mode_entry


/* =========================================================================
 * Flat 64-bit GDT (needed for the far jump above)
 * ====================================================================== */

    .balign 8
gdt64:
    .quad   0x0000000000000000   /* null descriptor */
    .quad   0x00AF9A000000FFFF   /* 64-bit code, DPL=0 */
    .quad   0x00CF92000000FFFF   /* 64-bit data, DPL=0 */
gdt64_end:

gdt64_ptr:
    .short  (gdt64_end - gdt64 - 1)
    .long   gdt64

/* =========================================================================
 * 64-bit long mode entry
 * ====================================================================== */

    .code64
long_mode_entry:
    /* Reload data segments */
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss

    /* Load GDT (64-bit pointer) */
    lgdt    gdt64_ptr

    /* Set up 64-bit stack */
    leaq    _boot_stack_top(%rip), %rsp

    /* Zero BSS */
    leaq    __bss_start(%rip), %rdi
    leaq    __bss_end(%rip),   %rcx
    subq    %rdi, %rcx
    shrq    $3, %rcx            /* count in qwords */
    xorq    %rax, %rax
    rep stosq

    /* arg0 = Multiboot2 info PA (saved in rdi earlier, now pass as rdi) */
    movq    %rdi, %rdi          /* Multiboot2 info already in rdi        */
    callq   uiox_boot_main

.Lhang64:
    hlt
    jmp     .Lhang64


/* =========================================================================
 * Boot page tables (must be 4096-byte aligned, in BSS)
 * ====================================================================== */

    .section ".bss.pagetables", "aw", @nobits
    .balign 4096
pml4_table: .space 4096
pdpt_table: .space 4096
pd_table:   .space 4096

    .section ".bss.stack", "aw", @nobits
    .balign 16
    .global _boot_stack_base
_boot_stack_base:
    .space  (16 * 1024)
    .global _boot_stack_top
_boot_stack_top:
