/*
The buffer is currently locked (looked is used busy sometyimes).
The bufer contains valid data.
The kernel must write the bfeer contents to disk before reassigning the buffer. this is knowas as delayed-write.
The kernel is currently reading ot writing the contents of the buffer to disk.
A process is currently waiting for the buffer to become free.
*/
struct BufferHeaders{
	uint8 device_num; //file system of data on disk, unique buffer identifire on disk
	unit8 block_num;  // device number of data on disk, unique buffer identifire on disk, it a logical file systrem number
	uint8 status;     //
	uint8 *ptrtodataArea; //
	uint8 *ptrtonextbufonhashqueue; //
	uint8 *ptrtopreveousbufonhashqueue; //
	uint8 *ptrtonextbufonfreelist;     //
	uint8 *ptrtopreveousbufonfreelist; //
};

/*
used this buffer pool according to least recently unsed algorithm. after it allocates a buffer to a disck, it can not use the buffer for 
for another block until all other buffers have been used more recently.
This is a doubly linked circular list of buffer with dummy buffer, header that marks its beginning and end.
*/
/*
struct BufferPool{
	uint8 *bufferpool1; // used by the buffer allocation algorithms to maintain the overall structure of the buffer pool.
	uint8 *bufferpool2; // used by the buffer allocation algorithms to maintain the overall structure of the buffer pool. //
};
struc BufferPoolHead{
	uint8 *next;
	uint8 *prev;
};
*/

struct BufferPool{
	uint8 *next;
	uint8 *prev;
};

/*
device block number ;
buffer into separet queue into circula, double linked list, similar to structure of the free list.
The number of buffer on a hash queue varies during lifetime of the system.
Kernel use hashing funtion that distribute the bufer unformly acrosss the set of hash queue 
and hash funtion is simple to improve the preformance.
admin should configure the number of hash queue when generating the OS.
This section describes five typical scenarios the kernel may follow in getblk to allocate a buffer for a disk block.
1. The kernel finds the block on its hash queue, and its buffer is free.
2. The kernel cannot find the block on the hash queue, so it allocates a buffer from the free list.
3. The kernel cannot find the block on the hash queue and, in attempting to allocate a buffer from the 
   free list (as in scenario 2), finds a buffer on the free list that has been marked "delayed write." 
   The kernel must write the "delayed write" buffer to disk and allocate another buffer.
4. The kernel cannot find the block on the hash queue, and the free list of buffers is empty.
5. The kernel finds the block on the hash queue, but its buffer is currently busy.

*/
/*
below should be based on hash table
*/
struct HashQueueHeaders{
	uint8 *blockNumber[]; // block number and mod numbers ,anagements
	
};


/*
Algorithm brelse use  to releasing the buffer.
input: locked buffer
output: node
{
		wakeup all procs: event, waiting for any buffer to become free;
		wakeup all procs: event, waiting for this buffer to become free;
		raise processor execution level to block interrupts;
		if(buffer contents valid and buffer not old)
			enqueue buffer at end of free list
		else
			enqueue buffer at beginning of free list
		lower processor execution level to allow interrupts;
		unlock(buffer);
}
*/




/*
Algorithm bread use for reading a disk block.
input: file system block number
output: buffer containing data
{
	get buffer for a block(algorithm getblk);
	if(buffer data valid)
		return buffer;
	initiate disk read;
	sleep(event disk read complete);
	return()buffer;
}
*/


/*
Algorithm breada use for read and read ahead
input: 1. file system block number for immediate read
	   2. file system block number for asynchronous read
output: buffer coantaining data for immediate read
{
	if(first block notin cache)
	{
		get buffer for first block(algorithm getblk)'
		if(buffer data no valid)
			initiate disk read;
	}
	if(second block not in cache)
	{
		get buffer for second block(algorithm getblk);
		if(buffer data valid)
			release buffer (algorithm brelse);
		else
			initiate disk read;
	}
	if(first block was orogonally in cache)
	{
		read first block (algorithm bread);
		return bufer;
	}
	sleep(event first buffer contains valid data);
	return buffer;
}	   
*/

/*
Algorithm bwrite for block write
input: buffer
output: none
{
	initiate disk write;
	if(I/O synchronous)
	{
		sleep(event I/O complete);
		release buffer (algorithm brelse);
	}
	else if (buffer marked for delayed write)
		mark buffer to put at head of free list;
}
*/




