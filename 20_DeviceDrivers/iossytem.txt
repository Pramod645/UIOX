/*
//The I/O subsystem allows a process to communicate with peripheral devices such
as disks, tape drives, terminals, printers, and networks, and the kernel modules that
control devices are known as device drivers.
//There is usually a one-to-one
correspondence between device drivers and device types: Systems may contain one
disk driver to control all disk drives, one terminal driver to control all terminals,
and one tape driver to control all tape drives.
//The system supports "software devices," which have no associated physical
device. For example, it treats physical memory as a device to allow a process
access to physical memory outside its address space, even though memory is not a
peripheral device.
//interfaces between processes and the I/O subsystem
and between the machine and the device drivers.
// is io subsystem and ???
//The user interface to devices goes through the file system :
Every device has a name that looks like a file name and is accessed like a file. The
device special file has an inode and occupies a node in the directory hierarchy of
the file system. The device file is distinguished from other files by the file type
stored in 1ts inode, either "block" or "character special," corresponding to the
device it represents. If a device has both a block and character interface, it is
represented by two device files: its block device special file and its character device
special file. System calls for regular files, such as open , close , read, and write ,
have an appropriate meaning for devices,
//The ioctl system call provides an interface that allows processes to control character devices,but it is not applicable to regular files.
//System configuration is the procedure by which administrators specify parameters
that are installation dependent. Some parameters specify the sizes of kernel tables,
such as the process table, inode table, and file table, and the number of buffers to
be allocated for the buffer pool. Other parameters specify devipe configuration,
telling the kernel which devices are included in the installation and their "address."
//System configuration is the procedure by which administrators specify parameters
that are installation dependent. Some parameters specify the sizes of kernel tables,
such as the process table, inode table, and file table, and the number of buffers to
be allocated for the buffer pool. Other parameters specify devipe configuration,
telling the kernel which devices are included in the installation and their "address."

//The hardware to driver interface consists of machine-dependent control registers
or I/O instructions for manipulating devices and interrupt vectors: When a device
interrupt occurs, the system identifies the interrupting device and calls the
appropriate interrupt handler.
//Administrators set up device special files with the mknod command, supplying
file type (block or character) and major and minor numbers. The mknod command
invokes the mknod system call to create the device file.

//

*/
/*
//System CaDs and the Driver Interface
//describes the interface between the kernel and device drivers. For
system calls that use file descriptor􁓵. the kernel follows pointers from the user file
descriptor to the kernel file table and inode, where it examines the file type and
accesses the block or character device switch table, as appropriate.


block device switch table
entry open    close   strategy
0     gdopen  gdclose gdstrategy
1     gtopen  gtclose gtstrategy

character device switch table
entry  open     close    read    write    ioctl
0      conopen  conclose conread conwrite conioctl
l      dzbopen  dzbclose dzbread dzbwrite dzbioctl
2      syopen   nulldev  syread  sywrite  syioctl
3      nulldev  nulldev  mmread  mmwrite  nodev
4      gdopen   gdclose  gdread  gdwrite  nodev
5      gtopen   gtclose  gtread  gtwrite  nodev


*/
/*
//The kernel follows the same procedure for opening a device as it does for opening
regular files, allocating an in-core inode, incrementing its
reference count, and assigning a file table entry and user file descriptor. The kernel
eventually returns the user file descriptor to the calling process, so that opening a
device looks like opening a regular file. However, it invokes the device-specific
open procedure before. returning to user mode.

1.Algorithm open //for deVice drivers
input: pathname, openmode
output: file descriptor
(
{
	convert pathname to inode, increment inode reference count,
	allocate entry in file table, user file descriptor,
	as in open of regular file;
	get major, minor number from inode;
	save context (algorithm setjmp) in case of long jump frol!l driver;
	if (block device)
	{
		use major number as index to block device switch table;
		call driver open procedure for index:
		pass minor number, open modes;
	}
	else
	{
		use major number as index to character device switch table;
		call driver open procedure for index:pass minor number, open modes;
	}
	if (open fails in driver)
	decrement file table, inode counts;
}
*/
/*
2.Algorithm close    //for devices
input: file descriptor
output: none
{
	do regular close algorithm () ;
	if (file table reference count not 0)
		goto finish;
	if (there is another open file and its major, minor numbers · are same as device being closed)
		goto finish; // not last close after all 
	if (character device)
	{
		use major number to index into character device switch table;
		call driver close routine: parameter minor number;
	}
	if (block device)
	{
		if (device mounted)
			goto finish;
		write device blocks in buffer cache to device;
		use major number to index into block device switch table;
		call driver close routine: parameter minor number;
		invalidate device blocks still in buffer cache;
	}
finish;
	release inode;
}
*/
/*
//The kernel algorithms for read and write of a device are similar to those for a
regular file. If the process is reading or writing a character device, the kernel
invokes the device driver read or write procedure.

3. Algorithm read // device driver

*/
/*
4. Algorithm write // for device 
*/
/*
//Strategy Interface
//The kernel uses the strategy interface to transmit data between the buffer cache
and a device, although as mentioned above, the read and write procedures of
character devices sometimes use their (block counterpart) strategy procedure to
transfer data directly between the device and the user address space. The strategy
procedure may queue 1/0 jobs for a device on a work list or do more sophisticated
processing to schedule 110 jobs. Drivers can set up data transmission for one
physical address or many, as appropriate. The kernel passes· a buffer header
address to the driver strategy procedure; the header contains a list of (page)
addresses and sizes for transmission of data to or from the device. This is also how
the swapping operations discussed

*/
/*
//
5. Algorithm ioctl

*/
/*
//Interrupt Handlers
//occurrence of an interrupt causes the
kernel to execute an interrupt handler, based on the correlation of the interrupting
device and an offset in the interrupt vector table. The kernel invokes the device
specific interrupt handler, passing it the 'device number or other parameters to
identify the specific unit that caused the interrupt.
//the device number used by the interrupt handler identifies a
hardware unit, and the minor number in the device file identifies a device for the ·
kernel. The device driver correlates the mmor device number to the hardware unit
number.

*/
/*
//Line disciplines manipulate data on clists. A clist, or character list, is a variablelength
linked list of cblocks with a count of the number of characters on the list.
A cblock contains a pointer to the next cblock on the linked list, a small character
array to contain data, and a set of offsets indicating the position of the valid data in
the cblock.The start offset indicates the first location of valid data
in the array, and the end offset indicates the first location of nonvalid data.
//The kernel maintains a linked list of free cblocks and has six operations on clists
and cblocks.
1 . It has an operation to assign a cblock from the free list to a driver.
2. It also has an operation to return a cblock to the free list.
3 . The kernel can retrieve the first character from a clist: I t removes the first
character from the first cblock on the clist and adjusts the clist character
count and the indices into the cblock so that subsequent operations will not
retrieve the same character. If a retrieval operation consumes the last
character of a cblock, the kernel places the empty cblock on the free list and
adjusts the clist pointers. If a clist contains no characters when a retrieval
operation is done, the kernel returns the null character.
4. The kernel can place a character onto the end of a clist by finding the last
cblock on the clist, putting the character onto it, and adjusting the offset
values. If the cblock is full, the kernel allocates a new cblock, links it onto
the end of the clist, and places the character into the new cblock.
5 . The kernel can remove a group of characters from the beginning o f a clist one
cblock at a time, the operation being equivalent to removing all the characters
in the cblock one at a time.
6. The kernel can place a cblock of characters onto the end of a clist.


6. Algorithm terminal_ write
{
	while (more data to be copied from user space)
	{
		if (tty flooded with output data)
		{
			start write operation to hardware with data on output clist;
			sleep (event: tty can accept more data) ;
			continue; // back to while loop 
		}
		copy cblock size of data from user space to output clist:
				line discipline converts tab characters, etc;
	}
	start write operatiop to hardware with data on output clist;
}

7. Algorithm read
*/

/*
8. Algorithm login
algonthm login // procedure for logging in 
{
	getty process executes:
	set process group (setpgrp system call) ;
	open tty line; // sleeps until opened 
	if (open successful)
	{
		exec login program:
		prompt for user name;
		turn off echo, prompt for password;
		if (successful) // matches password in /etc/passwd 
		{
			put tty in canonical mode (ioctO ;
			exec shell;
		}
		else
			count login attempts, try again up to a point;
	}
}
*/
/*
//STREAMS


*/