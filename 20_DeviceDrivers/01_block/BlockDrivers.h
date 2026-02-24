#define __BLOCKDRIVERS__H
#ifdef __BLOCKDRIVERS__H
/*
This deals with I/O drivers for block devices.

*/

#define Block Devices Handling
#ifdef Block Devices Handling
/*
Each operation on a block device driver involves a large number of kernel components.

Let us suppose, for instance, that a process issued a read() system call on some disk
file—we’ll see that write requests are handled essentially in the same way. Here is
what the kernel typically does to service the process request:
1. The service routine of the read() system call activates a suitable VFS function,
passing to it a file descriptor and an offset inside the file. The Virtual Filesystem

is the upper layer of the block device handling architecture, and it provides a
common file model adopted by all filesystems supported by Linux. We have
described at length the VFS layer in Chapter 12.
2. The VFS function determines if the requested data is already available and, if
necessary, how to perform the read operation. Sometimes there is no need to
access the data on disk, because the kernel keeps in RAM the data most recently
read from—or written to—a block device. The disk cache mechanism is
explained in Chapter 15, while details on how the VFS handles the disk operations
and how it interfaces with the disk cache and the filesystems are given in
Chapter 16.
3. Let’s assume that the kernel must read the data from the block device, thus it
must determine the physical location of that data. To do this, the kernel relies on
the mapping layer, which typically executes two steps:
a. It determines the block size of the filesystem including the file and computes
the extent of the requested data in terms of file block numbers. Essentially,
the file is seen as split in many blocks, and the kernel determines the
numbers (indices relative to the beginning of file) of the blocks containing
the requested data.
b. Next, the mapping layer invokes a filesystem-specific function that accesses
the file’s disk inode and determines the position of the requested data on
disk in terms of logical block numbers. Essentially, the disk is seen as split in
blocks, and the kernel determines the numbers (indices relative to the beginning
of the disk or partition) corresponding to the blocks storing the
requested data. Because a file may be stored in nonadjacent blocks on disk,
a data structure stored in the disk inode maps each file block number to a
logical block number.*
We will see the mapping layer in action in Chap


read operation on the block device. It makes use of
the generic block layer, which starts the I/O operations that transfer the
requested data. In general, each I/O operation involves a group of blocks that are
adjacent on disk.

Because the requested data is not necessarily adjacent on disk,
the generic block layer might start several I/O operations. Each I/O operation is
represented by a “block I/O” (in short, “bio”) structure, which collects all information
needed by the lower components to satisfy the request.

The generic block layer hides the peculiarities of each hardware block device,
thus offering an abstract view of the block devices.

Because almost all block
devices are disks, the generic block layer also provides some general data structures
that describe “disks” and “disk partitions

Below the generic block layer, the “I/O scheduler” sorts the pending I/O data
transfer requests according to predefined kernel policies. The purpose of the
scheduler is to group requests of data that lie near each other on the physical
medium.

the block device drivers take care of the actual data transfer by sending
suitable commands to the hardware interfaces of the disk controllers.
*/

/*
As you can see, there are many kernel components that are concerned with data
stored in block devices; each of them manages the disk data using chunks of different
length:
• The controllers of the hardware block devices transfer data in chunks of fixed
length called “sectors.” Therefore, the I/O scheduler and the block device drivers
must manage sectors of data.
• The Virtual Filesystem, the mapping layer, and the filesystems group the disk
data in logical units called “blocks.” A block corresponds to the minimal disk
storage unit inside a filesystem.
• As we will see shortly, block device drivers should be able to cope with “segments”
of data: each segment is a memory page—or a portion of a memory
page—including chunks of data that are physically adjacent on disk.
• The disk caches work on “pages” of disk data, each of which fits in a page frame.
• The generic block layer glues together all the upper and lower components, thus
it knows about sectors, blocks, segments, and pages of data.
*/

/*
lower kernel components that handle the block
devices—generic block layer, I/O scheduler, and block device drivers—thus we focus
our attention on sectors, blocks, and segments.
*/

#define Sectors
/*
To achieve acceptable performance, hard disks and similar devices transfer several
adjacent bytes at once. Each data transfer operation for a block device acts on a
group of adjacent bytes called a sector.

the size of a sector is conventionally set to 512 bytes.
*/


#define blocks
/*
While the sector is the basic unit of data transfer for the hardware devices, the block
is the basic unit of data transfer for the VFS and, consequently, for the filesystems.

the block size must be a power of 2 and cannot be larger than a page frame.

it must be a multiple of the sector size, because each block must include
an integral number of sectors. Therefore, on 80 × 86 architecture, the permitted
block sizes are 512, 1,024, 2,048, and 4,096 bytes.

each read or
write operation issued on a block device file is a “raw” access that bypasses the diskbased
filesystem; the kernel executes it by using blocks of largest size (4,096 bytes).

Each block requires its own block buffer, which is a RAM memory area used by the
kernel to store the block’s content.

Each buffer has a “buffer head” descriptor of type buffer_head. This descriptor contains
all the information needed by the kernel to know how to handle the buffer; thus,
before operating on each buffer, the kernel checks its buffer head.
*/




#define segments
/*
each disk I/O operation consists of transferring the contents of some
adjacent sectors from—or to—some RAM locations

In almost all cases, the data
transfer is directly performed by the disk controller with a DMA operation (see the
section “Direct Memory Access (DMA)” in Chapter 13). The block device driver simply
triggers the data transfer by sending suitable commands to the disk controller;
once the data transfer is finished, the controller raises an interrupt to notify the block
device driver.


*/



#endif // end of Block Devices Handling
/* ----------------------------------------------------------- */


#define The Generic Block Layer
#ifdef The Generic Block Layer
/*
The generic block layer is a kernel component that handles the requests for all block
devices in the system. Thanks to its functions, the kernel may easily:
• Put data buffers in high memory—the page frame(s) will be mapped in the kernel
linear address space only when the CPU must access the data, and will be
unmapped right after.
• Implement—with some additional effort—a “zero-copy” schema, where disk
data is directly put in the User Mode address space without being copied to kernel
memory first; essentially, the buffer used by the kernel for the I/O transfer
lies in a page frame mapped in the User Mode linear address space of a process.
• Manage logical volumes—such as those used by LVM (the Logical Volume Manager)
and RAID (Redundant Array of Inexpensive Disks): several disk partitions,
even on different block devices, can be seen as a single partition.
• Exploit the advanced features of the most recent disk controllers, such as large
onboard disk caches, enhanced DMA capabilities, onboard scheduling of the I/
O transfer requests, and so on.
*/

#define The Bio Structure
/*
The core data structure of the generic block layer is a descriptor of an ongoing I/O
block device operation called bio. Each bio essentially includes an identifier for a disk
storage area—the initial sector number and the number of sectors included in the
storage area—and one or more segments describing the memory areas involved in
the I/O operation. A bio is implemented by the bio data structure, whose fields are
listed in Table 14-1.
*/
struct bio
{
    sector_t bi_sector //First sector on disk of block I/O operation
    struct bio * bi_next //Link to the next bio in the request queue
    struct block_device * bi_bdev //Pointer to block device descriptor
    unsigned long bi_flags //Bio status flags
    unsigned long bi_rw //I/O operation flags
    unsigned short bi_vcnt// Number of segments in the bio’s bio_vec array
    unsigned short bi_idx //Current index in the bio’s bio_vec array of segments
    unsigned short bi_phys_segments //Number of physical segments of the bio after merging
    unsigned short bi_hw_segments/// Number of hardware segments after merging
    unsigned int bi_size //Bytes (yet) to be transferred
    unsigned int bi_hw_front_size //Used by the hardware segment merge algorithm
    unsigned int bi_hw_back_size //Used by the hardware segment merge algorithm
    unsigned int bi_max_vecs //Maximum allowed number of segments in the bio’s bio_ vecarray
    struct bio_vec * bi_io_vec //Pointer to the bio’s bio_vec array of segments
    bio_end_io_t * bi_end_io //Method invoked at the end of bio’s I/O operation
    atomic_t bi_cnt //Reference counter for the bio
    void * bi_private //Pointer used by the generic block layer and the I/O completion method of the block device driver
    bio_destructor_t * bi_destructor //Destructor method (usually bio_destructor()) invoked when the bio is being freed
};


/*
Each segment in a bio is represented by a bio_vec data structure, whose fields are
listed in Table 14-2. The bi_io_vec field of the bio points to the first element of an
array of bio_vec data structures, while the bi_vcnt field stores the current number of
elements in the array.
*/
struct bio_vec
{
    struct page * bv_page //Pointer to the page descriptor of the segment’s page frame
    unsigned int bv_len //Length of the segment in bytes
    unsigned int bv_offset //Offset of the segment’s data in the page frame
};


#define Representing Disks and Disk Partitions
/*
disk is a logical block device that is handled by the generic block layer. Usually a
disk corresponds to a hardware block device such as a hard disk, a floppy disk, or a
CD-ROM disk. However, a disk can be a virtual device built upon several physical
disk partitions, or a storage area living in some dedicated pages of RAM. In any case,
the upper kernel components operate on all disks in the same way thanks to the services
offered by the generic block layer.
A disk is represented by the gendisk object, whose fields are shown in Table 14-3.
*/
struct gendisk
{
    int major //Major number of the disk
    int first_minor //First minor number associated with the disk
    int minors //Range of minor numbers associated with the disk
    char [32] disk_name //Conventional name of the disk (usually, the canonical name of the corresponding device file)
    struct hd_struct ** part //Array of partition descriptors for the disk
    struct block_device_operations *fops //Pointer to a table of block device methods
    struct request_queue * queue //Pointer to the request queue of the disk (see “Request Queue Descriptors” later in this chapter)
    void * private_data //Private data of the block device driver
    sector_t capacity //Size of the storage area of the disk (in number of sectors)
    int flags //Flags describing the kind of disk (see below)
    char [64] devfs_name //Device filename in the (nowadays deprecated) devfs special filesystem
    int number// No longer used
    struct device * driverfs_dev //Pointer to the device object of the disk’s hardware device (see the section “Components of the Device Driver Model” in Chapter 13)
    struct kobject kobj //Embedded kobject (see the section “Kobjects” inChapter 13)
    struct timer_rand_state * random //Pointer to a data structure that records the timing of the disk’s interrupts; used by the kernel built-in random number generator
    int policy //Set to 1 if the disk is read-only (write operations forbidden), 0 otherwise
    atomic_t sync_io //Counter of sectors written to disk, used only for RAID
    unsigned long stamp //Timestamp used to determine disk queue usage statistics
    unsigned long stamp_idle //Same as above
    int in_flight //Number of ongoing I/O operations
    struct disk_stats * dkstats //Statistics about per-CPU disk usage
};

/*
The fops field of the gendisk object points to a block_device_operations table, which
stores a few custom methods for crucial operations of the block device (see
Table 14-4).
*/
open ;//Opening the block device file
release;// Closing the last reference to a block device file
ioctl;// Issuing an ioctl() system call on the block device file (uses the big kernel lock)
compat_ioctl;// Issuing an ioctl() system call on the block device file (does not use the big kernel lock)
media_changed;// Checking whether the removable media has been changed (e.g., floppy disk)
revalidate_disk;// Checking whether the block device holds valid data


/*
If a disk is split in partitions, their layout is kept in an array of hd_struct structures
whose address is stored in the part field of the gendisk object. The array is indexed
by the relative index of the partition inside the disk. The fields of the hd_struct
descriptor are listed in Table 14-5.
*/
struct hd_struct
{
    sector_t start_sect //Starting sector of the partition inside the disk
    sector_t nr_sects //Length of the partition (number of sectors)
    struct kobject //kobj Embedded kobject (see the section “Kobjects” in Chapter 13)
    unsigned int reads //Number of read operations issued on the partition
    unsigned int read_sectors //Number of sectors read from the partition
    unsigned int writes //Number of write operations issued on the partition
    unsigned int write_sectors //Number of sectors written into the partition
    int policy //Set to 1 if the partition is read-only, 0 otherwise
    int partno //The relative index of the partition inside the disk
};

#define Submitting a Request

#endif // end of The Generic Block Layer
/* ----------------------------------------------------------- */



#define The I/O Scheduler
#ifdef TThe I/O Scheduler
/*
Although block device drivers are able to transfer a single sector at a time, the block
I/O layer does not perform an individual I/O operation for each sector to be accessed
on disk; this would lead to poor disk performance, because locating the physical
position of a sector on the disk surface is quite time-consuming. Instead, the kernel
tries, whenever possible, to cluster several sectors and handle them as a whole, thus
reducing the average number of head movements.

When a kernel component wishes to read or write some disk data, it actually creates
a block device request. That request essentially describes the requested sectors and
the kind of operation to be performed on them (read or write). However, the kernel
does not satisfy a request as soon as it is created—the I/O operation is just scheduled
and will be performed at a later time. This artificial delay is paradoxically the
crucial mechanism for boosting the performance of block devices. When a new block
data transfer is requested, the kernel checks whether it can be satisfied by slightly
enlarging a previous request that is still waiting (i.e., whether the new request can be
satisfied without further seek operations). Because disks tend to be accessed sequentially,
this simple mechanism is very effective.

To keep the block device driver from being suspended, each I/O operation is processed
asynchronously. In particular, block device drivers are interrupt-driven (see
the section “Monitoring I/O Operations” in the previous chapter): the generic block
layer invokes the I/O scheduler to create a new block device request or to enlarge an
already existing one and then terminates. The block device driver, which is activated
at a later time, invokes the strategy routine to select a pending request and satisfy it
by issuing suitable commands to the disk controller. When the I/O operation terminates,
the disk controller raises an interrupt and the corresponding handler invokes
the strategy routine again, if necessary, to process another pending request.
Each block device driver maintains its own request queue, which contains the list of
pending requests for the device. If the disk controller is handling several disks, there
is usually one request queue for each physical block device. I/O scheduling is performed
separately on each request queue, thus increasing disk performance.


*/

//Request Queue Descriptors
/*Each request queue is represented by means of a large request_queue data structure
whose fields are listed in Table 14-6.*/
struct request_queue
{
   struct list_head queue_head //List of pending requests
    struct request * last_merge //Pointer to descriptor of the request in the queue to be considered first for possible merging
    elevator_t * elevator //Pointer to the elevator object (see the later section “I/O Scheduling Algorithms”)
    struct request_list rq //Data structure used for allocation of request descriptors
    request_fn_proc * request_fn //Method that implements the entry point of the strategy routine of the driver
    merge_request_fn * back_merge_fn //Method to check whether it is possible to merge a bio to the last request in the queue
    merge_request_fn * front_merge_fn //Method to check whether it is possible to merge a bio to the first request in the queue
    merge_requests_fn * merge_requests_fn //Method to attempt merging two adjacent requests in the queue
    make_request_fn * make_request_fn //Method invoked when a new request has to be inserted in the queue
    prep_rq_fn * prep_rq_fn //Method to build the commands to be sent to the hardware device to process this request
    unplug_fn * unplug_fn //Method to unplug the block device (see the section “Activating the Block Device Driver” later in the chapter)
    merge_bvec_fn * merge_bvec_fn //Method that returns the number of bytes that can be inserted into an existing bio when adding a new segment (usually undefined)
    activity_fn * activity_fn //Method invoked when a request is added to a queue (usually undefined)
    issue_flush_fn * issue_flush_fn //Method invoked when a request queue is flushed (the queue is emptied by processing all requests in a row)
    struct timer_list unplug_timer //Dynamic timer used to perform device plugging (see the later section “Activating the Block Device Driver”)
    int unplug_thresh// If the number of pending requests in the queue exceeds this value, the device is immediately unplugged (default is 4)
    unsigned long unplug_delay //Time delay before device unplugging (default is 3 milliseconds)
    struct work_struct unplug_work //Work queue used to unplug the device (see the later section “Activating the Block Device Driver”)
    struct backing_dev_info backing_dev_info //See the text following this table
    void * queuedata //Pointer to private data of the block device driver
    void * activity_data //Private data used by the activity_fn method
    unsigned long bounce_pfn //Page frame number above which buffer bouncing must be used (see the section “Submitting a Request” later in this chapter)
    int bounce_gfp //Memory allocation flags for bounce buffers
    unsigned long queue_flags //Set of flags describing the queue status
    spinlock_t * queue_lock //Pointer to request queue lock
    struct kobject kobj //Embedded kobject for the request queue
    unsigned long nr_requests //Maximum number of requests in the queue
    unsigned int nr_congestion_on //Queue is considered congested if the number of pending requests rises above this threshold
    unsigned int nr_congestion_off //Queue is considered not congested if the number of pending requests falls below this threshold
    unsigned int nr_batching //Maximum number (usually 32) of pending requests that can be submitted even when the queue is full  by a special “batcher” process
    unsigned short max_sectors //Maximum number of sectors handled by a single request (tunable)
    unsigned short max_hw_sectors //Maximum number of sectors handled by a single request (hardware constraint)
    unsigned short max_phys_segments //Maximum number of physical segments handled by a single request
    unsigned short max_hw_segments //Maximum number of hardware segments handled by a single request (the maximum number of distinct memory areas in a scatter-gather DMA operation)
    unsigned short hardsect_size //Size in bytes of a sector
    unsigned int max_segment_size //Maximum size of a physical segment (in bytes)
    unsigned long seg_boundary_mask //Memory boundary mask for segment merging
    unsigned int dma_alignment //Alignment bitmap for initial address and length of DMA buffers (default 511)
    struct blk_queue_tag * queue_tags //Bitmap of free/busy tags (used for tagged requests)
    atomic_t refcnt //Reference counter of the queue
    unsigned int in_flight //Number of pending requests in the queue
    unsigned int sg_timeout //User-defined command time-out (used only by SCSI generic devices)
    unsigned int sg_reserved_size //Essentially unused
    struct list_head drain_list //Head of a list of requests temporarily delayed until the I/O scheduler is dynamically replaced
};


//Request Descriptors
/*Each pending request for a block device is represented by a request descriptor, which
is stored in the request data structure illustrated in Table 14-7.*/
struct request
{
    struct list_head queuelist //Pointers for request queue list
    unsigned long flags //Flags of the request (see below)
    sector_t sector// Number of the next sector to be transferred
    unsigned long nr_sectors //Number of sectors yet to be transferred in the whole request
    unsigned int current_nr_sectors //Number of sectors in the current segment of the current bio yet to be transferred
    sector_t hard_sector //Number of the next sector to be transferred
    unsigned long hard_nr_sectors// Number of sectors yet to be transferred in the whole request (updated by the generic block layer)
    unsigned int hard_cur_sectors //Number of sectors in the current segment of the current bio yet to be transferred (updated by the generic block layer)
    struct bio * bio //First bio in the request that has not been completely transferred
    struct bio * biotail //Last bio in the request list
    void * elevator_private //Pointer to private data for the I/O scheduler
    int rq_status //Request status: essentially, either RQ_ACTIVE or RQ_INACTIVE
    struct gendisk * rq_disk //The descriptor of the disk referenced by the request
    int errors //Counter for the number of I/O errors that occurred on the current transfer
    unsigned long start_time// Request’s starting time (in jiffies)
    unsigned short nr_phys_segments //Number of physical segments of the request
    unsigned short nr_hw_segments //Number of hardware segments of the request
    int tag //Tag associated with the request (only for hardware devices supporting multiple outstanding data transfers)
    char * buffer //Pointer to the memory buffer of the current data transfer (NULL if the buffer is in highmemory)
    int ref_count //Reference counter for the request
    request_queue_t * q //Pointer to the descriptor of the request queue containing the request
    struct request_list * rl //Pointer to request_list data structure
    struct completion * waiting //Completion for waiting for the end of the data transfers (see the section “Completions” in Chapter 5)
    void * special //Pointer to data used when the request includes a “special” command to the hardware device
    unsigned int cmd_len //Length of the commands in the cmd field
    unsigned char [] cmd //Buffer containing the pre-built commands prepared by the request queue’s prep_rq_fn method
    unsigned int //data_len Usually, the length of data in the buffer pointed to by the data field
    void * data //Pointer used by the device driver to keep track of the data to be transferred
    unsigned int //sense_len Length of buffer pointed to by the sense field (0 if the sense field is NULL)
    void * sense //Pointer to buffer used for output of sense commands unsigned int timeout Request’s time-out
    struct request_pm_state *pm //Pointer to a data structure used for power-management commands
};

/*
The flags field stores a large number of flags, which are listed in Table 14-8. The
most important one is, by far, REQ_RW, which determines the direction of the data
transfer.
*/

enum request_flag{
    REQ_RW //Direction of data transfer: READ (0) or WRITE (1)
    REQ_FAILFAST// Requests says to not retry the I/O operation in case of error
    REQ_SOFTBARRIER //Request acts as a barrier for the I/O scheduler
    REQ_HARDBARRIER// Request acts as a barrier for the I/O scheduler and the device driver—it should be processed after older requests and before newer ones
    REQ_CMD //Request includes a normal read or write I/O data transfer
    REQ_NOMERGE// Request should not be extended or merged with other requests
    REQ_STARTED //Request is being processed
    REQ_DONTPREP// Do not invoke the prep_rq_fn request queue’s method to prepare in advance the commands to be sent to the hardware device
    REQ_QUEUED //Request is tagged—that is, it refers to a hardware device that can manage many outstanding data transfers at the same time
    REQ_PC //Request includes a direct command to be sent to the hardware device
    REQ_BLOCK_PC //Same as previous flag, but the command is included in a bio
    REQ_SENSE //Request includes a “sense” request command (for SCSI and ATAPI devices)
    REQ_FAILED //Set when a sense or direct command in the request did not work as expected
    REQ_QUIET //Request says to not generate kernel messages in case of I/O errors
    REQ_SPECIAL// Request includes a special command for the hardware device (e.g., drive reset)
    REQ_DRIVE_CMD //Request includes a special command for IDE disks
    REQ_DRIVE_TASK //Request includes a special command for IDE disks
    REQ_DRIVE_TASKFILE //Request includes a special command for IDE disks
    REQ_PREEMPT// Request replaces the current request in front of the queue (only for IDE disks)
    REQ_PM_SUSPEND //Request includes a power-management command to suspend the hardware device
    REQ_PM_RESUME //Request includes a power-management command to awaken the hardware device
    REQ_PM_SHUTDOWN //Request includes a power-management command to switch off the hardware device
    REQ_BAR_PREFLUSH //Request includes a “flush queue” command to be sent to the disk controller
    REQ_BAR_POSTFLUSH //Request includes a “flush queue” command, which has been sent to the disk controller
};

#define Managing the allocation of request descriptors
#define Avoiding request queue congestion



#define Activating the Block Device Driver
/*
As we saw earlier, it’s expedient to delay activation of the block device driver in
order to increase the chances of clustering requests for adjacent blocks. The delay is
accomplished through a technique known as device plugging and unplugging.* As
long as a block device driver is plugged, the device driver is not activated even if
there are requests to be processed in the driver’s queues.
The blk_plug_device() function plugs a block device—or more precisely, a request
queue serviced by some block device driver. Essentially, the function receives as an
argument the address q of a request queue descriptor. It sets the QUEUE_FLAG_PLUGGED
bit in the q->queue_flags field; then, it restarts the dynamic timer embedded in the q->
unplug_timer field.
The blk_remove_plug() function unplugs a request queue q: it clears the QUEUE_
FLAG_PLUGGED flag and cancels the execution of the q->unplug_timer dynamic
timer. This function can be explicitly invoked by the kernel when all mergeable
requests “in sight” have been added to the queue. Moreover, the I/O scheduler
unplugs a request queue if the number of pending requests in the queue exceeds
the value stored in the unplug_thres field of the request queue descriptor (by
default, 4).
*/

#define I/O Scheduling Algorithms
/*
When a new request is added to a request queue, the generic block layer invokes the
I/O scheduler to determine that exact position of the new element in the queue. The
I/O scheduler tries to keep the request queue sorted sector by sector. If the requests
to be processed are taken sequentially from the list, the amount of disk seeking is significantly
reduced because the disk head moves in a linear way from the inner track
to the outer one (or vice versa) instead of jumping randomly from one track to
another. This heuristic is reminiscent of the algorithm used by elevators when dealing
with requests coming from different floors to go up or down. The elevator moves
in one direction; when the last booked floor is reached in one direction, the elevator
changes direction and starts moving in the other direction. For this reason, I/O
schedulers are also called elevators.
Under heavy load, an I/O scheduling algorithm that strictly follows the order of the
sector numbers is not going to work well. In this case, indeed, the completion time of
a data transfer strongly depends on the physical position of the data on the disk.
Thus, if a device driver is processing requests near the top of the queue (lower sector
numbers), and new requests with low sector numbers are continuously added to the
queue, then the requests in the tail of the queue can easily starve. I/O scheduling
algorithms are thus quite sophisticated.
*/

//four different types of I/O schedulers—or elevators— called
#define Anticipatory
#define Deadline
#define CFQ //(Complete Fairness Queueing)
#define Noop //(No Operation)


#define Issuing a Request to the I/O Scheduler
/*
As seen in the section “Submitting a Request” earlier in this chapter, the generic_
make_request() function invokes the make_request_fn method of the request queue
descriptor to transmit a request to the I/O scheduler. This method is usually implemented
by the __make_request() function; it receives as its parameters a request_
queue descriptor q and a bio descriptor bio, and it performs the  operations
*/

blk_queue_bounce();

#endif // end of The I/O Scheduler
/* ----------------------------------------------------------- */



#define Block Device Drivers
#ifdef Block Device Drivers
/*
Block device drivers are the lowest component of the Linux block subsystem. They
get requests from I/O scheduler, and do whatever is required to process them.

Block device drivers are, of course, integrated within the device driver model
described in the section “The Device Driver Model” in Chapter 13. Therefore, each
of them refers to a device_driver descriptor; moreover, each disk handled by the
driver is associated with a device descriptor. These descriptors, however, are rather
generic: the block I/O subsystem must store additional information for each block
device in the system.
*/

#define Block Devices
/*
A block device driver may handle several block devices. For instance, the IDE device
driver can handle several IDE disks, each of which is a separate block device. Furthermore,
each disk is usually partitioned, and each partition can be seen as a logical
block device. Clearly, the block device driver must take care of all VFS system calls
issued on the block device files associated with the corresponding block devices.
Each block device is represented by a block_device descriptor, whose fields are listed
in Table 14-9.
*/
struct block_device
{
    dev_t bd_dev //Major and minor numbers of the block device
    struct inode * bd_inode //Pointer to the inode of the file associated with the block device in the bdev filesystem
    int bd_openers// Counter of how many times the block device has been opened
    struct semaphore bd_sem //Semaphore protecting the opening and closing of the block device
    struct semaphore bd_mount_sem //Semaphore used to forbid new mounts on the block device
    struct list_head bd_inodes //Head of a list of inodes of opened block device files for this block device
    void * bd_holder //Current holder of block device descriptor
    int bd_holders //Counter for multiple settings of the bd_holder field
    struct block_device * bd_contains //If block device is a partition, it points to the block device descriptor of the whole disk; otherwise, it points to this block device descriptor
    unsigned bd_block_size //Block size
    struct hd_struct * bd_part //Pointer to partition descriptor (NULL if this block device is not a partition)
    unsigned bd_part_count //Counter of how many times partitions included in this block device have been opened
    int bd_invalidated //Flag set when the partition table on this block device needs to be read
    struct gendisk * bd_disk //Pointer to gendisk structure of the disk underlying this block device
    struct list_head * bd_list //Pointers for the block device descriptor list
    struct backing_dev_info * bd_inode_back ing_dev_info //Pointer to a specialized backing_dev_info descriptor for this block device (usually NULL)
    unsigned long bd_private //Pointer to private data of the block device holder
};

#define Accessing a block device


#define Device Driver Registration and Initialization
//Defining a custom driver descriptor
//Reserving the major number
//Initializing the custom descriptor
//Initializing the gendisk descriptor
//Initializing the table of block device methods
//Allocating and initializing a request queue
//Setting up the interrupt handler
//Registering the disk

#define The Strategy Routine


#define The Interrupt Handler


#endif // end of “Block Device Drivers
/* ----------------------------------------------------------- */



#define Opening a Block Device File
#ifdef Opening a Block Device File
/*
The kernel opens a block device file every time that a filesystem is mounted over a
disk or partition, every time that a swap partition is activated, and every time that a
User Mode process issues an open() system call on a block device file. In all cases,
the kernel executes essentially the same operations: it looks for the block device
descriptor (possibly allocating a new descriptor if the block device is not already in
use), and sets up the file operation methods for the forthcoming data transfers.

In the section “VFS Handling of Device Files” in Chapter 13, we described how the
dentry_open() function customizes the methods of the file object when a device file is
opened. In this case, the f_op field of the file object is set to the address of the def_
blk_fops table, whose content is shown in Table 14-10.
*/
//Table 14-10. The default block device file operations (def_blk_fops table)
//Method Function
/*
open blkdev_open()
release blkdev_close()
llseek block_llseek()
read generic_file_read()
write blkdev_file_write()
aio_read generic_file_aio_read()
aio_write blkdev_file_aio_write()
mmap generic_file_mmap()
fsync block_fsync()
ioctl block_ioctl()
compat-ioctl compat_blkdev_ioctl()
readv generic_file_readv()
writev generic_file_write_nolock()
sendfile generic_file_sendfile()
*/

/*
Here we are only concerned with the open method, which is invoked by the dentry_
open() function. The blkdev_open() function receives as its parameters inode and
filp, which store the addresses of the inode and file objects respectively; the function
essentially executes the  steps:
*/

#endif // end of Opening a Block Device File
/* ----------------------------------------------------------- */


#endif // end of __BLOCKDRIVERS__H
