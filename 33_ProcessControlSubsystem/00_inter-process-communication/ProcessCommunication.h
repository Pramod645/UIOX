#ifndef __PROCESSES_COMMUNICATION__H
#define __PROCESSES_COMMUNICATION__H

/*
here first complete the Kernel Synchronization(5).
and also The Virtual Filesystem (12)

This  explains how User Mode processes can synchronize their actions and exchange data.

now ready, after having discussed I/O management and filesystems at length, to extend
the discussion to User Mode processes. These processes must rely on the kernel to
facilitate interprocess synchronization and communication.
*/
/*
application programmers have a variety of needs that call for different communication
mechanisms. Here are the basic mechanisms that Unix systems offer to
allow interprocess communication:
*/
#define Pipes and FIFOs (named pipes)
#define Semaphores
#define Messages
#define Shared memory regions
#define Sockets

#ifdef  Pipes
pipe( );
execve( );//system call to execute the program specified by filename
popen( );
pclose( );

struct ipe_inode_info{
    struct wait_queue * wait //Pipe/FIFO wait queue
    unsigned int nrbufs //Number of buffers containing data to be read
    unsigned int curbuf /Index of first buffer containing data to be read
    struct pipe_buffer [16] bufs //Array of pipe’s buffer descriptors
    struct page * tmp_page //Pointer to a cached page frame
    unsigned int start //Read position in current pipe buffer
    unsigned int readers //Flag for (or number of) reading processes
    unsigned int writers //Flag for (or number of) writing processes
    unsigned int waiting_writers //Number of writing processes sleeping in the wait queue
    unsigned int r_counter //Like readers, but used when waiting for a process that reads from the FIFO
    unsigned int w_counter //Like writers, but used when waiting for a process that writes into the FIFO
    struct fasync_struct *fasync_readers //Used for asynchronous I/O notification via signals
    struct fasync_struct *fasync_writers //Used for asynchronous I/O notification via signals
};

struct pipe_buffer{
    struct page * page //Address of the descriptor of the page frame for the pipe buffer
    unsigned int offset //Current position of the significant data inside the page frame
    unsigned int len //Length of the significant data in the pipe buffer
    struct pipe_buf_operations *ops //Address of a table of methods relative to the pipe buffer (NULL if thepipe buffer is empty)
};

map();
unmap();
release();
init_pipe_fs();

#define Creating and Destroying a Pipe
sys_pipe( );
do_pipe( );
read( )
pipe_write( );
write( );

#endif //end of Pipes

#ifdef FIFO

//Access type File operations Read method Write method
Read-only read_fifo_fops pipe_read( ) bad_pipe_w( )
Write-only write_fifo_fops bad_pipe_r( ) pipe_write( )
Read/write rdwr_fifo_fops pipe_read( ) pipe_write( )

fifo_open( );

struct kern_ipc_perm{
   spinlock_t lock //Spin lock protecting the IPC resource descriptor
    int deleted //Flag set if the resource has been released
    int key //IPC key
    unsigned int uid //Owner user ID
    unsigned int gid //Owner group ID
    unsigned int cuid //Creator user ID
    unsigned int cgid //Creator group ID  
    unsigned short mode //Permission bit mask
    unsigned long seq //Slot usage sequence number
    void * security //Pointer to a security structure (used by SELinux)
};

semctl( );
msgctl( );
shmctl( );



#endif // end of FIFO

#ifdef Semaphores

///proc/sys/kernel/sem file.

semget( );
semop( );
semop( );
semctl( );

struct sem_array{
    struct kern_ipc_perm sem_perm kern_ipc_perm data structure
long sem_otime Timestamp of last semop( )
long sem_ctime Timestamp of last change
struct sem * sem_base Pointer to first sem structure
struct sem_queue * sem_pending Pending operations
struct sem_queue ** sem_pending_last Last pending operation
struct sem_undo * undo Undo requests
unsigned long sem_nsems Number of semaphores in array
};

sem_queue data structure
struct sem_queue * next Pointer to next queue element
struct sem_queue ** prev Pointer to previous queue element
struct task_struct * sleeper Pointer to the sleeping process that requested the semaphore operation
struct sem_undo * undo Pointer to sem_undo structure
int pid Process identifier
int status Completion status of operation
struct sem_array * sma Pointer to IPC semaphore descriptor
int id Slot index of the IPC semaphore resource
struct sembuf * sops Pointer to array of pending operations
int nsops Number of pending operations
int alter Flag denoting whether the operation modifies the semaphore array


#endif //end of Semaphores


#ifdef Messages

// /proc/sys/kernel/msgmni, /proc/sys/kernel/msgmnb, and /proc/sys/ kernel/msgmax files, respectively

msg_queue data structure
struct kern_ipc_perm q_perm kern_ipc_perm data structure
long q_stime Time of last msgsnd( )
long q_rtime Time of last msgrcv( )
long q_ctime Last change time
unsigned long q_qcbytes Number of bytes in queue
unsigned long q_qnum Number of messages in queue
unsigned long q_qbytes Maximum number of bytes in queue
int q_lspid PID of last msgsnd( )
int q_lrpid PID of last msgrcv()
struct list_head q_messages List of messages in queue
struct list_head q_receivers List of processes receiving messages
struct list_head q_senders List of processes sending messages



msg_msg data structure
struct list_head m_list Pointers for message list
long m_type Message type
int m_ts Message text size
struct msg_msgseg * next Next portion of the message
void * security Pointer to a security data structure (used by SELinux)


#endif //end of Messages



#ifdef Shared memory regions

// /proc/sys/kernel/shmmni, /proc/sys/kernel/shmmax, and /proc/sys/kernel/shmall files, respectively.

shmid_kernel data structure
struct kern_ipc_perm shm_perm kern_ipc_perm data structure
struct file * shm_file Special file of the segment
int id Slot index of the segment
unsigned long shm_nattch Number of current attaches
unsigned long shm_segsz Segment size in bytes
long shm_atim Last access time
long shm_dtim Last detach time
long shm_ctim Last change time
int shm_cprid PID of creator
int shm_lprid PID of last accessing process
struct user_struct * mlock_user Pointer to the user_struct descriptor of the user that
locked in RAM the shared memory resource (see the section
“The clone( ), fork( ), and vfork( ) System Calls” in Chapter 3)

#define POSIX message queues
mq_open() Open (optionally creating) a POSIX message queue
mq_close() Close a POSIX message queue (without destroying it)
mq_unlink() Destroy a POSIX message queue
mq_send(),
mq_timedsend()
Send a message to a POSIX message queue; the latter function defines a time limit for the
operation
mq_receive(),
mq_timedreceive()
Fetch a message from a POSIX message queue; the latter function defines a time limit for
the operation
mq_notify() Establish an asynchronous notification mechanism for the arrival of messages in an empty
POSIX message queue
mq_getattr(),
mq_setattr()
Respectively get and set attributes of a POSIX message queue (essentially, whether the
send and receive operations should be blocking or nonblocking)











#endif //end of Shared memory regions


#ifdef Sockets

#endif //end of Sockets





#endif // end of __PROCESSES_COMMUNICATION__H
