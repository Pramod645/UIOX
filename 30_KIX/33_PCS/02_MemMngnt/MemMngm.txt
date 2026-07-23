/*
//At least part of a process must be contained in
primary memory to run; the CPU cannot execute a process that exists entirely in
secondary memory.However, primary memory is · a precious resource that
frequently cannot contain all active processes in the system.

//The memory management subsystem decides which
processes should reside (at least partially) in main memory, and manages the parts
of the virtual address space of a process that are not core resident. It monitors the
amount of available primary memory and may periodica􀂷y write processes to a
secondary memory device called the swap device to provide 'more space in primary
memory. At a later time, the kernel reads the data from the swap device back to
main memory

//systems transferred entire processes between primary
memory and the swap device, but did not transfer parts of a process independently,
except for shared text. Such a memory management policy is called swapping. It
made sense to implement such a policy on the PDP 1 1 , where the maximum
process size was 64K bytes. For this policy, the size of a process is bounded by the
amount of physical memory available on a system.

*/
/*
//The swap device is a block device in a configurable section of a disk. When:as the
kernel allocates space for files one block at a time, it allocates space on the swap
device in groups of contiguous blocks. Space allocated for files is used statically;
since it will exist for a long time, the allocation scheme is flexible to reduce the
amount of fragmentation and, hence, unallocatable space in the file system. But
the allocation of space on the swap device is transitory, depending on the pattern of
process scheduling. A process that resides on the swap device will eventually
migrate back to main memory, freeing the space it had occupied on the swap
device. Since speed is critical and the system can do I/0 faster in one multiblock
operation than in several single block operations, the kernel allocates contiguous
space on the swap device without regard for fragmentation.

//The
kernel maintains free space for file systems in a linked list of free blocks, accessible
from· the file system super block, but it maintains the free space for the swap device
in an in-core table, called a map . Maps, used for other resources besides the swap
device (some device drivers, for example) , allow a first-fit allocation of contiguous
"blocks" of a resource.


1. Algorithm malloc  // algorithm to alloeate Jllap space
input: map address//indicates which map to use, requested number of units output: address, if successful 0, otherwise
output:
{
	for (every map entry)
	{
		if (current map entry can fit requested units)
		{
			if (requested units == number of units in 'entry)
				delete entry from map;
			else
				adjust start address of entry;
			return (original address of entry) ;
		}
	}
	retum (O) ;
}
*/
/*
2. Algorithm swapper       //swap in swapped out processes,swap out other processes to make room
input: none
output: none
{
	loop:
		for (all swapped out processes that are ready to run)
			pick process swapped out longest;
		if (no such process)
		{
			sleep (event must swap in) ;
			goto loop;
		}
		if (enough room in main memory for process)
		{
			swap process in;
			goto loop;
		}
		//loop2: here .in revised algorithm (see page 285)
		for (all processes loaded in main memory, not zombie and not locked in memory)
		{
			if (there is a sleeping process)
				choose process such that priority + residence time is numerically highest;
			else //no sleeping processes 
				choose process such that residence time + nice is numerically highest;
		}
		if (chosen process not sleeping or residency requirements notsatisfied)
			sleep (event must swap process in) ;
		else
			swap out process;
		goto loop; // goto loop2 in revised algorithm 
}
*/
/*

Demaning paging
*/
/*
//If a process attempts to access a page whose valid bit is not set, it incurs a validity
fault and the kernel invokes the validity fault handler (Figure 9 .2 1 ) . The valid bit
is not set for pages outside the virtual address space of a process, nor is it set for
pages that are part of the virtual address space but do not currently have a physical
page assigned to them. The hardware supplies the kernel with the virtual address
that was accessed to cause the memory fault, and the kernel finds the page table
entry and disk block descriptor for ·the page. The kernel locks the region containing
the page table entry to prevent race conditions that would occur if the page stealer
attempted to swap the page out. If the disk block descriptor has no record of the
faulted page, the attempted memory reference is invalid and the kernel sends a
"segmentation violation" signal to the offending process (recall Figure 7 .25) . This
is the same procedure a swapping system follows when a process accesses an invalid
address, except that it recognizes the error immediately because all legal pages are
memory resident. If the memory reference was legal, the kernel allocates a page of
memory to read in the page contents from the swap device or from the executable
file.
The page that caused the fault is in one of five states:
I . On a swap device and not in memory,
2. On the free page list in memory,
3 . I n a n executable file,
4. Marked "demand zero,"
5. Marked "demand fill."

3. Algorithm vfault         //handler for validity faults
input: address where process faulted
output: none
{
	find region, . page table entry, disk block descriptor corresponding to faulted address, lock region;
	if (addres is outside virtual address space)
	{
		send signal (SIGSEGV: segmentation violation) to process;
		goto out;
	}
	if (address now valid) // process may have slept above 
		goto out;
	if (page in cache)
	{
		remove page from cache;
		adjust page table entry;
		while (page contents not valid) // another proc faulted first 
			sleep (event contents become valid) ;
	}
	else // page not in cache 
	{
		assign new page to region;
		put new page in cache, update pfdata entry;
		if (page not previously loaded and page "demand zero")
			clear assigned page to 0;
		else
		{
			read virtual page from swap dev or exec file;
			sleep (event I/O done) ;
		}
		awaken processes (event page contents valid) ;
	}
	set page valid bit;
	clear page modify bit, page age;
	recalculate process priority;
	out: unlock region; .
}
*/
/*
//The second kind of memory fault that a process can incur is a protection fault,
meaning that the process accessed a valid page but the permission bits associated
with the page did not permit access.  A process also incurs a protection fault
when it attempts to write a page whose copy on write bit was set during the fork
system call. The kernel must determine whether permission was denied because the
page requires a copy on write or whether something truly illegal happened.

4. Algorithm pfault      //protection fault handler
input: address where process faulted
output: none
{
	find region, page table entry, disk block descriptor,page frame for address, lock region;
	if (page not valid in memory)
		goto out;
	if (copy on write bit not set)
		goto out;       //real program error - signal
	if (page frame reference count > 1 )
	{
		allocate a new physical page;
		copy contents of old page to new page;
		decrement old page frame reference count;
		update page table entry to point to new physical page;
	}
	else //steal" page, since nobody else is using it 
	{
		if (copy of page exists on swap device)
			free space on swap device, break page association;
		if (page is on page hash queue)
			remove from hash queue;
	}
	set modify bit, clear copy on write bit in page table entry;
	recalculate process priority;
	check for signals;
	out:unlock region;
}
*/