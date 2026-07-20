#define __PAGE_CACHE__H
#ifdef __PAGE_CACHE__H

#define page cache
#ifdef page cache
/*
The I/O data transfer does
not start immediately: the disk update is delayed for a few seconds, thus giving a
chance to the processes to further modify the data to be written
*/
/*
Kernel code and kernel data structures don’t need to be read from or written to disk.*
Thus, the pages included in the page cache can be of the following types:
• Pages containing data of regular files; in Chapter 16, we describe how the kernel
handles read, write, and memory mapping operations on them.
• Pages containing directories; as we’ll see in Chapter 18, Linux handles the directories
much like regular files.
• Pages containing data directly read from block device files (skipping the filesystem
layer); as discussed in Chapter 16, the kernel handles them using the same
set of functions as for pages containing data of regular files.
• Pages containing data of User Mode processes that have been swapped out on
disk. As we’ll see in Chapter 17, the kernel could be forced to keep in the page
cache some pages whose contents have been already written on a swap area
(either a regular file or a disk partition).
• Pages belonging to files of special filesystems, such as the shm special filesystem
used for Interprocess Communication (IPC) shared memory region (see
Chapter 19).
*/
/*
Practically all read() and write() file operations rely on the page cache. The only
exception occurs when a process opens a file with the O_DIRECT flag set: in this case,
the page cache is bypassed and the I/O data transfers make use of buffers in the User
Mode address space of the process (see the section “Direct I/O Transfers” in
Chapter 16); several database applications make use of the O_DIRECT flag so that they
can use their own disk caching algorithm.
Kernel designers have implemented the page cache to fulfill two main requirements:
• Quickly locate a specific page containing data relative to a given owner. To take
the maximum advantage from the page cache, searching it should be a very fast
operation.
• Keep track of how every page in the cache should be handled when reading or
writing its content. For instance, reading a page from a regular file, a block
device file, or a swap area must be performed in different ways, thus the kernel
must select the proper operation depending on the page’s owner.
*/
//The core data structure of the page cache is the address_space object, a data structure embedded in the inode object that owns the page.*
//Each page descriptor includes two fields called mapping and index, which link the page to the page cache (see the section “Page Descriptors” in Chapter 8).
/*
The first
field points to the address_space object of the inode that owns the page. The second
field specifies the offset in page-size units within the owner’s “address space,” that is,
the position of the page’s data inside the owner’s disk image. These two fields are
used when looking for a page in the page cache.
*/

struct address_space
{
    struct inode * host //Pointer to the inode hosting this object, if any
    struct radix_tree_root page_tree //Root of radix tree identifying the owner’s pages
    spinlock_t tree_lock //Spin lock protecting the radix tree
    unsigned int i_mmap_writable //Number of shared memory mappings in the address space
    struct prio_tree_root i_mmap //Root of the radix priority search tree (see Chapter 17)
    struct list_head i_mmap_nonlinear //List of non-linear memory regions in the address space
    spinlock_t i_mmap_lock //Spin lock protecting the radix priority search tree
    unsigned int truncate_count //Sequence counter used when truncating the file
    unsigned long nrpages //Total number of owner’s pages
    unsigned long writeback_index //Page index of the last write-back operation on the owner’s pages
    struct address_space_ operations * a_ops //Methods that operate on the owner’s pages
    unsigned long flags //Error bits and memory allocator flags
    struct backing_dev_info * backing_dev_info //Pointer to the backing_dev_info of the block device holding the data of this owner
    spinlock_t private_lock //Usually, spin lock used when managing the private_list list
    struct list head private_list //Usually, a list of dirty buffers of indirect blocks associated with the inode
    struct address_space * assoc_mapping //Usually, pointer to the address_space object of the block device including the indirect blocks
};
/*
The i_mapping field of the inode always
points to the address_space object of the owner of the pages containing the inode’s
data. The host field of the address_space object points to the inode object in which
the descriptor is embedded.
*/

/*
A crucial field of the address_space object is a_ops, which points to a table of type
address_space_operations containing the methods that define how the owner’s pages
are handled. These methods are
*/
writepage; //Write operation (from the page to the owner’s disk image)
readpage;// Read operation (from the owner’s disk image to the page)
sync_page;// Start the I/O data transfer of already scheduled operations on owner’s pages
writepages;// Write back to disk a given number of dirty owner’s pages
set_page_dirty;// Set an owner’s page as dirty
readpages;// Read a list of owner’s pages from disk
prepare_write;// Prepare a write operation (used by disk-based filesystems)
commit_write;// Complete a write operation (used by disk-based filesystems)
bmap;// Get a logical block number from a file block index
invalidatepage ;//Invalidate owner’s pages (used when truncating the file)
releasepage;// Used by journaling filesystems to prepare the release of a page
direct_IO;// Direct I/O transfer of the owner’s pages (bypassing the page cache)





#define The Radix Tree
#ifdef The Radix Tree
//The page_tree field of an address_space object is the root of a radix tree, which contains pointers to the descriptors of the owner’s pages.
/*
Table 15-3. Highest index and maximum file size for each radix tree height
Radix tree
height Highest index Maximum file size
0 none 0 bytes
1 26 -1 = 63 256 kilobytes
2 212 -1 = 4 095 16 megabytes
3 218 -1 = 262 143 1 gigabyte
4 224-1 = 16 777 215 64 gigabytes
5 230 -1 = 1 073 741 823 4 terabytes
6 232 -1 = 4 294 967 295 16 terabytes
*/
#endif // end of The Radix Tree


#define Page Cache Handling Functions
//Finding a page
find_get_page();
//Adding a page
add_to_page_cache();
//Removing a page
remove_from_page_cache( );
//Updating a page
read_cache_page();


#define The Tags of the Radix Tree
/*
the page cache not only allows the kernel to quickly retrieve a
page containing specified data of a block device; the cache also allows the kernel to
quickly retrieve pages in the cache that are in a given state.
*/



#endif // end of page cache
/*-----------------------enf of page cache ------------------- */


#define Storing Blocks in the Page Cache
#ifdef Storing Blocks in the Page Cache
/*
the VFS,
the mapping layer, and the various filesystems group the disk data in logical units
called “blocks.
*/
/*
In old versions of the Linux kernel, there were two different main disk caches: the
page cache, which stored whole pages of disk data resulting from accesses to the contents
of the disk files, and the buffer cache, which was used to keep in memory the
contents of the blocks accessed by the VFS to manage the disk-based filesystems.
Starting from stable version 2.4.10, the buffer cache does not really exist anymore. In
fact, for reasons of efficiency, block buffers are no longer allocated individually;
instead, they are stored in dedicated pages called “buffer pages,” which are kept in
the page cache.
Formally, a buffer page is a page of data associated with additional descriptors called
“buffer heads,” whose main purpose is to quickly locate the disk address of each
individual block in the page. In fact, the chunks of data stored in a page belonging to
the page cache are not necessarily adjacent on disk.
*/

//Block Buffers and Buffer Heads
/*
Each block buffer has a buffer head descriptor of type buffer_head. This descriptor
contains all the information needed by the kernel to know how to handle the block;
thus, before operating on each block, the kernel checks its buffer head. The fields of
a buffer head are listed in Table 15-4.
*/
struct buffer_head
{
    unsigned long b_state// Buffer status flags
    struct buffer_head * b_this_page //Pointer to the next element in the buffer page’s list
    struct page * b_page //Pointer to the descriptor of the buffer page holding this block
    atomic_t b_count //Block usage counter
    u32 b_size //Block size
    sector_t b_blocknr //Block number relative to the block device (logical block number)
    char * b_data// Position of the block inside the buffer page
    struct block_device * b_bdev //Pointer to block device descriptor
    bh_end_io_t * b_end_io //I/O completion method
    void * b_private //Pointer to data for the I/O completion method
    struct list_head b_assoc_buffers //Pointers for the list of indirect blocks associated with an inode (see the section “The address_space Object” earlier in this chapter)
};
/*
The b_data field specifies the position of the block buffer inside the buffer page.
Actually, the encoding of this position depends on whether the page is in high memory
or not. If the page is in high memory, the b_data field contains the offset of the
block buffer with respect to the beginning of the page; otherwise, b_data contains the
linear address of the block buffer.
The b_state field may store several flags. Some of them are of general use and are
listed in Table 15-5. Each filesystem may also define its own private buffer head flags.
*/

enum buffer_head_flag{
    BH_Uptodate //Set if the buffer contains valid data
    BH_Dirty //Set if the buffer is dirty—that is, it contains data that must be written to the block device
    BH_Lock //Set if the buffer is locked, which usually happens when the buffer is involved in a disk transfer
    BH_Req //Set if data transfer for initializing the buffer has already been requested
    BH_Mapped //Set if the buffer is mapped to disk—that is, if the b_bdev and b_blocknr fields of the corresponding buffer head are significant
    BH_New// Set if the corresponding block has just been allocated and has never been accessed
    BH_Async_Read //Set if the buffer is being read asynchronously
    BH_Async_Write //Set if the buffer is being written asynchronously
    BH_Delay //Set if the buffer is not yet allocated on disk
    BH_Boundary// Set if the block to be submitted after this one will not be adjacent to this one
    BH_Write_EIO //Set if there was an I/O error when writing this block
    BH_Ordered //Set if the block should be written strictly after the blocks submitted before it (used by journaling filesystems)
    BH_Eopnotsupp// Set if the block device driver does not support the operation requested
};



#define Managing the Buffer Heads
/*
The buffer heads have their own slab allocator cache, whose kmem_cache_s descriptor
is stored in the bh_cachep variable. The alloc_buffer_head() and free_buffer_head()
functions are used to get and release a buffer head, respectively.
The b_count field of the buffer head is a usage counter for the corresponding block
buffer. The counter is increased right before each operation on the block buffer and
decreased right after. The block buffers kept in the page cache are examined both
periodically and when free memory becomes scarce, and only the block buffers having
null usage counters may be reclaimed

The function that locates a block inside the page cache (_ _getblk();

When a kernel control path stops accessing a block buffer, it should invoke either
_ _brelse( ) or _ _bforget( ) to decrease the corresponding usage counter. The difference
between these two functions is that _ _bforget() also removes the block
from any list of indirect blocks (b_assoc_buffers buffer head field; see the previous
section “Block Buffers and Buffer Heads”) and marks the buffer as clean, thus forcing
the kernel to forget any change in the buffer that has yet to be written on disk.
*/



#define Buffer Pages
/*
Whenever the kernel must individually address a block, it refers to the buffer page
that holds the block buffer and checks the corresponding buffer head.
Here are two common cases in which the kernel creates buffer pages:
• When reading or writing pages of a file that are not stored in contiguous disk
blocks. This happens either because the filesystem has allocated noncontiguous
blocks to the file, or because the file contains “holes” (see the section “File
Holes” in Chapter 18).
• When accessing a single disk block (for instance, when reading a superblock or
an inode block).
In the first case, the buffer page’s descriptor is inserted in the radix tree of a regular
file. The buffer heads are preserved because they store precious information: the
block device and the logical block number that specify the position of the data in the
disk. We will see how the kernel makes use of this type of buffer page in Chapter 16.
In the second case, the buffer page’s descriptor is inserted in the radix tree rooted at
the address_space object of the inode in the bdev special filesystem associated with
the block device (see the section “The address_space Object” earlier in this chapter).
This kind of buffer pages must satisfy a strong constraint: all the block buffers must
refer to adjacent blocks of the underlying block device.
*/



#define Allocating Block Device Buffer Pages


#define Releasing Block Device Buffer Pages


#define Searching Blocks in the Page Cache
__find_get_block();
__getblk();
__bread();



#define Submitting Buffer Heads to the Generic Block Layer
/*
A couple of functions, submit_bh() and ll_rw_block(), allow the kernel to start an
I/O data transfer on one or more buffers described by their buffer heads.
*/
submit_bh();
ll_rw_block();



#endif // end of Storing Blocks in the Page Cache







/*-----------------------enf of Storing Blocks in the Page Cache ------------------- */
#define Writing Dirty Pages to Disk
#ifdef Writing Dirty Pages to Disk
/*
the kernel keeps filling the page cache with pages containing data
of block devices. Whenever a process modifies some data, the corresponding page is
marked as dirty—that is, its PG_dirty flag is set.

Unix systems allow the deferred writes of dirty pages into block devices, because this
noticeably improves system performance. Several write operations on a page in cache
could be satisfied by just one slow physical update of the corresponding disk sectors.
Moreover, write operations are less critical than read operations, because a process
is usually not suspended due to delayed writings, while it is most often
suspended because of delayed reads. Thanks to deferred writes, each physical block
device will service, on the average, many more read requests than write ones.

A dirty page might stay in main memory until the last possible moment—that is,
until system shutdown. However, pushing the delayed-write strategy to its limits has
two major drawbacks:
• If a hardware or power supply failure occurs, the contents of RAM can no longer
be retrieved, so many file updates that were made since the system was booted
are lost.
• The size of the page cache, and hence of the RAM required to contain it, would
have to be huge—at least as big as the size of the accessed block devices.
Therefore, dirty pages are flushed (written) to disk under the following conditions:
• The page cache gets too full and more pages are needed, or the number of dirty
pages becomes too large.
• Too much time has elapsed since a page has stayed dirty.
• A process requests all pending changes of a block device or of a particular file to
be flushed; it does this by invoking a sync( ), fsync( ), or fdatasync( ) system
call (see the section “The sync( ), fsync( ), and fdatasync() System Calls” later in
this chapter).

*/

#define The pdflush Kernel Threads
/*a kernel thread called bdflush to systematically scan
the page cache looking for dirty pages to flush, and they used a second kernel thread
called kupdate to ensure that no page remains dirty for too long.*/
struct pdflush_work
{
    struct task_struct * who //Pointer to kernel thread descriptor
    void(*)(unsigned long) fn //Callback function to be executed by the kernel thread
    unsigned long arg0 //Argument to callback function
    struct list head list //Links for the pdflush_list list
    unsigned long when_i_went_to_sleep //Time in jiffies when kernel thread became available
};

#define Looking for Dirty Pages To Be Flushed
#define Retrieving Old Dirty Pages



#endif // end of Writing Dirty Pages to Disk
/*-----------------------enf of Writing Dirty Pages to Disk ------------------- */
#define System Calls
#ifdef System Calls

sync( );
fsync( );
fdatasync();
/*
The service routine sys_sync( ) of the sync() system call invokes a series of auxiliary
functions:
wakeup_bdflush(0);
sync_inodes(0);
sync_supers();
sync_filesystems(0);
sync_filesystems(1);
sync_inodes(1);
*/
#endif // end of System Calls
/*-----------------------enf of System Calls ------------------- */


#endif // end of __PAGE_CACHE__H
