/*
//Process state and transition
1. User Ruuning          <--sys Call, Intrrupt-->:2.Kernel Running, <--return user:7.Preempted
2.Kernel Running         <--sys Call, Intrrupt-->:1. User Ruuning, preempt--->:7.Preempted, <-intrrupt, interrupt return->:2.Kernel Running, exit-->:9.Zombie, sleep-->:4.Asleep In Memory, <--reschedule process:3.Ready to Run In Memory
3.Ready to Run In Memory <--swap in, swap out-->:5.Ready to Run, Swapped, reschedule process->:2.Kernel Running , <-enough mem :8.Created, <--wakeup:4.Asleep In Memory
4.Asleep In Memory       <--sleep:2.Kernel Running , wakeup-->:3.Ready to Run In Memory, swap out-->:6.Sleep,Swapped
5.Ready to Run, Swapped  <--swap in, swap out-->:3.Ready to Run In Memory, <--wakeup:6.Sleep,Swapped, <--not enough memory:8.Created
6.Sleep,Swapped          <--swap out:4.Asleep In Memory , wakeup-->:5.Ready to Run, Swapped 
7.Preempted              <--prempt:2.Kernel Running, return to user-->1. User Ruuning 
8.Created                <--fork, enough memory-->:3.Ready to Run In Memory, not enough memory-->5.Ready to Run, Swapped
9.Zombie                 <--exit:2.Kernel Running 

1 . The process is executing in user mode.
2. The process is executing in kernel mode.
3. The process is not executing but is ready to run as soon as the kernel schedules it .
4 . The process is sleeping and resides in main memory.
5. The process is ready to run, but the swapper (process 0) must swap the process into main memory before the kernel can schedule it to execute.
 will reconsider this state in a paging system.
6. The process is sleeping, and the swapper has swapped the process to secondary storage to make room for other processes in main memory.
7 . The process is returning from the kernel to user mode, but the kernel preempts it and does a context switch to schedule another process. The
distinction between this state and state 3 ("ready to run") will be brought out shortly.
8 . The process i s newly created and is in a transition state; the process exists, 
but it is not ready to run, nor is it sleeping. This state is the start state for all processes except process 0.
9 . The process executed the exit system call and is in the zombie state. The process no longer exists, but it leaves a record containing an exit code and
some timing statistics for its · parent process to collect. The zombie state is the final state of a process.

*/
/*
1. Process table: contains fields that must always be accessible to the kernel
2. U area : u area contains fields that need to be accessible only to the running process
// Kernel allocates space for the u area only when creating a process, it does not need u area for process table entires that do not have processes.
*/
/*
struct processTable
{
	state;               // idenfy the process state
	processTableEntry;  // alow kernel to locate the process and its u area inmain memory and this get used for context switch
	processSize;        // kernel knows howmuch space to allocate for the process
	userID;             // determine the verious process priviledges
	processID:          //specify the relationship of process to each other
	eventDescriptor;    // describe the process in in the sleep state 
	schedulingParameters;//allo kernel to determine the order in which processes move to the state,kernel runing and user runing
	enum signal;         // signal sent to process but not handled
	timers;              //itgives process execution time and kernel resources utilization,used for process accounting and for the calculation of process scheduling priority.
	useSetTimer;         // sent an alarmsignal to a process
}
*/
/*
struct uArea //A process can access its u area when it executes in kernel mode but not when it executes in user mode.Because the kernel can access only one u area at a time by
			its virtual address, the u area partially defines the context of the process that is
			running on the system.
{
	ptrToProcessTable;    // it identifies the corresponds to the u area
	userID;               // it determine various privileges allowed the process such as file access right
	timerFileledReconrd;  // it record the time the process spent executing in user mode and in kernelmode
	array[];              // it indicates how the process wished to react to signals
	controlTerminal;      // it identifies the login termina associateed with the process
	error;                // it records error encountered dut=ring a system calculation
	returnValue;          // it contains the result of system calls
	ioParamatares;        // it describe the amount of data to dransfer, the adress of the source or target data array in user space,file offset and so only
	currentDirectoryAndRoot;     // it describe the filesystem envirenment of the process
	userFileDescriptor;   // it have user descriptor table records the files the process have open 
	linimt;               // it restrict the size of a process and the size of a fileit can write
	permissionMode;       // setting on the process creates
}
*/
/*
struct systemMemory
{
	text;               //
	data;               // share memory should be of this section
	stack;               //
}
*/
/*
// system V kernel devides the virtual addess space of a process into logical regions
//A region is a contiguous area of the virtual address space of a process that can be treated as a distinct object to be shared or protected.
//text, data, and stack usually form separate regions of a process.Several processes can share a region.
//Pre Proc Region Tables(Virtual Adresses)          and Regions (physicals addresses)
//The data regions and stack regions of the two processes are private but test is common region among the processes.
struct perProcRegionTable
{
	Text;
	Data;
	Stack;
}
// mapping logical page number to physical page number
struct regions 
{                
	page;         //memory management architecture based on pages, the memory management hardware divides physical memory into a set of equal-sized blocks called pages.Every addressable location in memory
                   is contained in a page and, consequently, every memory location can be addressed by a (page number, byte offset in page)
	pageTable;    //its a physical addresses. Page table entries may also contain machine-dependent information such as permission bits to allow reading or writing of the page. 
					The kernel stores page tables in memory and accesses them like all other kernel data structures.
	regionTable;  //The region table entry contains a pointer to a table of physical page numbers called a page table .
}

*/
/*
struct systemMemory //
{
	addressOfPageTable;   //
	virtualAddress        //
	noOfPagesinPageTable; //
}
*/
/*
union contextOfaProcess //user-level context,register context and system level context
{
	userAddressSpace;       //consists of the process text, data, user stack, and shared memory that occupy the virtual address space of the procesS.
	hardwareRegion;        //
	kernelDataStructure;   //
}
*/
/*
struct registerContext
{
	programCounter;      
	statur Register;
	stackPointer;
	generalPurposeRegistersp[];  //it contain data generated by the process during its execution.
}
*/
/*
struct systemLvlContext
{
	processTableEntry;       //it defines the state of a process
	uArea;                   //it contains process control information that need be accessed only in the context of the process.
	preRegionEntries;        // region tables and page tables, define the mapping from virtual to physical addresses and therefore define the text, data, stack, and other regions of a process.
	kernelStack;             // it contains the stack frames of kernel procedures as. a process executes in kernel mode.
	*systemLvlcontext;       // a process consists of a set of layers, visualized as a last-in-first-out stack
}
*/

/*
1. Algorithm inthand   // handle interrupts 
input: none
output: none
{
		save(push) current context layer;      //It saves the current register context 
		determine interrupt source;
		find interrupt vector;
		call interrupt handler;
		restore(pop) preveous context layer;
}
*/
/*
2. Algorithm syscall  //algorithm for invocation of system call
input: system call number
output: result of system call
{
	find entry in system call table corresponding to system call number;
	determine number of parameters to system call;
	copy parameters from address space to u area;
	save current context for abortive retun;
	invoke system call code in kernel;
	if(error during execution of system call)
	{
		set register 0 in user saved register context to error number;
		trun on carry bit in PS registers in user saved register context;
	}
	else
	{
		set registers 0, 1 in user saved register context to return values from system call;
		
	}
}
*/
/*
//The kernel allocates a new region (algorithm al/ocreg) during fork , exec , and shmget (shared memory) system calls.
3. Algorithm allocreg   //allocate a region data structure
input: inode pointer, region type
output: locked region
{
	remove region from linked list of free regions;
	assign region type;
	assign region inode pointer;
	if(inode pointer not null)
		increment inode reffrence count;
	place region on linked list of active regions;
	return(locked region);
}
*/
/*
struct regionTableEntry
{
	ptrToInode;           // file whose contents were originally loaded into the region
	regionType;           // (text, shared memory, private data or stack)
	sizeOfRegion;         
	locationOfRegion;     // The location of the region in physical memory
	ststuOfRegion;        // which may be a combination of 1.locked(inodes (algorithms iget and iput)) 2.in demand 3.in the process of being loaded into memory 4.valid, loaded into memory
	reffrenceCount;       // giving the number of processes that reference the region.
}
*/
/*
//The kernel attaches a region during the fork , exec , and shmat system calls to connect it to the address space of a process
4. Algorithm attachreg    // attach a region to a process
inputs: pointer to locked region bering attached, process to which region is being attached, virtual address in process where region will be attached, region type
output: per process region table entry
{
	allocate per process region table entry for process;
	initialize per process region table entry:set pointer to region being attached;set type field;set virtual address field;
	check legality of virtual address, region size;
	increment region reference count;
	increment process size according to attached region;
	initialize new hardware register triple for process;
	retum (per process region table entry);
}
*/
/*
//A process may expand or contract its virtual address space with the sbrk system call.
//the stack of a process automatically expands (that is, the process does not make an explicit system call) according to the depth of nested procedure
//calls. Internally, the kernel invokes the algorithm growreg to change the size of a region
5. Algorithm growreg        //change the size of a region
inputs: pointer to per process region table entry, change in size of region (may be positive or negative)
output: none
{
	if (region size increasing)
	{
		check legality of new region size;
		allocate auxiliary tables (page tables) ;
		if (not system supporting demand paging)
		{
			allocate physical memory;
			initialize auxiliary tables, as necessary;
		}
	}
	else    //region size decreasing
	{
		free physical memory, as appropriate;
		free auxiliary tables, as appropriate;
	}
	do (other) initialization of auxiliary tables, as necessary;
	set size field in process table;
}
*/
/*
6. Algorithm loadreg       //load a portion of a file into a region
input: pointer to per process region table entry, virtual address to load region, inode pointer of file for loading, byte offset in file for start of region, byte count for amount of data to load
output: none
{
	increase region size according to eventual size of region(algorithm growreg) ;
	mark region state: being loaded into memory;
	unlock region ;
	set up u a rea parameters for reading file:target virtual address where data is read to,start offset value for reading file,count of bytes to read from file;
	read file into region (internal variant of read algorithm) ;
	lock region;
	mark region state: completely loaded into memory;
	awaken all processes waiting for region to be loaded;
}
*/
/*
//When a region is no longer attached to any processes, the kernel can free the region and return it to the list of free regions
7. Algorithm freereg            //free an allocated region
input: pointer to a. (locked) region)
output: none
{
	if (region reference count non zero)
	{
		// some process still using region 
		release region lock;
		if (region has an associated inode)
			release inode lock;
		return;
	}
	if (region has associated inode)
		release inode (algorithm iput) ;
	free physical memory still associated with region;
	free auxiliary tables associated with region;
	clear region fields;
	place region on region free list;
	unlock region;
}
*/
/*
8. Algorithm detechreg              //detach a region from a process
input:pointer to per process region table entry
output: none
{
	get auxiliary memory management tables for process, release as appropriate;
	decrCilllent process size;
	decrem.ent region reference count;
	if (region reference count is 0 and region not sticky bit)
	{
		free region (algorithm freereg) ;
	}
	else    //either reference count non-0 or region sticky bit on
	{
		free inode lock, if applicable (inode associated with region) ;
		free · region lock;
	}
}
*/
/*
9. Algorithm dupreg          //duplicate an existing region
input: pointer to region table entry
outout: pointer to a region that looks identical to input region
{
	if (region type shared)      // caller will increment region reference count with subsequent attachreg call
		return(input region pointer);
	allocate new region (algorithm allocreg) ;
	set up auxiliary memory management structures, as currently exists in input region;
	allocate physical memory for region contents;
	"copy" region contents from input region to newly allocated region;
	return(pointer to allocated region) ;
}
*/
/*
10. Algorithm sleep 
input: sleep address, priority
output: 1 if process awakened as a result of a signal that process catches,longjump algorithm if process awakened as a result of a signal that is does not catch, 0 otherwise
{
	raise processor execution level to block all interrupts;
	set process state to sleep;
	put process on sleep hash queue, based on sleep address;
	save sleep address in process table slot;
	set process priority level to input priority;
	if (process sleep is NOT interruptible)
	{
		do context switch;  //process resumes execution here when it wakes up
		reset processor priority level to allow interrupts as when process went to sleep;
		return (O) ;
	}
	if (no signal pending against process) // here, process sleep is interruptible by signals
	{
		do context switch;   //process resumes execution here when it wakes up
		if (no signal pending against process)
		{
			reset processor priority level to what it was when process went to sleep;
			return (O) ;
		}
	}
	remove process from sleep hash queue, if still there;
	reset processor priority level to what it was when process went to sleep;
	if (process sleep priority set to catch signals)
		return ( l )
	do longjmp algorithm;
}
*/
/*
11. Algorithm wakeup            //wake up a sleeping process
input: sleep address
output: none
{
	raise processor execution level to block all interrupts;
	find sleep hash queue for sleep address;
	for (every process asleep on sleep address)
	{
		remove process from hash queue;
		mark process state "ready to run";
		put process on scheduler list of processes ready to run;
		clear field in process table entry for sleep address;
		if (process not loaded in memory)
			wake up swapper process (0) ;
		else if (awakened process is more elligible to run than currently running process)
			set scheduler flag;
	}
	restore processor execution level to original level;
}
*/