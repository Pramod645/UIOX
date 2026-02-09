


#define BIOS ADDRESS
#define reserved 
#define KERNEL



machine_specific_memory_setup(){
    Table 2-10. Variables describing the kernel’s physical memory layout
Variable name Description
num_physpages Page frame number of the highest usable page frame
totalram_pages Total number of usable page frames
min_low_pfn Page frame number of the first usable page frame after the kernel image in RAM
max_pfn Page frame number of the last usable page frame
max_low_pfn Page frame number of the last page frame directly mapped by the kernel (low memory)
totalhigh_pages Total number of page frames not directly mapped by the kernel (high memory)
highstart_pfn Page frame number of the first page frame not directly mapped by the kernel
highend_pfn Page frame number of the last page frame not directly mapped by the kernel
}

The symbol _text, which corresponds to physical address 0x00100000, denotes the
address of the first byte of kernel code. The end of the kernel code is similarly identified
by the symbol _etext. Kernel data is divided into two groups: initialized and
uninitialized. The initialized data starts right after _etext and ends at _edata. The
uninitialized data follows and ends up at _end.

Unavailable page frames
Available page frames
Kernel code
Initialized kernel data
Uninitialized kernel data

#define Process Page Tables


Linear addresses from 0x00000000 to 0xbfffffff can be addressed when the process
runs in either User or Kernel Mode.

Linear addresses from 0xc0000000 to 0xffffffff can be addressed only when the
process runs in Kernel Mode.


#define Kernel Page Tables
master kernel Page Global Directory
right after the kernel image is loaded into memory, the CPU is still
running in real mode; thus, paging is not enabled.

first phase, the kernel creates a limited address space including the kernel’s
code and data segments, the initial Page Tables, and 128 KB for some dynamic data
structures. This minimal address space is just large enough to install the kernel in
RAM and to initialize its core data structures.
In the second phase, the kernel takes advantage of all of the existing RAM and sets
up the page tables properly.

#define Provisional kernel Page Tables
A provisional Page Global Directory is initialized statically during kernel compilation,
while the provisional Page Tables are initialized by the startup_32() assembly language function defined in arch/i386/kernel/head.S.

swapper_pg_dir variable

The startup_32( ) assembly language function also enables the paging unit. This is
achieved by loading the physical address of swapper_pg_dir into the cr3 control register
and by setting the PG flag of the cr0 control register, as shown in the following
equivalent code fragment:
movl $swapper_pg_dir-0xc0000000,%eax
movl %eax,%cr3 /* set the page table pointer.. */
movl %cr0,%eax
orl $0x80000000,%eax
movl %eax,%cr0 /* ..and set paging (PG) bit */

The _ _pa macro is used to convert a linear address starting from PAGE_OFFSET to the
corresponding physical address, while the _ _va macro does the reverse.

The swapper_pg_dir Page Global Directory is reinitialized by a cycle equivalent to the
following:
pgd = swapper_pg_dir + pgd_index(PAGE_OFFSET); /* 768 */
phys_addr = 0x00000000;
while (phys_addr < (max_low_pfn * PAGE_SIZE)) {
pmd = one_md_table_init(pgd); /* returns pgd itself */
set_pmd(pmd, _ _pmd(phys_addr | pgprot_val(_ _pgprot(0x1e3))));
/* 0x1e3 == Present, Accessed, Dirty, Read/Write,
Page Size, Global */
phys_addr += PTRS_PER_PTE * PAGE_SIZE; /* 0x400000 */
++pgd;
}

#define Fix-Mapped Linear Addresses


#define Handling the Hardware Cache and the TLB

L1_CACHE_BYTES macro yields the size of a cache line in bytes

To optimize the cache hit rate, the kernel considers the architecture in making the
following decisions.
• The most frequently used fields of a data structure are placed at the low offset
within the data structure, so they can be cached in the same line.
• When allocating a large set of data structures, the kernel tries to store each of
them in memory in such a way that all cache lines are used uniformly.


#define Handling the TLB(Translation Lookaside Buffers)
Handling the TLB
Processors cannot synchronize their own TLB cache automatically because it is the
kernel, and not the hardware, that decides when a mapping between a linear and a
physical address is no longer valid.

offers several TLB flush methods that should be applied appropriately,
depending on the type of page table change

Method name Description Typically used when
flush_tlb_all Flushes all TLB entries (including those that
refer to global pages, that is, pages whose
Global flag is set)
Changing the kernel page table
entries
flush_tlb_kernel_range Flushes all TLB entries in a given range of
linear addresses (including those that refer to
global pages)
Changing a range of kernel page
table entries
flush_tlb Flushes all TLB entries of the non-global
pages owned by the current process
Performing a process switch
flush_tlb_mm Flushes all TLB entries of the non-global
pages owned by a given process
Forking a new process
flush_tlb_range Flushes the TLB entries corresponding to a
linear address interval of a given process
Releasing a linear address interval
of a process
flush_tlb_pgtables Flushes the TLB entries of a given contiguous
subset of page tables of a given process
Releasing some page tables of a
process
flush_tlb_page Flushes the TLB of a single Page Table entry of
a given process
Processing a Page Fault


Despite the rich set of TLB methods offered by the generic Linux kernel, every microprocessor
usually offers a far more restricted set of TLB-invalidating assembly language
instructions.


Macro name Description Used by
_ _flush_tlb() Rewrites cr3 register back into itself flush_tlb,
flush_tlb_mm,
flush_tlb_range
_ _flush_tlb_global() Disables global pages by clearing the PGE flag
of cr4, rewrites cr3 register back into itself,
and sets again the PGE flag
flush_tlb_all,
flush_tlb_kernel_range
_ _flush_tlb_single(addr) Executes invlpg assembly language
instruction with parameter addr
flush_tlb_page









