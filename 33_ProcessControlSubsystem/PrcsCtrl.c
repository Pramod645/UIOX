/*
The fork system call creates a new process, the exit call terminates process execution, and the wait call allows a parent process
to synchronize its execution with the exit of a child process. Signals inform processes of asynchronous events. Because the kernel synchronizes execution of
exit and wait via signals, the chapter presents signals before exit and wait . The exec system call allows a process to invoke a .. new" program, overlaying its address space with the executable image of a file. The brk system call allows a process to allocate more memory dynamically; similarly, the system allows the user stack to grow dynamically by allocating more space when necessary, using the same mechanisms as for brk .

//Almost all calls use sleep and wakeup
//exec interacts with the file system algorithms

1.System Calls dealing with memory Managment
2.System calls Dealing with Synchronization
3.Miscellaneous

System Calls dealing with memory Managment
1. fork:   a.dupreg, b.attachreg
2. exec:   a.detechreg, b.allocreg, c. attachreg, d.growreg, e.loadreg, f.mapreg
3. brk:    a.growreg
4. exit:   a.detechreg 

System calls Dealing with Synchronization
5. wait:
6. signal
7. kill

Miscellaneous
8. setpgrp
9. setuid
*/

/*
1. Algorithm fork
input: none
output: to parent process, child PID number to child process, 0
{
	check for available kernel resources;
	get free proc table slot, unique PID number;
	check that user not running too many processes;
	mark child state "being created;"
	copy data from parent proc table slot to new child slot;
	increment counts on current directory inode and changed root (if applicable) ;
	increment open file counts in file table;
	make copy of parent context (u area, text, data, stack) in memory;
	push dummy system level context layer onto child system level context; 
		dummy context contains data allowing child process to recognize itself, and start running from here when scheduled;
	if (executing process is parent process)
	{
		change child state to "ready to run;"
		return (child ID) ;                 // from system to user
	}
	else //executing process is the child process
	{
		initialize u area timing fields;
		return (O)                       //to user 
	}
}
*/
/*
//Signals inform processes of the occurrence of asynchronous events
//Processes may send each other signals with the kill system call, or the kernel may send signals internally. There are 19 signals in the System V UNIX system that can be classified as follows
1.Signals having to do with the termination of a process, sent when a process exits or when a process invokes the signal system call with the death of child parameter;
2.Signals having to do with process induced exceptions such as when a process accesses an address outside its virtual address space, when it attempts to write
memory that is read-only (such as program text) , or when it executes a privileged instruction or for various hardware errors;
3.Signals having to do with the unrecoverable conditions during a system call, such as running out of system resources during exec after the original address
space has been released
4.Signals caused by an unexpected error condition during a system call, such as making a nonexistent system call  , writing a pipe that has no reader
processes, or using an illegal "reference" value for the /seek system call. It would be more consistent to return an error on such system calls instead of
generating a signal, but the use of signals to abort misbehaving processes is more pragmatic;
5.Signals originating from a process in user mode, such as When a process wishes to receive an alarm signal after_ a period of time, 0( when processes send
arbitrary signals to each other with the kill system call;
6.Signals related to terminal interaction such as when a user hangs up a terminal or when a user presses the "break" or "delete" keys on a terminal keyboard;
7.Signals for tracing execution of a process.

enum signal
{
	1.sys call,interrupt:2;
	2.retun;                  // check and Handle Signls
	3.retun to user;          // check and Handle Signls
	4.interupt,interrupt retun;
	5.exit;
	6.preempt;
	7.sleep;              // check for Signals
	8.reschedule process; // check for Signals
	9.preempted;
	10.wakeup:2;
	11.swap out:2;
	12.swap in:2;
	13.not enought memory;
	14.enough memory;
	15.fork;
	16.
	17.
	18.
	19.
}


2. Algorithm issig             //test for receipt of signals
input: none
output: true, if process received signals that it does not ignore false otherwise
{
	while (received signal field in process table entry not 0)
	{
		find a signal number sent to the process;
		if (signal is death of child)
		{
			if (ignoring death of child signals)
				free process table entries of zombie children;
			else if (catching death of child signals)
				return (true) ;
		}
		else if (not ignoring signal)
			return (true) ;
		turn off signal bit in received signal field in process table;
	}
	return (false) ;
}
*/
/*
3. Algorithm psig         //handle signals after recognizing their existence
input: none
output: none
{
	get signal number set in process table entry;
	clear signal number in process table entry;
	if (user had called signal sys call to ignore this signal)
		return; // done 
	if (user specified function to handle the signal)
	{
		get user virtual address of signal catcher stored in u area; //the next statement has undesirable side-effects
		clear u area entry that stored address of signal catcher,modify user level context:artificially create user stack frame to mimic
																							call to signal catcher function;
		modify system level context:write address of signal catcher into program counter field of user saved register context;
		return;
	}
	if (signal is type that system should dump core image of process)
	{
		create file named "core" in current directory;
		write contents of user level context to file "core";
	}
	invoke exit algorithm immediately;
}
*/
/*
4. Algorithm exit   
input:
output:
{
	ignore all signals;
	if (process group leader with associated control terminal)
	{
		send hanup signal to all members of process group;
		reset process group for all members to 0;
	}
	close all open files (internal version of algorithm close) ;
	release current directory (algorithm iput) ;
	relea$e current (changed) root, if exists (algorithm iput) ;
	free regions, memory associated with process (algorithm freereg) ;
	write accounting record;
	make process state zombie
	assign parent process ID of all child processes to be init proc115s ( 1 ) ;
		if any children were zombie, send death of child signal to init;
	send death of child signal to parent process;
	context switch;	
}
*/
/*
5. Algorithm wait
input: address of variable to store status of exiting process
output: child ID, child exit code
{
	if (waiting process has no child processes)
		return (error) ;
	for (;;)  //loop until return £rom inside loop 
	{
		if (waiting process has zombie child)	
		{
			pick arbitrary zombie child;
			add child CPU usage to parent;
			free child process table entry;
			return (child ID, child exit code) ;
		}
		if (process has no children)
			return error;
		sleep at interruptible priority (event child process exits) ;
		
	}
}
*/
/*
6. Algorithm exec
input:file name, parameter list, environment variables list
output: none
{
	get file inode (algorithm namei);
	verify file executable, user has permission to execute;
	read file headers, check that it is a load module;
	copy exec parameters from old address space to system space;
	for (every region attached to process)
		detach all old regions (algorithm detach);
	for (every region specified in load module)
	{
		allocate new regions (algorithm allocreg);
		attach the regions (algorithm attachreg);
		load region into memory if appropriate (algorithm loadreg);
	}
	copy exec parameters into new user stack region;
	special processing for setuid programs, tracing;
	initialize user register save area for return;
	release inode of file (algorithm iput);
}
*/
/*
7. Algorithm xalloc //allocate and initialize text region 
input: inode of executable file
output: none
{
	if (executable file does not have separate text region)
		return;
	if (text region associated with text of inode)
	{
		//text region already exists ... attach to it 
		lock region;
		while (contents of region not ready yet)
		{ 
			// manipulation of reference count prevents total removal of the region
			increment region reference count;
			unlock region;
			sleep (event contents of region ready) ;
			lock region;
			decrement region reference count;
		}
		attach region to process (algorithm attachreg) ;
		unlock region;
		return;
	}
	//no such text region exists---create one 
	allocate text region (algorithm allocreg) ; // region is locked 
	if (inode mode has sticky bit set)
		turn on region sticky flag;
	attach region to virtual address indicated by inode file header(algorithm attachreg) ;
	if (file specially formatted for paging system)  
		//this case further checked 
	else // not formatted for paging system 
		read file text into region (algorithm loadreg) ;
	change region protection in per process region table to read only;
	unlock region;
}
*/
/*
//
8. Algorithm   //user id of a process


*/

/*
// changing the sise of process ,A process may increase or decrease the size of its data region by using the brk system call.
9. Algorithm brk
input: new break value
output: old break value
{
	lock process data region;
	if (region size increasing)
	{
		if (new region size is illegal)
		{
			unlock data region;
			return (error) ;
		}
	}
	change region size (algorithm growreg) ;
	zero out addresses in new data space;
	unlock process data region;
}
*/
/*
//the shell, 
10. Algorithm


*/

/*
// system boot and the init process
//To initialize a system from an inactive state, an administrator goes through a
"bootstrap" sequence: The administrator "boots" the system. Boot procedures
vary according to machine type, but the goal is common to all machines: to get a
copy of the operating system into machine memory and to start executing it. This
is usually done in a series of stages; hence the name bootstrap. The administrator
may set switches on the computer console to specify the address of a special hardcoded
bootstrap program or just push a single button that instructs the machine to
load a bootstrap program from its microcode. This program may consist of only a
few instructions that instruct the machine to execute another program.
//the bootstrap procedure eventually reads the boot block (block 0) of a
disk, and loads it into memory. The program contained in the boot block loads the
kernel from the file system
//After the kernel is loaded in memory, the boot
program transfers control to the start address of the kernel, and the kernel st<1rts
running (algorithm start ,


11. Algorithm start    //system startup procedure
input: none
output: none
{
	initialize all kernel data structures; //The kernel initializes its internal data structures. For instance, it constructs the
											//linked lists of free buffers and inodes, constructs hash queues for buffers and inodes,
											//initializes region structures, page table entries, and so on.
	pseudo-mount of root;                 //it mounts the root file system onto root ("/") and fashions the
											//environment for process 0, creating a u area , initializing slot 0 in the process table
										//and making root the current directory of process 0, among other things.
	hand-craft environment of process 0;
	fork process 1 :
	{
		// process 1 in here
		allocate region;
		attach region to init address space;
		grow region to accommodate code about to copy in;
		copy code from kernel space to init user space to exec init;
		change mode: return from kernel to user mode;
		//init never gets here---as result of above change mode,init exec's /etc/init and becomes a "normal" user processwith respect to invocation of system //calls
	}
	//proc 0 continues here 
	fork kernel processes;
    //process 0 invokes the swapper to manage the allocation of process address space to main memory and the swap devices.
    //This is an infinite loop; process 0 usually sleeps in the loop unless there is work for it to do
   execute code for swapper algorithm;
}


12. Algorithm init  //init process, process 1 of the system
input:
output:
{
	fd = open ("/etc/injttab", 0 RDONLY) ;
	while (line read(fd, buffer) )
	{
		//read every line of file 
		if (invoked state !- buffer state)
			continue; // loop back to while 
		// state matched
		if (for() -- 0)
		{
			execl("process specified in butTer") ;
			exit();
		}
        //init process does not wait loop back to while
	}
	while ((id = wait((int*) O)) != -1 )
	{
	//check here if a spawned child died; 
	//consider respawning it, 
	//otherwise, just continue
	}
}


//in the system are either user processes, daemon processes, orkernel processes.

//Most processes on typical systems are user processes, associated with users at a terminal
//Daemon processes are not associated with any users but
do system-wide functions, such as administration and control of networks, execution
of time-dependent activities, line printer spooling, and so on. /nit may spawn
daemon processes that exist throughout the lifetime of the system or, on occasion,
users may spawn them. They are like user processes in that they run at user mode
and make system calls to access system services.
//Kernel processes execute only in kernel mode. Process 0 spawns kernel
processes, such as the page-reclaiming process vhand, and then becomes the
swapper process. Kernel processes are similar to daemon processes in that they
provide system-wide services, but they have greater control over their execution
priorities since their code is part of the kernel. They can access kernel algorithms
and data structures directly without the use of system calls, so they are extremely
powerful. However, they are not as flexible as daetnon processes, because the
kernel must be recompiled to change them.
*/