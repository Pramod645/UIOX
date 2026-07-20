PAGE_SHIFT
PAGE_SHIFT
PMD_SHIFT
PUD_SHIFT
PGDIR_SHIFT
PTRS_PER_PTE, PTRS_PER_PMD, PTRS_PER_PUD, and PTRS_PER_PGD

Function name Description
pte_user() Reads the User/Supervisor flag
pte_read() Reads the User/Supervisor flag (pages on the 80 × 86 processor cannot
be protected against reading)
pte_write() Reads the Read/Write flag
pte_exec() Reads the User/Supervisor flag (pages on the 80x86 processor cannot be
protected against code execution)


Macro name Description
pgd_index(addr) Yields the index (relative position) of the entry in the Page Global Directory
that maps the linear address addr.

Paging: director
Paging table 
Layout file
Process table

Table 2-8. Page allocation functions
Function name Description
pgd_alloc(mm) Allocates a new Page Global Directory; if PAE is enabled, it also allocates the
three children Page Middle Directories that map the User Mode linear
addresses. The argument mm (the address of a memory descriptor) is ignored
on the 80x86 architecture.
pgd_free( pgd) Releases the Page Global Directory at address pgd; if PAE is enabled, it also
releases the three Page Middle Directories that map the User Mode linear
addresses.