/*#1. accessing existing files, such as open, read, write, lseek, and close,
#2. create new files, namely, creat and mknod,
#3. maneuver through the file system: chdir, chroot, chown, chrrtod, slat, and fstat. 
#4. investigates more advanced system calls: pipe and dup are important for the implementation of pipes in the shell
#5. mount and umount extend the file system tree visible to users;
#6. link and unlink change the structure of the file system hierarchy
*/
/*three kernel data structures: the file table, with one entry allocated for every opened file in the system, 
 the user file descriptor table, with one entry allocated for every file descriptor known to a process, and 
 the mount table, containing information for every active file system.*/
 /*
 1.Return File Descriptor: a.open, b.creat, c.dup, d.pipe, and e.close
 2.Use of namei:           a.open, b.creat, c.chdir, d.chroot, e.chown, f.chmod, g.stat, h.link, i.unlink, j.mknod, k.mount and l.unmount
 3.Assign inodes:          a.creat, b.mknod, c.link and d.unlink
 4.File Attributes:        a.chown, b.chmod and c.stat
 5.File I/O:               a.read, b.write and c.lseek
 6.File Systems Structure: a.mount and b.unmount
 7.Tree Manupulation:      a.chdir and b.chown
 */
 /*
• System calls that return file descriptors for use in other system calls;
• System calls that use the namei algorithm to parse a path name;
• System calls that assign and free inodes, using algorithms ial/oc and ifree ;
• System calls that set or change the attributes of a file;
• System calls that do 1/0 to and from a process, using algorithms al/oc , Jree ,and the buffer allocation algorithms;
• System calls that change the structure of the file system;
• System calls that allow a process to change its view of the file system tree.
 */
 
/*
1.Algorithm open
inputs: file name,type of open and file permission(for creation type of open)
output: file descriptor
{
	convert file name to inode(algorithm namei);
	if(filedoes not exit or not permitted access)
		return(error);
	allocate file table entry for inode,initialize count, offset;
	allocate user file descriptor entry, set pointer to file table entry;
	if(type of open specifies truncate file)
		free all file blocks(algorithm free);
	unlock(namei); // locked above in namei
	return(user file descriptor);
}
*/
/*
2.Algorithm read
input: user file descriptor, address of buffer in user process, number of bytesto read
output: count of bytes copied inot user space
{
	get file table entry from user file descriptor;
	check file accessibility;
	set parameters in u area for user address, byte count,I/O to user;
	get inode from file table;
	lock inode;
	set byte offset in u area from file table offset;
	while(count not satisfied)
	{
		conver file offset to disk block (algorithm bmap);
		calculate offset inot block,number of bytes to read;
		if(number of bytes to read is 0) // try to read end of file
			break; // out of loop
		read block(algorithm breada if with read ahead, algorithm bread otherwise);
		copy data from system buffer to user address;
		update u area fields for file byte offset, read count, address to write into user space;
		release buffer; // locked in bread 		
	}
	unlock inode;
	update file table offset for next read;
	return(total number of bytes read);
}
*/
/*
if the file does not corttain a block that
corresponds" to the byte offset to be written, the kernel allocates a new block using
algorithm alloc and assigns the block number to the correct position in the;; inodes
table of contents.

3. Algorithm write // similar to read



*/
/*
4. Algorithm lseek


*/
/*
5.Algorithm close


*/
/*
6.Algorithm creat
input: file name, permission settings
output: file descriptor
{
	get inode for file name(algorithm namei);
	if(file already exists)
	{
		if(not permitted access)
		{
			release inode(algorithm iput);
			return(error);
		}
	}
	else //file does not exits yet
	{
		assign free inode from file system(algorithm ialloc);
		create new directory entry in parent direcotry: include new file name and newly assigned inode number;
	}
	allocate file table entry for inode, initialize count;
	if(file did exist at time of create)
		free all file blocks(algorithm free);
	unlock(inode);
	retunr(user file descriptor);
}
*/
/*
//mknod creates special files in the system, including named pipes, device files, and directories.
//It sets the file type field in the inode to indicate that the file type is a pipe, directory or special file.
//ifthe file is a character special or block special device file, it writes the major and minor device numbers into the inode.
7.Algorithm make new node// mknod
inputs:node(file name), file type,permission,major, minor device number(f for block, character special files)
outputs:none
{
	if(new node not named pipe and user not super user)
		return(error);
	get inode of parent of new node(algorithm namei);
	if(new node already exists)
	{
		release parent inode(algorithm iput);
		return(error);
	}
	assign free inode from file system for new node(algorithm ialloc);
	create new directory entry in parant direcotry: include new node name and newly assigned inode number;
	release parant directory inode(algorithm iput);
	if(new node is block or character special file)
		write major,minor numbers inot inode structure;
	release new node inode(algorithm iput);
}
*/
/*
//When the system is first booted, process 0 makes the file system root its current directory during initialization.
//It executes the algorithm iget on the root inode, saves it in the u area as its current directory, and releases the inode lock.
//When a new process is created via the fork system call, the new process inherits the current directory of the old process in its u area, and the kernel increments the inode reference count accordingly.

8.Algorithm chdir 
input: new direcotry name
output:none
{
	get inode for new direcotry name (algorithm namei);
	if(inode not that of direcotry or process not permitted access to file)
	{
		release inode(algorithm iput);
		return(error);
	}
	unlock inode;
	release "old" current direcotry inode(algorithm);
	place new inode into current direcotry slot in u area;
}
*/
/*
9. Algorithm chroot

*/
/*
//To change the owner of a file, the kernel converts the file name to an inode using algorithm namei .
//and releases the inode via algorithm iput .

10. Algorithm chown

*/
/*
// use similar approch as chroot to changinf the flag modes in the onode instead of the owner numbers;

11. Algorithm chmod

*/
/*
//The system calls stat and fstat allow processes to query the status of files, returning
information such as the file type, file owner, access permissions, file size, number of
links, inode number, and file access times.

12. Algorithm stat


*/
/*
13. Algorithm fstat


*/
/*
14.Algorithm pipe // unammed and named. this is unamed
input: noe
output: read file descriptor, write file descriptor
{
	assign new inode from pipe device(algorithm ialloc);
	allocate file table entry for reading, another for writing;
	initialize file table entries to point to new inode;
	allocate user file descriptor for reading, another for writing,initialize to point to respective file table entries;
	set inode reffrence count to 2;
	initialize count of inode readers, writers to 1;
}
*/
/*
15. Algorithm dup


*/
/*
16. Algorithm mount
inputs: file name of block special file, direcotry name of mount point options(read only)
outout:none
{
	if(not super user)
		return (error);
	get inode for block special file(algorithm namei);
	make legality checks;
	get inode for "mount on" direcotory name(algorithm namei);
	if(not direcotry, or reffrence count > 1)
	{ 
		release inode(algorithm iput);
		return (error);
	}
	find empty slot in mount table;
	invoke block device driver open routine;
	get free buffer from buffer cache;
	read super block into free buffer;
	initialize super block fields;
	get root inode of mount device(algorithm iget), save in mount table;
	mark inode of "mounted on" direcotory as mount point;
	release special file inode(algorithm);
	unlock inode of mount point direcotory;
}

*/
/*
17. Algorithm unmount
input: special file name of file system to be unmount
output: none
{
	if(not super user)
		return (error);
	get inode of special file(algorithm namei);
	extract major, minor number of device being unmount;
	get mount table entry,based on major, min number, for unmounting file system;
	release inode of special file(algorithm iput);
	remove shared text entries from region table for files belonging to file system;
	update super block, inode, flush buffers;
	if(file from file system still in use)
		return (error);
	get root inode of mounted file system from mount table;
	lock inode;
	release inode(algorithm iput);
	invoke close routine for special devices;
	invalidate buffers in pool from unmounted file system;
	get inode of mount point from mount table;
	lock inode;
	clear flag marking it as mount point;
	release inode(algorithm iput);
	free buffer used for super block;
	free mount table slot;
}
*/
/*
//The link system call links a file to a new name in the file system directory
structure, creating a new directory entry for an existing inode;
18. Algorithm link
input: existing file name, new file name
output: none
{
	get inode for existing file name (algorithm namei) ;
	if (too many links on file or linking directory without super user permission)
	{
		release inode (algorithm iput) ;
		retum(error) ;
	}
	increment link count on inode;
	update disk copy of inode;
	unlock inode;
	get parent inode for directory to contain new file name (algorithm namei) ;
	if (new file name already exists or existing file, new file on different file systems)
	{
		undo update done above;
		return (error) ;
	}
	create new directory entry in parent directory of new file name:include new file name, inode number of existing file name;
	release parent directory inode (algorithm iput) ;
	release inode of existing file (algorithm iput) ;
}
*/
/*
19. Algorithm unlink
input: file name
output: none
{
	get parent inode of file to be unlinked (algorithm namei) ;// if unlinking the current directory . . . 
	if (last component of file name is " .")
		increment inode reference count;
	else
		get inode of file to be unlinked (algorithm iget) ;
	if (file is directory but user is not super user)
	{
		release inodes (algorithm iput) ;
		return (error) ;
	}
	if (shared text file and link count currently 1 )
		remove from region table;
	write parent directory: zero inode number of unlinked file;
	release inode parent directory (algorithm iput) ;
	decrement file link count;
	release file inode (algorithm iput) ; //iput checks if link count is 0: if so,releaseS file blocks (algorithm free) and frees in ode (algorithm ifree) ;
}
*/