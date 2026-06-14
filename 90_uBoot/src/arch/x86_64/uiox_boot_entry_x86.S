/*
 * uiox_boot_entry_x86.S  —  x86-64 Multiboot2 + 32→64-bit transition.
 */
    .code32
    .section ".multiboot2","a"
MB2_MAGIC  = 0xE85250D6
MB2_ARCH   = 0
MB2_LEN    = mb2_end - mb2_start
MB2_CSUM   = -(MB2_MAGIC + MB2_ARCH + MB2_LEN)
mb2_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long MB2_LEN
    .long MB2_CSUM
    .short 0; .short 0; .long 8   /* terminator tag */
mb2_end:

    .section ".text.boot","ax"
    .code32
    .global _start32
    .type   _start32,%function
_start32:
    cli
    mov     %ebx, %edi   /* save multiboot2 info ptr */

    lgdt    gdt64_ptr

    /* enable PAE */
    mov     %cr4,%eax; or $0x20,%eax; mov %eax,%cr4

    /* identity-map first 2 GB with 2 MB pages */
    movl    $pml4,%eax; movl $pdp,%ecx; or $3,%ecx; movl %ecx,(%eax)
    movl    $pdp, %eax; movl $pd,  %ecx; or $3,%ecx; movl %ecx,(%eax)
    movl    $pd,  %eax; xorl %ecx,%ecx; movl $512,%edx
.Lmap: movl %ecx,%esi; shll $21,%esi; orl $0x83,%esi
    movl %esi,(%eax); addl $8,%eax; inc %ecx; cmp %edx,%ecx; jl .Lmap

    movl $pml4,%eax; movl %eax,%cr3

    /* EFER.LME */
    movl $0xC0000080,%ecx; rdmsr; or $0x100,%eax; wrmsr

    /* enable paging */
    movl %cr0,%eax; or $0x80000001,%eax; movl %eax,%cr0

    ljmp $0x08,$_start64

    .code64
    .global _start64
_start64:
    movw $0x10,%ax
    movw %ax,%ds; movw %ax,%es; movw %ax,%ss
    xorw %ax,%ax; movw %ax,%fs; movw %ax,%gs

    leaq _boot_stack_top(%rip),%rsp

    /* zero BSS */
    leaq _bss_start(%rip),%rdi
    leaq _bss_end(%rip),%rcx
    subq %rdi,%rcx; shrq $3,%rcx
    xorq %rax,%rax; rep stosq

    call uboot_hw_init_x86

    xorl %esi,%esi
    call uiox_boot_main

.Lhang64: hlt; jmp .Lhang64

    .section ".data","aw"
    .align 8
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF  /* code64 ring0 */
    .quad 0x00AF92000000FFFF  /* data64 ring0 */
gdt64_ptr:
    .short . - gdt64 - 1
    .long gdt64

    .section ".bss.pgtable","aw"
    .align 4096
pml4: .space 4096
pdp:  .space 4096
pd:   .space 4096

    .section ".bss.boot_stack","aw"
    .align 16
    .space 65536
_boot_stack_top:
