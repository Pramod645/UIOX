#define __VIRTUAL_FILESYSTEM__H
#ifdef __VIRTUAL_FILESYSTEM__H
//it come under scfc 
/*
manages to support multiple filesystem types in the same way other
Unix variants do, through a concept called the Virtual Filesystem
The idea behind the Virtual Filesystem is to put a wide range of information in the
kernel to represent many different types of filesystems;

*/
#define native Linux filesystem
#define NTFS filesystem


//five standard Unix file types
#define REGULAR_FILE
#define DIRECOTIRES
#define SYMBOLIC_LINKS
#define 4
#define 5





//
#define The Role of the Virtual Filesystem //(VFS)
/*
The Virtual Filesystem (also known as Virtual Filesystem Switch or VFS) is a kernel
software layer that handles all system calls related to a standard Unix filesystem. Its
main strength is providing a common interface to several kinds of filesystems.
*/

// tree file system
//1.
#define Disk-based filesystems
//filesystems supported by the VFS are: allmost all

//2.
#define Network filesystems
/*
These allow easy access to files included in filesystems belonging to other networked
computers.
*/

//3.
#define Special filesystems
/*
These do not manage disk space, either locally or remotely. The /proc filesystem
is a typical example of a special filesystem
*/



//common file model consists of the following object types
#define FILE_MODEL
#ifdef FILE_MODEL
#define superblock object
/*
Stores information concerning a mounted filesystem. For disk-based filesystems,
this object usually corresponds to a filesystem control block stored on disk.
*/
#define inode object
/*
Stores general information about a specific file. For disk-based filesystems, this
object usually corresponds to a file control block stored on disk. Each inode
object is associated with an inode number, which uniquely identifies the file
within the filesystem.
*/
#define file object
/*
Stores information about the interaction between an open file and a process.
This information exists only in kernel memory during the period when a process
has the file open.
*/
#define dentry object
/*
Stores information about the linking of a directory entry (that is, a particular
name of the file) with the corresponding file. Each disk-based filesystem stores
this information in its own particular way on disk.
*/

#define dentry chache

#endif // end of FILE_MODEL


#define System Calls Handled by the VFS
/*
Table 12-1. Some system calls handled by the VFS
System call name Description
mount( ) umount( ) umount2() Mount/unmount filesystems
sysfs( ) Get filesystem information
statfs( ) fstatfs( ) statfs64() fstatfs64()
ustat( )
Get filesystem statistics
chroot( ) pivot_root() Change root directory
chdir( ) fchdir( ) getcwd( ) Manipulate current directory
mkdir( ) rmdir( ) Create and destroy directories
getdents( ) getdents64() readdir( ) link( )
unlink( ) rename( ) lookup_dcookie()
Manipulate directory entries
readlink( ) symlink( ) Manipulate soft links
chown( ) fchown( ) lchown( ) chown16()
fchown16() lchown16()
Modify file owner
chmod( ) fchmod( ) utime( ) Modify file attributes
stat( ) fstat( ) lstat( ) access( ) oldstat()
oldfstat() oldlstat() stat64() lstat64()
fstat64()
Read file status
open( ) close( ) creat( ) umask( ) Open, close, and create files
dup( ) dup2( ) fcntl() fcntl64() Manipulate file descriptors
select( ) poll( ) Wait for events on a set of file descriptors
truncate( ) ftruncate( ) truncate64()
ftruncate64()
Change file size
lseek( ) _llseek( ) Change file pointer
read( ) write( ) readv( ) writev( ) sendfile( )
sendfile64() readahead()
Carry out file I/O operations
io_setup() io_submit() io_getevents()
io_cancel() io_destroy()
Asynchronous I/O (allows multiple outstanding
read and write requests)
pread64() pwrite64() Seek file and access it
mmap( ) mmap2() munmap( ) madvise() mincore()
remap_file_pages()
Handle file memory mapping
fdatasync( ) fsync( ) sync( ) msync( ) Synchronize file data
flock( ) Manipulate file lock
setxattr() lsetxattr() fsetxattr() getxattr()
lgetxattr() fgetxattr() listxattr() llistxattr()
flistxattr() removexattr() lremovexattr()
fremovexattr()
Manipulate file extended attribute
*/

#define VFS Data Structures
#ifdef VFS Data Structures
/*
Each VFS object is stored in a suitable data structure, which includes both the object
attributes and a pointer to a table of object methods.
*/
// supe block object, All superblock objects are linked in a circular doubly linked list.
struct uper_block{
    struct list_head s_list;  //Pointers for superblock list
    dev_t s_dev;              //Device identifier
    unsigned long s_blocksize //Block size in bytes
    unsigned long s_old_blocksize // Block size in bytes as reported by the underlying block device driver
    unsigned char s_blocksize_bits //Block size in number of bits
    unsigned char s_dirt           //Modified (dirty) flag
    unsigned long long s_maxbytes //Maximum size of the files
struct file_system_type *s_type //Filesystem type
struct super_operations *s_op //Superblock methods
struct dquot_operations * dq_op Disk quota handling methods
struct quotactl_ops *s_qcop //Disk quota administration methods
struct export_operations *s_export_op //Export operations used by network filesystems
unsigned long s_flags //Mount flags
unsigned long s_magic //Filesystem magic number
struct dentry * s_root //Dentry object of the filesystem’s root directory
struct rw_semaphore s_umount //Semaphore used for unmounting
struct semaphore s_lock //Superblock semaphore
int s_count //Reference counter
int s_syncing //Flag indicating that inodes of the superblock are being synchronized
int s_need_sync_fs //Flag used when synchronizing the superblock’s mounted filesystem
atomic_t s_active //Secondary reference counter
void * s_security //Pointer to superblock security structure
struct xattr_handler **s_xattr //Pointer to superblock extended attribute structure
struct list_head s_inodes //List of all inodes
struct list_head s_dirty //List of modified inodes
struct list_head s_io //List of inodes waiting to be written to disk
struct hlist_head s_anon //List of anonymous dentries for handling remote network filesystems
struct list_head s_files //List of file objects
struct block_device * s_bdev //Pointer to the block device driver descriptor
struct list_head s_instances //Pointers for a list of superblock objects of a given filesystem type (see the later section “Filesystem Type Registration”)
struct quota_info s_dquot //Descriptor for disk quota
int s_frozen //Flag used when freezing the filesystem (forcing it to a consistent state)
wait_queue_head_t s_wait_unfrozen //Wait queue where processes sleep until the filesystem is unfrozen
char[] s_id //Name of the block device containing the superblock
void * s_fs_info //Pointer to superblock information of a specific filesystem
struct semaphore s_vfs_rename_sem //Semaphore used by VFS when renaming files across directories
u32 s_time_gran //Timestamp’s granularity (in nanoseconds)
};

/*superblock operations, which implement higher-level operations
like deleting files or mounting disks. They are listed in the order they appear
in the super_operations table*/
alloc_inode(sb);
destroy_inode(inode);
read_inode(inode);
dirty_inode(inode);
write_inode(inode, flag);
put_inode(inode);
drop_inode(inode);
delete_inode(inode);
put_super(super);
write_super(super);
sync_fs(sb, wait);
write_super_lockfs(super);
unlockfs(super);
statfs(super, buf);
remount_fs(super, flags, data);
clear_inode(inode);
umount_begin(super);
show_options(seq_file, vfsmount);
quota_read(super, type, data, size, offset);
quota_write(super, type, data, size, offset);



//Inode Objects
/*
All information needed by the filesystem to handle a file is included in a data structure
called an inode. A filename is a casually assigned label that can be changed, but
the inode is unique to the file and remains the same as long as the file exists. An
inode object in memory consists of an inode structure whose fields are described
*/
struct inode_struct{
    struct hlist_node i_hash //Pointers for the hash list
    struct list_head i_list //Pointers for the list that describes the inode’s current state
    struct list_head i_sb_list //Pointers for the list of inodes of the superblock
    struct list_head i_dentry //The head of the list of dentry objects referencing this inode
    unsigned long i_ino //inode number
    atomic_t i_count //Usage counter
    umode_t i_mode //File type and access rights
    unsigned int i_nlink //Number of hard links
    uid_t i_uid //Owner identifier
    gid_t i_gid //Group identifier
    dev_t i_rdev //Real device identifier
    loff_t i_size //File length in bytes
    struct timespec i_atime //Time of last file access
    struct timespec i_mtime //Time of last file write
    struct timespec i_ctime //Time of last inode change
    unsigned int i_blkbits //Block size in number of bits
    unsigned long i_blksize //Block size in bytes
    unsigned long i_version //Version number, automatically increased after each use
    unsigned long i_blocks //Number of blocks of the file
    unsigned short i_bytes //Number of bytes in the last block of the file
    unsigned char i_sock //Nonzero if file is a socket
    spinlock_t i_lock //Spin lock protecting some fields of the inode
    struct semaphore i_sem //inode semaphore
    struct rw_semaphore i_alloc_sem //Read/write semaphore protecting against race conditions in direct I/O file operations
    struct inode_operations * i_op //inode operations
    struct file_operations * i_fop //Default file operations
    struct super_block * i_sb //Pointer to superblock object
    struct file_lock * i_flock //Pointer to file lock list
    struct address_space * i_mapping //Pointer to an address_space object (see Chapter 15)
    struct address_space i_data //address_space object of the file
    struct dquot * [] i_dquot /inode disk quotas
    struct list_head i_devices //Pointers for a list of inodes relative to a specific character or block device (see Chapter 13)
    struct pipe_inode_info * i_pipe ///Used if the file is a pipe (see Chapter 19)
    struct block_device * i_bdev //Pointer to the block device driver
    struct cdev * i_cdev //Pointer to the character device driver
    int i_cindex //Index of the device file within a group of minor numbers
    __u32 i_generation //inode version number (used by some filesystems)
    unsigned long i_dnotify_mask //Bit mask of directory notify events
    struct dnotify_struct * i_dnotify //Used for directory notifications
    unsigned long i_state //inode state flags
    unsigned long dirtied_when //Dirtying time (in ticks) of the inode
    unsigned int i_flags //Filesystem mount flags
    atomic_t i_writecount //Usage counter for writing processes
    void * i_security //Pointer to inode’s security structure
    void * u.generic_ip //Pointer to private data
    seqcount_t i_size_seqcount //Sequence counter used in SMP systems to get consistent values for i_size

};

#define I_DIRTY

#ifdef I_DIRTY
#define I_DIRTY
#elif
#define I_DIRTY_SYNC
#define I_DIRTY_DATASYNC
#define I_DIRTY_PAGES
#endif

#define I_LOCK
#define I_FREEING
#define I_CLEAR
#define I_NEW


//Here are the inode operations in the order they appear in the inode_operations table
create(dir, dentry, mode, nameidata);
lookup(dir, dentry, nameidata);
link(old_dentry, dir, new_dentry);
unlink(dir, dentry);
symlink(dir, dentry, symname);
mkdir(dir, dentry, mode);
rmdir(dir, dentry);
mknod(dir, dentry, mode, rdev);
rename(old_dir, old_dentry, new_dir, new_dentry);
readlink(dentry, buffer, buflen);
follow_link(inode, nameidata);
put_link(dentry, nameidata);
truncate(inode);
permission(inode, mask, nameidata);
setattr(dentry, iattr);
getattr(mnt, dentry, kstat);
setxattr(dentry, name, value, size, flags);
getxattr(dentry, name, buffer, size);
listxattr(dentry, buffer, size);
removexattr(dentry, name);



//File Objects
/*
A file object describes how a process interacts with a file it has opened. The object is
created when the file is opened and consists of a file structure, whose fields are
described in Table 12-4. Notice that file objects have no corresponding image on
disk, and hence no “dirty” field is included in the file structure to specify that the
file object has been modified.
*/
struct file_struct
{
    struct list_head f_list //Pointers for generic file object list
    struct dentry * f_dentry //dentry object associated with the file
    struct vfsmount * f_vfsmnt //Mounted filesystem containing the file
    struct file_operations * f_op //Pointer to file operation table
    atomic_t f_count //File object’s reference counter
    unsigned int f_flags //Flags specified when opening the file
    mode_t f_mode //Process access mode
    int f_error //Error code for network write operation
    loff_t f_pos //Current file offset (file pointer)
    struct fown_struct f_owner //Data for I/O event notification via signals
    unsigned int f_uid //User’s UID
    unsigned int f_gid //User group ID
    struct file_ra_state f_ra //File read-ahead state (see Chapter 16)
    size_t f_maxcount //Maximum number of bytes that can be read or written with a single operation (currently set to 231–1)
    unsigned long f_version //Version number, automatically increased after each use
    void * f_security //Pointer to file object’s security structure
    void * private_data //Pointer to data specific for a filesystem or a device driver
    struct list_head f_ep_links //Head of the list of event poll waiters for this file
    spinlock_t f_ep_lock //Spin lock protecting the f_ep_links list
    struct address_space * f_mapping //Pointer to file’s address space object (see Chapter 15)
};
//File objects are allocated through a slab cache named filp

//The following list describes the file operations in the order in which they appear in the file_operations table:
llseek(file, offset, origin);
read(file, buf, count, offset);
aio_read(req, buf, len, pos);
write(file, buf, count, offset);
aio_write(req, buf, len, pos);
readdir(dir, dirent, filldir);
poll(file, poll_table);
ioctl(inode, file, cmd, arg);
unlocked_ioctl(file, cmd, arg);
compat_ioctl(file, cmd, arg);
mmap(file, vma);
open(inode, file);
flush(file);
release(inode, file);
fsync(file, dentry, flag);
aio_fsync(req, flag);
fasync(fd, file, on);
lock(file, cmd, file_lock);
readv(file, vector, count, offset);
writev(file, vector, count, offset);
sendfile(in_file, offset, count, file_send_actor, out_file);
sendpage(file, page, offset, size, pointer, fill);
get_unmapped_area(file, addr, len, offset, flags);
check_flags(flags);
dir_notify(file, arg);
flock(file, flag, lock);


//dentry Objects
/*
“The Common File Model” that the VFS considers
each directory a file that contains a list of files and other directories.
The kernel creates a dentry object for every component of a pathname that a process
looks up; the dentry object associates the component to its corresponding inode.

dentry objects have no corresponding image on disk, and hence no field
is included in the dentry structure to specify that the object has been modified. Dentry
objects are stored in a slab allocator cache whose descriptor is dentry_cache; dentry
objects are thus created and destroyed by invoking kmem_cache_alloc( ) and kmem_
cache_free( ).
*/
struct dentry_struct
{
    atomic_t d_count //Dentry object usage counter
    unsigned int d_flags //Dentry cache flags
    spinlock_t d_lock //Spin lock protecting the dentry object
    struct inode * d_inode //Inode associated with filename
    struct dentry * d_parent //Dentry object of parent directory
    struct qstr d_name //Filename
    struct list_head d_lru //Pointers for the list of unused dentries
    struct list_head d_child //For directories, pointers for the list of directory dentries in the same parent directory
    struct list_head d_subdirs //For directories, head of the list of subdirectory dentries
    struct list_head d_alias //Pointers for the list of dentries associated with the same inode (alias)
    unsigned long d_time //Used by d_revalidate method
    struct dentry_operations* d_op //Dentry methods
    struct super_block * d_sb //Superblock object of the file
    void * d_fsdata //Filesystem-dependent data
    struct rcu_head d_rcu //The RCU descriptor used when reclaiming the dentry object (see the section “Read-Copy Update (RCU)” in Chapter 5)
    struct dcookie_struct * d_cookie //Pointer to structure used by kernel profilers
    struct hlist_node d_hash //Pointer for list in hash table entry
    int d_mounted //For directories, counter for the number of filesystems mounted on this dentry
    unsigned char[] d_iname //Space for short filename
};
//Each dentry object may be in one of four states:
struct denetry_state
{
    Free; //The dentry object contains no valid information and is not used by the VFS. The corresponding memory area is handled by the slab allocator.
    Unused; //The dentry object is not currently used by the kernel. The d_count usage counter of the object is 0, but the d_inode field still points to the associated inode.
    In use; //The dentry object is currently used by the kernel. The d_count usage counter is positive, and the d_inode field points to the associated inode object.
    Negative; //The inode associated with the dentry does not exist, either because the corresponding disk inode has been deleted or because the dentry object was created by resolving a pathname of a nonexistent file.
};

//The methods associated with a dentry object are called dentry operations
d_revalidate(dentry, nameidata);
d_hash(dentry, name);
d_compare(dir, name1, name2);
d_delete(dentry);
d_release(dentry);
d_iput(dentry, ino);

#define The dentry Cache
#ifdef The dentry Cache
/*
To maximize efficiency in handling dentries, Linux uses a dentry cache, which consists
of two kinds of data structures:
• A set of dentry objects in the in-use, unused, or negative state.
• A hash table to derive the dentry object associated with a given filename and a
given directory quickly. As usual, if the required object is not included in the
dentry cache, the search function returns a null value.
*/

#endif // end of The dentry Cache

#define Files Associated with a Process
#ifdef Files Associated with a Process
/*
In Unix Filesystem
that each process has its own current working directory and its own root directory.

A whole data structure of
type fs_struct is used for that purpose , and each process descriptor
has an fs field that points to the process fs_struct structure.
*/

struct fs_struct
{
    atomic_t count //Number of processes sharing this table
    rwlock_t lock //Read/write spin lock for the table fields
    int umask //Bit mask used when opening the file to set the file permissions
    struct dentry * root //Dentry of the root directory
    struct dentry * pwd //Dentry of the current working directory
    struct dentry * altroot //Dentry of the emulated root directory (always NULL for the 80 × 86 architecture)
    struct vfsmount * rootmnt //Mounted filesystem object of the root directory
    struct vfsmount * pwdmnt //Mounted filesystem object of the current working directory
    struct vfsmount * altrootmnt //Mounted filesystem object of the emulated root directory (always NULL for the 80 × 86 architecture)
};

/*
A second table, whose address is contained in the files field of the process descriptor,
specifies which files are currently opened by the process.
*/
static files_struct{
    atomic_t count// Number of processes sharing this table
    rwlock_t file_lock //Read/write spin lock for the table fields
    int max_fds //Current maximum number of file objects
    int max_fdset //Current maximum number of file descriptors
    int next_fd //Maximum file descriptors ever allocated plus 1
    struct file ** fd //Pointer to array of file object pointers
    fd_set * close_on_exec //Pointer to file descriptors to be closed on exec( )
    fd_set * open_fds //Pointer to open file descriptors
    fd_set close_on_exec_init //Initial set of file descriptors to be closed on exec( )
    fd_set open_fds_init //Initial set of file descriptors
    struct file *[] fd_array //Initial array of file object pointers
};
#endif // end of Files Associated with a Process

#endif // end of VFS Data Structures




#define Filesystem Types
#ifdef Filesystem Types
//Special Filesystems
/*
While network and disk-based filesystems enable the user to handle information
stored outside the kernel, special filesystems may provide an easy way for system
programs and administrators to manipulate the data structures of the kernel and to
implement special features of the operating system.
Table 12-8. Most common special filesystems
Name Mount point Description

Special filesystems are not bound to physical block devices. However, the kernel
assigns to each mounted special filesystem a fictitious block device that has the value
0 as major number and an arbitrary value (different for each special filesystem) as a
minor number.
*/
struct specialfilesystems
{
    bdev none //Block devices (see Chapter 13)
    binfmt_misc any //Miscellaneous executable formats (see Chapter 20)
    devpts /dev/pts //Pseudoterminal support (Open Group’s Unix98 standard)
    eventpollfs none //Used by the efficient event polling mechanism
    futexfs none //Used by the futex (Fast Userspace Locking) mechanism
    pipefs none //Pipes (see Chapter 19)
    proc /proc //General access point to kernel data structures
    rootfs none //Provides an empty root directory for the bootstrap phase
    shm none //IPC-shared memory regions (see Chapter 19)
    mqueue any //Used to implement POSIX message queues (see Chapter 19)
    sockfs none //Sockets
    sysfs /sys //General access point to system data (see Chapter 13)
    tmpfs any //Temporary files (kept in RAM unless swapped)
    usbfs /proc/bus/usb //USB devices
};


//Filesystem Type Registration
/*
Often, the user configures Linux to recognize all the filesystems needed when compiling
the kernel for his system. But the code for a filesystem actually may either be
included in the kernel image or dynamically loaded as a module (see Appendix B).
The VFS must keep track of all filesystem types whose code is currently included in
the kernel. It does this by performing filesystem type registration.

Each registered filesystem is represented as a file_system_type object whose fields
are illustrated in Table 12-9.
*/
struct file_system_type
{
    const char * name //Filesystem name
    int fs_flags //Filesystem type flags
    struct super_block * (*)( ) get_sb //Method for reading a superblock
    void (*)() kill_sb //Method for removing a superblock
    struct module * owner //Pointer to the module implementing the filesystem (see Appendix B)
    struct file_system_type * next //Pointer to the next element in the list of filesystem types
    struct list_head fs_supers //Head of a list of superblock objects having the same filesystem type
};
//fs_flags field stores several flags
enum fs_flags{
    FS_REQUIRES_DEV //Every filesystem of this type must be located on a physical disk device.
    FS_BINARY_MOUNTDATA //The filesystem uses binary mount data.
    FS_REVAL_DOT //Always revalidate the “.” and “..” paths in the dentry cache (for network filesystems).
    FS_ODD_RENAME //“Rename” operations are “move” operations (for network filesystems).
};

#endif  // end of Filesystem Types


#defie Filesystem Handling
#ifdef Filesystem Handling
/*
every traditional Unix system, Linux makes use of a system’s root filesystem: it is
the filesystem that is directly mounted by the kernel during the booting phase and
that holds the system initialization scripts and the most essential system programs.
*/


//namespace 
/*
In a traditional Unix system, there is only one tree of mounted filesystems: starting
from the system’s root filesystem, each process can potentially access every file in a
mounted filesystem by specifying the proper pathname. In this respect, Linux 2.6 is
more refined: every process might have its own tree of mounted filesystems—the socalled
namespace of the process.

A process can even change the root filesystem of its
namespace by using the Linux-specific pivot_root() system call.

The list field is the head of a doubly linked circular list collecting all mounted filesystems
that belong to the namespace.
*/
struct namespace_struct
{
    atomic_t count //Usage counter (how many processes share the namespace)
    struct vfsmount * root //Mounted filesystem descriptor for the root directory of the namespace
    struct list_head list// Head of list of all mounted filesystem descriptors
    struct rw_semaphore sem //Read/write semaphore protecting this structure
};


//Filesystem Mounting
/*
In most traditional Unix-like kernels, each filesystem can be mounted only once.
Suppose that an Ext2 filesystem stored in the /dev/fd0 floppy disk is mounted on /flp
by issuing the command:
mount -t ext2 /dev/fd0 /flp
Until the filesystem is unmounted by issuing a umount command, every other mount
command acting on /dev/fd0 fails.
*/
struct vfsmount
{
    struct list_head mnt_hash //Pointers for the hash table list.
    struct vfsmount * mnt_parent //Points to the parent filesystem on which this filesystem is mounted.
    struct dentry * mnt_mountpoint //Points to the dentry of the mount point directory where the filesystem is mounted.
    struct dentry * mnt_root //Points to the dentry of the root directory of this filesystem.
    struct super_block * mnt_sb //Points to the superblock object of this filesystem.
    struct list_head mnt_mounts //Head of a list including all filesystem descriptors mounted on directories of this filesystem.
    struct list_head mnt_child //Pointers for the mnt_mounts list of mounted filesystem descriptors.
    atomic_t mnt_count //Usage counter (increased to forbid filesystem unmounting).
    int mnt_flags //Flags.
    int mnt_expiry_mark //Flag set to true if the filesystem is marked as expired (the filesystem can be automatically unmounted if the flag is set and no one is using it).
    char * mnt_devname //Device filename.
    struct list_head mnt_list //Pointers for namespace’s list of mounted filesystem descriptors.
    struct list_head mnt_fslink //Pointers for the filesystem-specific expire list.
    struct namespace * mnt_namespace //Pointer to the namespace of the process that mounted the filesystem.
};
//The vfsmount data structures are kept in several doubly linked circular lists:
/*
• A hash table indexed by the address of the vfsmount descriptor of the parent filesystem
and the address of the dentry object of the mount point directory. The
hash table is stored in the mount_hashtable array, whose size depends on the
amount of RAM in the system. Each item of the table is the head of a circular
doubly linked list storing all descriptors that have the same hash value. The mnt_
hash field of the descriptor contains the pointers to adjacent elements in this list.
• For each namespace, a circular doubly linked list including all mounted filesystem
descriptors belonging to the namespace. The list field of the namespace
structure stores the head of the list, while the mnt_list field of the vfsmount
descriptor contains the pointers to adjacent elements in the list.
• For each mounted filesystem, a circular doubly linked list including all child
mounted filesystems. The head of each list is stored in the mnt_mounts field of the
mounted filesystem descriptor; moreover, the mnt_child field of the descriptor
stores the pointers to the adjacent elements in the list.
*/


/*
The mnt_flags field of the descriptor stores the value of several flags that specify how
some kinds of files in the mounted filesystem are handled. These flags, which can be
set through options of the mount command, are listed in Table 12-13.
*/
enum mnt_flags{
    MNT_NOSUID Forbid //setuid and setgid flags in the mounted filesystem
    MNT_NODEV Forbid //access to device files in the mounted filesystem
    MNT_NOEXEC Disallow //program execution in the mounted filesystem
};
//Here are some functions that handle the mounted filesystem descriptors:
alloc_vfsmnt(name); //Allocates and initializes a mounted filesystem descriptor
free_vfsmnt(mnt);//Frees a mounted filesystem descriptor pointed to by mnt
lookup_mnt(mnt, dentry);//Looks up a descriptor in the hash table and returns its address



//Mounting a Generic Filesystem
//mount( ) system call is used to mount a generic filesystem
enum gmnt_flags{
    MS_RDONLY //Files can only be read
    MS_NOSUID //Forbid setuid and setgid flags
    MS_NODEV //Forbid access to device files
    MS_NOEXEC //Disallow program execution
    MS_SYNCHRONOUS //Write operations on files and directories are immediate
    MS_REMOUNT //Remount the filesystem changing the mount flags
    MS_MANDLOCK //Mandatory locking allowed
    MS_DIRSYNC //Write operations on directories are immediate
    MS_NOATIME //Do not update file access time
    MS_NODIRATIME //Do not update directory access time
    MS_BIND //Create a “bind mount,” which allows making a file or directory visible at another point of the system directory tree (option --bind of the mount command)
    MS_MOVE //Atomically move a mounted filesystem to another mount point (option --move of the mount command)
    MS_REC// Recursively create “bind mounts” for a directory subtree
    MS_VERBOSE //Generate kernel messages on mount errors
};

mount( ) ;
sys_mount( );
do_mount();
do_kern_mount();

//Mounting the Root Filesystem
init_rootfs();
init_mount_tree();


//Unmounting a Filesystem
umount( );


//Pathname Lookup
/*
Pathname lookup is performed by the path_lookup() function, which receives three
parameters:
name
A pointer to the file pathname to be resolved.
flags
The value of flags that represent how the looked-up file is going to be accessed.
The allowed values are included later in Table 12-16.
nd
The address of a nameidata data structure, which stores the results of the lookup
operation and whose fields are shown in Table 12-15.
When path_lookup() returns, the nameidata structure pointed to by nd is filled with
data pertaining to the pathname lookup operation.
*/

struct nameidata
{
    struct dentry * dentry //Address of the dentry object
    struct vfs_mount * mnt //Address of the mounted filesystem object
    struct qstr last //Last component of the pathname (used when the LOOKUP_ PARENT flag is set)
    unsigned int flags //Lookup flags
    int last_type //Type of last component of the pathname (used when the LOOKUP_PARENT flag is set)
    unsigned int depth //Current level of symbolic link nesting (see below); it must be smaller than 6
    char[ ] * saved_names// Array of pathnames associated with nested symbolic links
    union intent //One-member union specifying how the file will be accessed
};

/*
The flags field stores the value of some flags used in the lookup operation; they are
listed in Table 12-16. Most of these flags can be set by the caller in the flags parameter
of path_lookup().
*/

enum lookup_path{
    LOOKUP_FOLLOW ,//If the last component is a symbolic link, interpret (follow) it
    LOOKUP_DIRECTORY ,//The last component must be a directory
    LOOKUP_CONTINUE ,//There are still filenames to be examined in the pathname
    LOOKUP_PARENT ,//Look up the directory that includes the last component of the pathname
    LOOKUP_NOALT ,//Do not consider the emulated root directory (useless in the 80x86 architecture)
    LOOKUP_OPEN,// Intent is to open a file
    LOOKUP_CREATE ,//Intent is to create a file (if it doesn’t exist)
    LOOKUP_ACCESS ;//Intent is to check user’s permission for a file
};      




//Standard Pathname Lookup

//Parent Pathname Lookup

//Lookup of Symbolic Links


#endif  // end of Filesystem Handling


#define Implementations of VFS System Calls
#ifdef Implementations of VFS System Calls

open( );// System call
//The flags of the open( ) system call
enum flagsForOpen{
    O_RDONLY //Open for reading
    O_WRONLY //Open for writing
    O_RDWR //Open for both reading and writing
    O_CREAT //Create the file if it does not exist
    O_EXCL //With O_CREAT, fail if the file already exists
    O_NOCTTY //Never consider the file as a controlling terminal
    O_TRUNC //Truncate the file (remove all existing contents)
    O_APPEND //Always write at end of the file
    O_NONBLOCK //No system calls will block on the file
    O_NDELAY //Same as O_NONBLOCK
    O_SYNC //Synchronous write (block until physical write terminates)
    FASYNC //I/O event notification via signals
    O_DIRECT //Direct I/O transfer (no kernel buffering)
    O_LARGEFILE// Large file (size greater than 2 GB)
    O_DIRECTORY //Fail if file is not a directory
    O_NOFOLLOW //Do not follow a trailing symbolic link in pathname
    O_NOATIME //Do not update the inode’s last access time
};
sys_open( );

//The read( ) and write( ) System Calls
//The close( ) System Call

#endif // end of Implementations of VFS System Calls



#define File Locking
#ifdef File Locking
/*
When a file can be accessed by more than one process, a synchronization problem
occurs. What happens if two processes try to write in the same file location? Or
again, what happens if a process reads from a file location while another process is
writing into it?
In traditional Unix systems, concurrent accesses to the same file location produce
unpredictable results. However, Unix systems provide a mechanism that allows the
processes to lock a file region so that concurrent accesses may be easily avoided.
The POSIX standard requires a file-locking mechanism based on the fcntl( ) system
call. It is possible to lock an arbitrary region of a file (even a single byte) or to lock
the whole file (including data appended in the future). Because a process can choose
to lock only a part of a file, it can also hold multiple locks on different parts of the
file.
This kind of lock does not keep out another process that is ignorant of locking. Like
a semaphore used to protect a critical region in code, the lock is considered “advisory”
because it doesn’t work unless other processes cooperate in checking the existence
of a lock before accessing the file. Therefore, POSIX’s locks are known as
advisory locks.
Traditional BSD variants implement advisory locking through the flock( ) system
call. This call does not allow a process to lock a file region, only the whole file. Traditional
System V variants provide the lockf( ) library function, which is simply an
interface to fcntl( ).
More importantly, System V Release 3 introduced mandatory locking: the kernel
checks that every invocation of the open( ), read( ), and write( ) system calls does
not violate a mandatory lock on the file being accessed. Therefore, mandatory locks
are enforced even between noncooperative processes.*
Whether processes use advisory or mandatory locks, they can use both shared read
locks and exclusive write locks. Several processes may have read locks on some file
region, but only one process can have a write lock on it at the same time. Moreover,
it is not possible to get a write lock when another process owns a read lock for the
same file region, and vice versa.
*
*/


//File-Locking Data Structures
/*
All type of Linux locks are represented by the same file_lock data structure whose
fields are shown in Table 12-19.
*/
struct file_lock
{
    struct file_lock * fl_next //Next element in list of locks associated with the inode
    struct list_head fl_link //Pointers for active or blocked list
    struct list_head fl_block //Pointers for the lock’s waiters list
    struct files_struct * fl_owner //Owner’s files_struct
    unsigned int fl_pid //PID of the process owner wait_queue_head_t fl_wait Wait queue of blocked processes
    struct file * fl_file //Pointer to file object
    unsigned char fl_flags //Lock flags
    unsigned char fl_type //Lock type
    loff_t fl_start// Starting offset of locked region
    loff_t fl_end //Ending offset of locked region
    struct fasync_struct * fl_fasync// Used for lease break notifications
    unsigned long fl_break_time //Remaining time before end of lease
    struct file_lock_operations * fl_ops //Pointer to file lock operations
    struct lock_manager_operations * fl_mops //Pointer to lock manager operations
    union fl_u //Filesystem-specific information
};
/*
All lock_file structures that refer to the same file on disk are collected in a singly
linked list, whose first element is pointed to by the i_flock field of the inode object.
The fl_next field of the lock_file structure specifies the next element in the list.
*/

#endif // end of File Locking

#define FL_FLOCK Locks
#ifdef FL_FLOCK Locks
/*
An FL_FLOCK lock is always associated with a file object and is thus owned by the process
that opened the file (or by all clone processes sharing the same opened file).
When a lock is requested and granted, the kernel replaces every other lock that the
process is holding on the same file object with the new lock. This happens only when
a process wants to change an already owned read lock into a write one, or vice versa.
Moreover, when a file object is being freed by the fput( ) function, all FL_FLOCK locks
that refer to the file object are destroyed. However, there could be other FL_FLOCK
read locks set by other processes for the same file (inode), and they still remain
active.
The flock( ) system call allows a process to apply or remove an advisory lock on an
open file. It acts on two parameters: the fd file descriptor of the file to be acted upon
and a cmd parameter that specifies the lock operation. A cmd parameter of LOCK_SH
requires a shared lock for reading, LOCK_EX requires an exclusive lock for writing, and
LOCK_UN releases the lock.*
*/


#endif // end of FL_FLOCK Locks


#define FL_POSIX Locks
#ifdef FL_POSIX Locks
/*
An FL_POSIX lock is always associated with a process and with an inode; the lock is
automatically released either when the process dies or when a file descriptor is closed
(even if the process opened the same file twice or duplicated a file descriptor). Moreover,
FL_POSIX locks are never inherited by a child across a fork( ).
When used to lock files, the fcntl( ) system call acts on three parameters: the fd file
descriptor of the file to be acted upon, a cmd parameter that specifies the lock operation,
and an fl pointer to a flock data structure* stored in the User Mode process
address space; its fields are described in Table 12-20.
*/
struct flock
{
    short l_type// F_RDLOCK (requests a shared lock), F_WRLOCK (requests an exclusive lock), F_UNLOCK (releases the lock)
    short l_whence //SEEK_SET (from beginning of file), SEEK_CURRENT (from current file pointer), SEEK_ END (from end of file)
    off_t l_start //Initial offset of the locked region relative to the value of l_whence
    off_t l_len //Length of locked region (0 means that the region includes all potential writes past the current end of the file)
    pid_t l_pid //PID of the owner
};



#endif // end of FL_POSIX Locks

#endif // end of __VIRTUAL_FILESYSTEM__H