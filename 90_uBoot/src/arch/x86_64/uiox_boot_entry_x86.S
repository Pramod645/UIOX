/*
 * uiox_boot_entry_x86.S
 * x86-64 bootloader entry.
 * Multiboot2 header + 32-bit entry + switch to 64-bit long mode.
 */
    .code32
    .section ".multiboot2","a"

    /* Multiboot2 header --------------------------------------- */
MB2_MAGIC  = 0xE85250D6
MB2_ARCH   = 0              /* i386                             */
MB2_LEN    = mb2_end - mb2_start
MB2_CSUM   = -(MB2_MAGIC + MB2_ARCH + MB2_LEN)

mb2_start:
    .long   MB2_MAGIC
    .long   MB2_ARCH
    .long   MB2_LEN
    .long   MB2_CSUM
    /* terminator tag */
    .short  0               /* type=0                           */
    .short  0               /* flags                            */
    .long   8               /* size                             */
mb2_end:

    .section ".text.boot","ax"
    .code32
    .global _start32
    .type   _start32, %function

_start32:
    cli
    /* save multiboot2 info pointer from ebx                    */
    mov     %ebx, %edi

    /* load GDT for 64-bit mode                                 */
    lgdt    gdt64_ptr

    /* enable PAE */
    mov     %cr4, %eax
    or      $0x20, %eax
    mov     %eax, %cr4

    /* set up PML4 identity map in .bss.pgtable                 */
    movl    $pml4, %eax
    movl    $pdp,  %ecx
    or      $0x3,  %ecx       /* present + writable             */
    movl    %ecx,  (%eax)

    movl    $pdp,  %eax
    movl    $pd,   %ecx
    or      $0x3,  %ecx
    movl    %ecx,  (%eax)

    /* map first 2 GB with 2 MB pages */
    movl    $pd,   %eax
    movl    $0,    %ecx
    movl    $512,  %edx
.Lmap_loop:
    movl    %ecx, %esi
    shll    $21,  %esi        /* 2 MB per entry                 */
    orl     $0x83, %esi       /* present + writable + huge      */
    movl    %esi, (%eax)
    addl    $8,   %eax
    inc     %ecx
    cmp     %edx, %ecx
    jl      .Lmap_loop

    /* load PML4 into CR3 */
    movl    $pml4, %eax
    movl    %eax,  %cr3

    /* set EFER.LME */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $0x100, %eax
    wrmsr

    /* enable paging */
    movl    %cr0, %eax
    orl     $0x80000001, %eax
    movl    %eax, %cr0

    /* far jump to 64-bit code segment */
    ljmp    $0x08, $_start64

    .code64
    .global _start64
    .type   _start64, %function

_start64:
    /* load data segment selectors */
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    xorw    %ax, %ax
    movw    %ax, %fs
    movw    %ax, %gs

    /* set up stack */
    leaq    _boot_stack_top(%rip), %rsp

    /* zero BSS */
    leaq    _bss_start(%rip), %rdi
    leaq    _bss_end(%rip),   %rcx
    subq    %rdi, %rcx
    shrq    $3, %rcx
    xorq    %rax, %rax
    rep     stosq

    /* call arch HW init */
    call    uboot_hw_init_x86

    /* call C bootloader main
       rdi = multiboot2 info pointer (saved in edi above)       */
    movq    %rdi, %rdi         /* already in rdi                */
    xorl    %esi, %esi         /* boot flags = 0                */
    call    uiox_boot_main

.Lhang64:
    hlt
    jmp     .Lhang64

    /* -- Minimal GDT ----------------------------------------- */
    .section ".data","aw"
    .align  8
gdt64:
    .quad   0                  /* null                          */
    .quad   0x00AF9A000000FFFF /* code64 ring0                  */
    .quad   0x00AF92000000FFFF /* data64 ring0                  */
gdt64_ptr:
    .short  . - gdt64 - 1
    .long   gdt64

    /* -- Page tables (3 levels, statically allocated) -------- */
    .section ".bss.pgtable","aw"
    .align  4096
pml4: .space  4096
pdp:  .space  4096
pd:   .space  4096

    /* -- Boot stack ------------------------------------------ */
    .section ".bss.boot_stack","aw"
    .align  16
    .space  65536
_boot_stack_top:
