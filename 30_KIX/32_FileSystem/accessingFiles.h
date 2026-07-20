#define __ACCESSINGFILES__H
#ifdef __ACCESSINGFILES__H
/*
Accessing a disk-based file is a complex activity that involves the VFS abstraction
layer (Chapter 12), handling block devices (Chapter 14), and the use of the page
cache (Chapter 15).
here shows how the kernel builds on all those facilities
to carry out file reads and writes. The topics covered in this files apply both to
regular files stored in disk-based filesystems and to block device files; these two
kinds of files will be referred to simply as “files.”
*/

//There are many different ways to access a file.
#define Canonical mode
/*
The file is opened with the O_SYNC and O_DIRECT flags cleared, and its content is
accessed by means of the read() and write() system calls. In this case, the read(
) system call blocks the calling process until the data is copied into the User
Mode address space (however, the kernel is always allowed to return fewer bytes
than requested!). The write() system call is different, because it terminates as
soon as the data is copied into the page cache (deferred write). This case is covered
in the section “Reading and Writing a File.”
*/
#define Synchronous mode
/*
The file is opened with the O_SYNC flag set—or the flag is set at a later time by the
fcntl() system call. This flag affects only the write operation (read operations are
always blocking), which blocks the calling process until the data is effectively
written to disk. The section “Reading and Writing a File” covers this case, too.
*/
#define Memory mapping mode
/*
After opening the file, the application issues an mmap() system call to map the file
into memory. As a result, the file appears as an array of bytes in RAM, and the
application accesses directly the array elements instead of using read(), write(),
or lseek(). This case is discussed in the section “Memory Mapping.”
*/

#define Direct I/O mode
/*
The file is opened with the O_DIRECT flag set. Any read or write operation transfers
data directly from the User Mode address space to disk, or vice versa,
bypassing the page cache. We discuss this case in the section “Direct I/O Transfers.”
(The values of the O_SYNC and O_DIRECT flags can be combined in four
meaningful ways.)
*/

#define Asynchronous mode
/*
The file is accessed—either through a group of POSIX APIs or by means of
Linux-specific system calls—in such a way to perform “asynchronous I/O:” this
means the requests for data transfers never block the calling process; rather, they
are carried on “in the background” while the application continues its normal
execution. We discuss this case in the section “Asynchronous I/O.”
*/




#define Reading and writing file
#ifdef Reading and writing file
generic_file_read( );
generic_file_write();

#define Reading from a File
struct local_iov
{
    /* data */
} iovec;// it contains the address (buf) and the length (count) of the User Mode buffer that shall receive the data read from the file.

struct kiocb;
{
    struct list_head ki_run_list //Pointers for the list of I/O operations to be retried later
    long ki_flags// Flags of the kiocb descriptor
    int ki_users //Usage counter of the kiocb descriptor
    unsigned int ki_key //Identifier of the asynchronous I/O operation, or KIOCB_ SYNC_KEY (0xffffffff) for synchronous I/O operations
    struct file * ki_filp Pointer to the file object associated with the ongoing I/O operation
    struct kioctx * ki_ctx //Pointer to the asynchronous I/O context descriptor for this operation (see the section “Asynchronous I/O” later in this chapter)
    int (*) (struct kiocb *, struct io_event *) ki_cancel //Method invoked when canceling an asynchronous I/O operation
    ssize_t (*) (struct kiocb *) ki_retry //Method invoked when retrying an asynchronous I/O operation
    void (*) (struct kiocb *) ki_dtor// Method invoked when destroying the kiocb descriptor
    struct list_head ki_list //Pointers for the list of active ongoing I/O operation on an asynchronous I/O context
    union ki_obj //For synchronous operations, pointer to the process descriptor that issued the I/O operation; for asynchronous operations, pointer to the iocb User Mode data structure
    __u64 ki_user_data //Value to be returned to the User Mode process
    loff_t ki_pos //Current file position of the ongoing I/O operation
    unsigned short ki_opcode //Type of operation (read, write, or sync)
    size_t ki_nbytes// Number of bytes to be transferred
    char * ki_buf //Current position in the User Mode buffer
    size_t ki_left //Number of bytes yet to be transferred wait_queue_t ki_wait Wait queue used for asynchronous I/O operations
    void * private //Freely usable by the filesystem layer
}kiocb;//it is used to keep track of the completion status of an ongoing synchronous or asynchronous I/O operation.

#define init_sync_kiocb


struct read_descriptor_t
{
    size_t written //How many bytes have been copied into the User Mode buffer
    size_t count //How many bytes are yet to be transferred
    char * arg.buf //Current position in the User Mode buffer
    int error //Error code of the read operation (0 for no error)
};

#define Read-Ahead of Files
/*
stored on disk in large groups of adjacent sectors, so that they can be retrieved
quickly with few moves of the disk heads. When a program reads or copies a file, it
often accesses it sequentially, from the first byte to the last one. Therefore, many
adjacent sectors on disk are likely to be fetched when handling a series of a process’s
read requests on the same file.

The main data structure used by the read-ahead algorithm is the file_ra_state
descriptor whose fields are listed in Table 16-3. Each file object includes such a
descriptor in its f_ra field.
*/
struct file_ra_state
{
    unsigned long start //Index of first page in the current window
    unsigned long size //Number of pages included in the current window (-1 for read-ahead temporarily disabled, 0 for empty current window)
    unsigned long flags //Flags used to control the read-ahead
    unsigned long cache_hit //Number of consecutive cache hits (pages requested by the process and found in the page cache)
    unsigned long prev_page //Index of the last page requested by the process
    unsigned long ahead_start //Index of the first page in the ahead window
    unsigned long ahead_size //Number of pages in the ahead window (0 for an empty ahead window)
    unsigned long ra_pages //Maximum size in pages of a read-ahead window (0 for read-ahead permanently disabled)
    unsigned long mmap_hit //Read-ahead hit counter (used for memory mapped files)
    unsigned long mmap_miss //Read-ahead miss counter (used for memory mapped files)
};
//The page_cache_readahead() function
//The handle_ra_miss() function

#define Writing to a File



#define Writing Dirty Pages to Disk

#endif //end of Reading and writing file
////////////////////////////////////////////////////////////////

#define Memory Mapping
#ifdef Memory Mapping
/*
As already mentioned in the section “Memory Regions” in Chapter 9, a memory
region can be associated with some portion of either a regular file in a disk-based filesystem
or a block device file. This means that an access to a byte within a page of the
memory region is translated by the kernel into an operation on the corresponding
byte of the file. This technique is called memory mapping.
*/
//Two kinds of memory mapping exist:
#define Shared
/*
Each write operation on the pages of the memory region changes the file on disk;
moreover, if a process writes into a page of a shared memory mapping, the
changes are visible to all other processes that map the same file.
*/
#define Private
/*
Meant to be used when the process creates the mapping just to read the file, not
to write it. For this purpose, private mapping is more efficient than shared mapping.
But each write operation on a privately mapped page will cause it to stop
mapping the page in the file. Thus, a write does not change the file on disk, nor
is the change visible to any other processes that access the same file. However,
pages of a private memory mapping that have not been modified by the process
are affected by file updates performed by other processes.
*/

//A process can create a new memory mapping by issuing an mmap( ) system call
enum flag{
    MAP_SHARED;
    MAP_PRIVATE;
};
//To destroy or shrink a memory mapping, the process may use the munmap( ) system call (see the later section “Destroying a Memory Mapping”)


#define Memory Mapping Data Structures
/*
A memory mapping is represented by a combination of the following data structures:
• The inode object associated with the mapped file
• The address_space object of the mapped file
• A file object for each different mapping performed on the file by different
processes
• Avm_area_struct descriptor for each different mapping on the file
• A page descriptor for each page frame assigned to a memory region that maps
the file
*/


#define Creating a Memory Mapping
/*
To create a new memory mapping, a process issues an mmap( ) system call, passing
the following parameters to it:
• A file descriptor identifying the file to be mapped.
• An offset inside the file specifying the first character of the file portion to be
mapped.
• The length of the file portion to be mapped.
• A set of flags. The process must explicitly set either the MAP_SHARED flag or the
MAP_PRIVATE flag to specify the kind of memory mapping requested.*
• A set of permissions specifying one or more types of access to the memory
region: read access (PROT_READ), write access (PROT_WRITE), or execution access
(PROT_EXEC).
• An optional linear address, which is taken by the kernel as a hint of where the
new memory region should start. If the MAP_FIXED flag is specified and the kernel
cannot allocate the new memory region starting from the specified linear
address, the system call fails.

The mmap( ) system call returns the linear address of the first location in the new
memory region.
*/


#define Destroying a Memory Mapping
/*
When a process is ready to destroy a memory mapping, it invokes munmap( ); this system
call can also be used to reduce the size of each kind of memory region. The
parameters used are:
• The address of the first location in the linear address interval to be removed.
• The length of the linear address interval to be removed.
The sys_munmap( ) service routine of the system call essentially invokes the do_
munmap( ) function
*/


#define Demand Paging for Memory Mapping
/*
For reasons of efficiency, page frames are not assigned to a memory mapping right
after it has been created, but at the last possible moment—that is, when the process
attempts to address one of its pages, thus causing a Page Fault exception.
*/


#define Flushing Dirty Memory Mapping Pages to Disk
/*

*/
msync( );


#define Non-Linear Memory Mappings
sys_remap_file_pages();


#endif //end of Memory Mapping

/////////////////////////////////////////////////////////////////////

#define Direct I/O Transfer
#ifdef Direct I/O Transfer




#endif //end of Direct I/O Transfer
//////////////////////////////////////////


#define Asynchronous I/O
#ifdef Asynchronous I/O
/*
The POSIX 1003.1 standard defines a set of library functions—listed in Table 16-4—
for accessing the files in an asynchronous way. “Asynchronous” essentially means
that when a User Mode process invokes a library function to read or write a file, the
function terminates as soon as the read or write operation has been enqueued, possibly
even before the actual I/O data transfer takes place. The calling process can thus
continue its execution while the data is being transferred.
*/

//Table 16-4. The POSIX library functions for asynchronous I/O
aio_read();// Asynchronously reads some data from a file
aio_write();// Asynchronously writes some data into a file
aio_fsync();// Requests a flush operation for all outstanding asynchronous I/O operations (does not block)
aio_error() ;//Gets the error code for an outstanding asynchronous I/O operation
aio_return();// Gets the return code for a completed asynchronous I/O operation
aio_cancel();// Cancels an outstanding asynchronous I/O operation
aio_suspend();// Suspends the process until at least one of several outstanding I/O operations completes

//Table 16-5. Linux system calls for asynchronous I/O
io_setup() ;//Initializes an asynchronous context for the current process
io_submit() ;//Submits one or more asynchronous I/O operations
io_getevents();// Gets the completion status of some outstanding asynchronous I/O operations
io_cancel() ;//Cancels an outstanding I/O operation
io_destroy();// Removes an asynchronous context for the current process

#endif //end of Asynchronous I/O


#endif  // end of 