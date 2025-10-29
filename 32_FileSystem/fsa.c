#iget  // 1. returen the preveously identified mode reading from disk via the buffer cache
#iput // 2. release the mode
#bmap // 3. set the kernel parametersfor accesing the file
#namei // 4. convert a user-level path to an mode, using the algo iget, iput and bmap
#alloc // 5. allocate disk blocks for files
#free // 6. free disk blocks for files
#ialloc or Wim // 7. assign inodes for files
#ifree // 8. free inodes for files


/*
struct /*file_/*inodes{ // access by well define set of system calls and specify file by a character string  that is the file path name and kernel conver path name to file's mode
	owner;
	access right;
	file size;
	file_location;
	/*
	owner mjb
	group os
	type regular file
	perms rwxr-xr-xr-x
	accessed dec 22 1988 1:33 P.M
	modified dec 22 1988 1:33 P.M
	inodes dec 22 1988 1:33 P.M // exist in the state from on disk, kernel read it into core mode and manipulate 
	size 3300 bytes // ata in a file is addressable by the number of bytes from the beginning of the file, starting from byte offset 0, and the file size is 1 greater than the highest byte offset of data in the file
	disk addresses //users treat the data in a file as a logical stream of bytes, the kernel saves the data in discontiguous disk blocks. The Mode identifies the disk blocks that contain the file's data.
	*//*
} // sample of disk inode
*/

/*
block num = ((inode number - 1) / number of inodes per block) + start block of inode list
byte offset of inode = ((inode number -1 ) modulo (number of inodes per block)) * size of disk inodes
1. Algorithm iget // algorith for allocation of In-core Inodes
input: file system inode number
output: locked inode
{
	while (not done)
	{
		if (inode in inode cache)
		{
		if (inode locked)
			{
				sleep (event inode becomes unlocked);
				continue;
			}
			/* special processing for mount points *//*
			if (inode on inode free list) 
				remove from free list;
			increase inode reffrences count;
			return (inode);
		}*/
		/*inode not in inode cache*//*
		if (no inodes on free list)
			return (error);
		remove new inode from free list;
		reset inode number and file system;
		remove inode from old hash queue, place on new one;
		read inode from disk (algorithm bread);
		initialize inode (ex. reffrences count to 1);
		return(inode);
	}
	
}

*/


/*
2. Algorithm iput // release an Inode
input: pointer to in-core inode
output: none
{
	lock inode if not alreday locked;
	decrement inode reffernce count;
	if(reffrence count == 0)
	{
		if(inode link count == 0)
		{
			free disk block for file (algorithm free);
			set file type to 0;
			free inode (algorithm ifree);
		}
		if(file accessed or inode changed or file changed)
			update disk inode;
		put inode on free list;
	}
	release inode lock;
}
*/

/*
3. Algorithm bmap //block map of logical file byte offset to file system block
input: inode
		byte offset
output:block number in the file system, byte offset into block, bytes of I/O in block, read ahead block number
{
	calculate logical block number in file from byte offset;
	calculate start byte in block for I/O; // output 2
	calculate number of bytes to copy to user; // output 3
	check if read-ahead applicable, mark inode; // output 4
	determine level of indirection;
	while(not at necessary level of indirection)
	{
		calculate index into inode or indirect block from logical block number in file;
		get disk block number from inode or indirect block;
		release buffer from preveous disk read, if any (algorithm brelse);
		if(no more levels of indirection)
			return(block numbner);
		read indirect disk block(algorithm bread);
		adjust logical block number in file according to level of indirection;
	}
}
*/
/*
// u area contains a pointer to the current directory inode
4. Algorithm namei // conver path name to inode
input: path name
output: locked inode
{
	if(path name start from root)
		working inode == root inode (algorithm iget);
	else
		working inode = current directory inode (algorithm iget);
	while(there is more path name)
	{
		read next path name component from input;
		verify that working inode is of directory, access permission OK;
		if(working inode is of root and component is "..")
			continue; // llop baeck to while
		read directory (working inode by repeated use of algorithm bmap, bread and brelse);
		if(component matches an entry in directory(working inode))
		{
			get inode number for matcheed component;
			release working inode (algorithm iput);
			working inode  = inode of matched component(algorithm iget);
		}
		else // component not in directory
		return (no inode);
	}
	retrun (working inode);
}
*/

/*
7. Algorithm ialloc // allocate inode, algorithm for assining new inode
input: file system
output: loked inode
{
	while (not doe)
	{
		if(super block locked)
			sleep(event super block become free);
			continue; // while loop			
	}
	if(inode list in super block is empty)
	{
		lock super block;
		get remembered inode for free inode search;
		search disk for free inode untill super block full, or no morefree inodes(algorithm bread and brelse);
		unlock super block;
		wakeup(event super block becomes free);
		if(no free inodes found on disk)
			return(no inode);
		set rememebred inode for next free inode search;
	}
	// there are inodes in super block inode list
	get inode number from super block inode list;
	get inode(algorithm iget);
	if(inode not free after all)
	{
		write inode to disk;
		release inode(algorithm input);
		continue; // whileloop
	}
	//inode is free
	initialize inode;
	write inode to disk;
	decreament filesystem free inode count'
	return(inode);
}
*/
/*
8.Algorithm ifree //inode free, algorithm for freeing inode
input: file system inode number
output: none
{
	increament file system free inode count;
	if(super block locked)
		return;
	if(inode list full)
	{
		if(inode number less than rememebered inode for search)
			set rememebred inode for search = input inode number;
	}
	else
		store inode number in inode list;
	return;
}
*/
/*
mkfs: make file system that organize the data blocks of a file system in a linked list;
also pipe salso called fifo and special files;
5. Algorithm alloc // file system block allocation
input: file system numberoutput: buffer for a new block
{
	while(super block locked)
		sleep(event super block not locked);
	remove block from super block free list;
	if(removed last block from free list)
	{
		lock super block;
		read block just to taken from free list (algorithmbread);
		copy block numbers in block inot super block;
		release block buffer (algorithm brelse);
		unlock super block;
		wakeup process(event super block not locked);
	}
	get buffer fro block removed from super block list(algorithm getblk);
	zero buffer conttents;
	decreament total count of free blocks;
	mark super block modified;
	return buffer;
}
*/
