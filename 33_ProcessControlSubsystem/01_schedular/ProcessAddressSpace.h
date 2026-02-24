#ifndef __PROCESSS_ADDRESS_SPACE__H
#define __PROCESSS_ADDRESS_SPACE__H

/*
kernel function gets dynamic memory in a fairly
straightforward manner by invoking one of a variety of functions: __get_free_pages( )
or alloc_pages() to get pages from the zoned page frame allocator, kmem_cache_
alloc( ) or kmalloc( ) to use the slab allocator for specialized or general-purpose
objects, and vmalloc( ) or vmalloc_32() to get a noncontiguous memory area. If the
request can be satisfied, each of these functions returns a page descriptor address or a
linear address identifying the beginning of the allocated dynamic memory area.

These simple approaches work for two reasons:
• The kernel is the highest-priority component of the operating system. If a kernel
function makes a request for dynamic memory, it must have a valid reason to
issue that request, and there is no point in trying to defer it.
• The kernel trusts itself. All kernel functions are assumed to be error-free, so the
kernel does not need to insert any protection against programming errors.

When allocating memory to User Mode processes, the situation is entirely different:
• Process requests for dynamic memory are considered non-urgent. When a process’s
executable file is loaded, for instance, it is unlikely that the process will
address all the pages of code in the near future. Similarly, when a process
invokes malloc( ) to get additional dynamic memory, it doesn’t mean the process
will soon access all the additional memory obtained. Thus, as a general rule,
the kernel tries to defer allocating dynamic memory to User Mode processes.
• Because user programs cannot be trusted, the kernel must be prepared to catch
all addressing errors caused by processes in User Mode.

*/


#define 
//chapter 20 and 16. 19

/*
system call while creating task.
System call Description
brk( ) Changes the heap size of the process
execve( ) Loads a new executable file, thus changing the process address space
_exit( ) Terminates the current process and destroys its address space
fork( ) Creates a new process, and thus a new address space
mmap( ), mmap2() Creates a memory mapping for a file, thus enlarging the process address space
mremap( ) Expands or shrinks a memory region
remap_file_pages() Creates a non-linear mapping for a file (see Chapter 16)
munmap( ) Destroys a memory mapping for a file, thus contracting the process address space
shmat( ) Attaches a shared memory region
shmdt( ) Detaches a shared memory region
*/

//All information related to the process address space is included in an object called the memory descriptor of type mm_struct


struct mm_struct{
    struct vm_area_struct *mmap //Pointer to the head of the list of memory region objects
    struct rb_root mm_rb //Pointer to the root of the red-black tree of memory region objects
    struct vm_area_struct *mmap_cache //Pointer to the last referenced memory region object
    unsigned long (*)() get_unmapped_area// Method that searches an available linear address interval in the process address space
    void (*)() unmap_area //Method invoked when releasing a linear address interval
    unsigned long mmap_base //Identifies the linear address of the first allocated anonymous memory region or file memory mapping (see the section “Program Segments and Process Memory Regions” in Chapter 20)
    unsigned long free_area_cache //Address from which the kernel will look for a free interval of linear addresses in the process address space
    pgd_t * pgd //Pointer to the Page Global Directory
    atomic_t mm_users //Secondary usage counter 
    atomic_t mm_count //Main usage counter
    int map_count //Number of memory regions
    struct rw_semaphore mmap_sem //Memory regions’ read/write semaphore
    spinlock_t page_table_lock //Memory regions’ and Page Tables’ spin lock
    struct list_head //mmlist Pointers to adjacent elements in the list of memory descriptors
    unsigned long start_code //Initial address of executable code
    unsigned long end_code //Final address of executable code
    unsigned long start_data //Initial address of initialized data
    unsigned long end_data //Final address of initialized data
    unsigned long start_brk //Initial address of the heap
    unsigned long brk //Current final address of the heap
    unsigned long start_stack //Initial address of User Mode stack
    unsigned long arg_start //Initial address of command-line arguments
    unsigned long arg_end //Final address of command-line arguments
    unsigned long env_start //Initial address of environment variables
    unsigned long env_end //Final address of environment variables
    unsigned long rss //Number of page frames allocated to the process
    unsigned long anon_rss //Number of page frames assigned to anonymousmemory mappings
    unsigned long total_vm //Size of the process address space (number of pages)
    unsigned long locked_vm //Number of “locked” pages that cannot be swapped out (see Chapter 17)
    unsigned long shared_vm //Number of pages in shared file memory mappings
    unsigned long exec_vm //Number of pages in executable memory mappings
    unsigned long stack_vm //Number of pages in the User Mode stack
    unsigned long reserved_vm //Number of pages in reserved or special memory regions
    unsigned long def_flags Default access flags of the memory regions
    unsigned long nr_ptes Number of Page Tables of this process
    unsigned long [] saved_auxv Used when starting the execution of an ELF program (see Chapter 20)
    unsigned int dumpable Flag that specifies whether the process can produce a core dump of the memory cpumask_t cpu_vm_mask Bit mask for lazy TLB switches (see Chapter 2)
    mm_context_t context //Pointer to table for architecture-specific information (e.g., LDT’s address in 80x86 platforms)
    unsigned long swap_token_time //When this process will become eligible for having the swap token (see the section “The Swap Token” in Chapter 17)
    char recent_pagein //Flag set if a major Page Fault has recently occurred
    int core_waiters //Number of lightweight processes that are dumping the contents of the process address space to a core file (see the section “Deleting a Process Address Space” later in this chapter)
    struct completion * core_startup_done //Pointer to a completion used when creating a core file (see the section “Completions” in Chapter 5)
    struct completion core_done //Completion used when creating a core file
    rwlock_t ioctx_list_lock //Lock used to protect the list of asynchronous I/O contexts (see Chapter 16)
    struct kioctx * ioctx_list // List of asynchronous I/O contexts (see Chapter 16)
    struct kioctx default_kioctx //Default asynchronous I/O context (see Chapter 16)
    unsigned long hiwater_rss //Maximum number of page frames ever owned by the process
    unsigned long hiwater_vm //Maximum number of pages ever included in the memory regions of the process
}; // All memory descriptors are stored in a doubly linked list. Each descriptor stores the address of the adjacent list items in the mmlist field.

#define Memory Descriptor of Kernel Threads


struct vm_area_struct{
    struct mm_struct * vm_mm //Pointer to the memory descriptor that owns the region.
    unsigned long vm_start //First linear address inside the region.
    unsigned long vm_end //First linear address after the region.
    struct vm_area_struct *vm_next //Next region in the process list.
    pgprot_t vm_page_prot //Access permissions for the page frames of the region.
    unsigned long vm_flags //Flags of the region.
    struct rb_node vm_rb //Data for the red-black tree (see later in this chapter).
    union shared //Links to the data structures used for reverse mapping (see the section “Reverse Mapping for Mapped Pages” in Chapter 17).
    struct list_head anon_vma_node //Pointers for the list of anonymous memory regions (see the section “Reverse Mapping for Anonymous Pages” in Chapter 17).
    struct anon_vma * anon_vma //Pointer to the anon_vma data structure (see the section “Reverse Mapping for Anonymous Pages” in Chapter 17).
    struct vm_operations_struct* vm_ops //Pointer to the methods of the memory region.
    unsigned long vm_pgoff //Offset in mapped file (see Chapter 16). For anonymous pages, it is either zero or equal to vm_start/PAGE_SIZE (see Chapter 17).
    struct file * vm_file //Pointer to the file object of the mapped file, if any.
    void * vm_private_data //Pointer to private data of the memory region.
    unsigned long vm_truncate_count //Used when releasing a linear address interval in a non-linear file memory mapping.

};


//vm_operations_struct data structure, below The methods to act on a memory region
struct vm_operations_struct{
    open //Invoked when the memory region is added to the set of regions owned by a process.
    close //Invoked when the memory region is removed from the set of regions owned by a process.
    nopage //Invoked by the Page Fault exception handler when a process tries to access a page not present in RAM whose linear address belongs to the memory region (see the later section “Page Fault ExceptionHandler”).
    populate //Invoked to set the page table entries corresponding to the linear addresses of the memory region (prefaulting). Mainly used for non-linear file memory mappings.
};




#define Memory Region Data Structures

/*
All the regions owned by a process are linked in a simple list.
Regions appear in the
list in ascending order by memory address; however, successive regions can be separated
by an area of unused memory addresses. The vm_next field of each vm_area_
struct element points to the next element in the list. The kernel finds the memory
regions through the mmap field of the process memory descriptor, which points to the
first memory region descriptor in the list.
The map_count field of the memory descriptor contains the number of regions
owned by the process. By default, a process may own up to 65,536 different memory
regions; however, the system administrator may change this limit by writing in
the /proc/sys/vm/max_map_count file.
*/



#define Memory Region Access Rights
/*
We have already discussed two kinds of flags associated with a page:
• A few flags such as Read/Write, Present, or User/Supervisor stored in each Page
Table entry (see the section “Regular Paging” in Chapter 2).
• A set of flags stored in the flags field of each page descriptor (see the section
“Page Frame Management” in Chapter 8).
*/

/*
We now introduce a third kind of flag: those associated with the pages of a memory
region. They are stored in the vm_flags field of the vm_area_struct descriptor
*/

enum {
    VM_READ //Pages can be read
    VM_WRITE //Pages can be written
    VM_EXEC //Pages can be executed
    VM_SHARED //Pages can be shared by several processes
    VM_MAYREAD //VM_READ flag may be set
    VM_MAYWRITE //VM_WRITE flag may be set
    VM_MAYEXEC //VM_EXEC flag may be set
    VM_MAYSHARE //VM_SHARE flag may be set
    VM_GROWSDOWN //The region can expand toward lower addresses
    VM_GROWSUP //The region can expand toward higher addresses
    VM_SHM //The region is used for IPC’s shared memory
    VM_DENYWRITE //The region maps a file that cannot be opened for writing
    VM_EXECUTABLE //The region maps an executable file
    VM_LOCKED //Pages in the region are locked and cannot be swapped out
    VM_IO //The region maps the I/O address space of a device
    VM_SEQ_READ //The application accesses the pages sequentially
    VM_RAND_READ //The application accesses the pages in a truly random order
    VM_DONTCOPY //Do not copy the region when forking a new process
    VM_DONTEXPAND // Forbid region expansion through mremap() system call
    VM_RESERVED ///The region is special (for instance, it maps the I/O address space of a device), so its pages must not be swapped out
    VM_ACCOUNT //Check whether there is enough free memory for the mapping when creating an IPC shared memory region (see Chapter 19)
    VM_HUGETLB //The pages in the region are handled through the extended paging mechanism (see the section “Extended Paging” in Chapter 2)
    VM_NONLINEAR //The region implements a non-linear file mapping
}





#define Memory Region Handling

do_mmap( );
do_munmap( );
find_vma( );
find_vma_prev( );
find_vma( );
find_vma_prepare();//
find_vma_intersection( );
get_unmapped_area();
insert_vm_struct( );
vma_link();
__vma_unlink();


#define Allocating a Linear Address Interval
do_mmap( );
do_mmap_pgoff( );
get_unmapped_area();
calc_vm_prot_bits();
find_vma_prepare();
do_mmap_pgoff();
do_munmap();
security_vm_enough_memory( );
vma_merge();
kmem_cache_alloc( );//vm_area_struct data structure for the new memory region by this funtion
hmem_zero_setup();
vma_link();
make_pages_present( );
get_user_pages();
follow_page();
get_user_pages();
handle_mm_fault();


#define Releasing a Linear Address Interval
do_munmap();
split_vma();
split_vma();
unmap_region();
split_vma();
kmem_cache_alloc();
unmap_region();





#define Page Fault Exception Handler
/*
Page Fault exception handler must distinguish exceptions
caused by programming errors from those caused by a reference to a page that
legitimately belongs to the process address space but simply hasn’t been allocated yet.

The memory region descriptors allow the exception handler to perform its job quite
efficiently.
*/

do_page_fault( );




#define Handling a Faulty Address Outside the Address Space
/*
If address does not belong to the process address space, do_page_fault( ) proceeds to
execute the statements at the label bad_area. If the error occurred in User Mode, it
sends a SIGSEGV signal to current (see the section “Generating a Signal” in
Chapter 11) and terminates:
*/


#define Handling a Faulty Address Inside the Address Space
/*
If address belongs to the process address space, do_page_fault( ) proceeds to the
statement labeled good_area:
*/


#define Demand Paging
/*
The term demand paging denotes a dynamic memory allocation technique that consists
of deferring page frame allocation until the last possible moment—until the process
attempts to address a page that is not present in RAM, thus causing a Page Fault
exception.
*/

handle_pte_fault( );
do_no_page( );
do_anonymous_page( );
alloc_page();
lru_cache_add_active();

#define Copy On Write
/*

*/



#define Handling Noncontiguous Memory Area Accesses




#define Creating a Process Address Space
/*
The clone( ), fork( ), and vfork( ) System Calls” in Chapter 3, we mentioned
that the kernel invokes the copy_mm( ) function while creating a new process.
*/
copy_mm( );
clone( );
pgd_alloc( );
init_new_context( );



#define Deleting a Process Address Space
exit_mm( );


#define Managing the Heap
malloc(size);
calloc(n,size);
realloc(ptr,size);
free(addr);
brk(addr);
sbrk(incr);
sys_brk(addr);





#endif // end of __PROCESSS_ADDRESS_SPACE__H
