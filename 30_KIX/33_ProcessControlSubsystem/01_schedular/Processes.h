#ifndef __PROCESSES_H
#define __PROCESSES_H

#define Processes
#define LigntweightProcesses
#define Thread
//D:\linux\include\linux\regset.h
//D:\linux\include\linux\sched.h
//D:\linux\arch\Kconfig
//D:\linux\include\linux\sched.h 818 line

#define TASK_RUNNING
#define TASK_INTERRUPTIBLE
#define TASK_UNINTERRUPTIBLE
// and so on

enum tast_State{
    SIGSTOP,
    SIGTSTP;
    // and so on
};

struct  state
{
    //runing?
    /* data */
};

struct Processes
{

    //PID??
    /* data */
};


struct thread_info
{
    /* thread_info structure, and the Kernel Mode process
stack. The length of this memory area is usually 8,192 bytes 
first page frame aligned to a multiple of 213;
a process in
Kernel Mode accesses a stack contained in the kernel data segment, which is different
from the stack used by the process in User Mode.
*/
};

union thread_union {
struct thread_info thread_info;
unsigned long stack[2048]; /* 1024 for 4KB stacks */
};

#define /*alloc_thread_info and free_thread_info macros to allocate and
release the memory area storing a thread_info structure and a kernel stack*/

#define Doubly linked lists //
/*
list_head data structure, whose only fields
next and prev represent the forward and back pointers of a generic doubly linked list
element,
pointers in a list_
head field store the addresses of other list_head fields rather than the addresses of
the whole data structures in which the list_head structure is included;
*/

#define LIST_HEAD(list_name)

/*
list_add(n,p)
Inserts an element pointed to by n right after the specified element pointed
to by p. (To insert n at the beginning of the list, set p to the address of the
list head.)
list_add_tail(n,p)
Inserts an element pointed to by n right before the specified element
pointed to by p. (To insert n at the end of the list, set p to the address of the
list head.)
list_del(p)
Deletes an element pointed to by p. (There is no need to specify the head of
the list.)
list_empty(p) Checks if the list specified by the address p of its head is empty.
list_entry(p,t,m)
Returns the address of the data structure of type t in which the list_
head field that has the name m and the address p is included.
list_for_each(p,h) Scans the elements of the list specified by the address h of the head; in each
iteration, a pointer to the list_head structure of the list element is
returned in p.
list_for_each_entry(p,h,m)
Similar to list_for_each, but returns the address of the data structure
embedding the list_head structure rather than the address of the
list_head structure itself.
*/

/*
head of the process list is the init_task task_struct descriptor

*/

#define /*SET_LINKS and REMOVE_LINKS macros are used to insert and to remove a process 
descriptor in the process list */

/*scans the whole process list*/
#define for_each_process(p) \
for (p=&init_task; (p=list_entry((p)->tasks.next, \
struct task_struct, tasks) \
) != &init_task; )

struct prio_array_t{
    int nr_active //The number of process descriptors linked into the lists
    unsigned long[5] bitmap //A priority bitmap: each flag is set if and only if the corresponding
    //priority list is not empty
    struct list_head [140] //queue The 140 heads of the priority lists

};

#define pid_hashfn(x) hash_long((unsigned long) x, pidhash_shift)

//chaining to handle colliding PIDs
//PID hash table

struct pid{
    int nr The PID number
struct hlist_node pid_chain The links to the next and previous elements in the hash chain list
struct list_head pid_list The head of the per-PID list
};

#define do_each_task_pid(nr, type, task)
#define while_each_task_pid(nr, type, task)
#define find_task_by_pid_type(type, nr)
#define find_task_by_pid(nr)
#define attach_pid(task, type, nr)
#define detach_pid(task, type)
#define next_thread(task)


/*Process Resource Limits*/
#define RLIMIT_AS /*The maximum size of process address space, in bytes. The kernel checks this value when the
process uses malloc( ) or a related function to enlarge its address space (see the section
“The Process’s Address Space” in Chapter 9).*/
#define RLIMIT_CORE /*The maximum core dump file size, in bytes. The kernel checks this value when a process is
aborted, before creating a core file in the current directory of the process (see the section
“Actions Performed upon Delivering a Signal” in Chapter 11). If the limit is 0, the kernel
won’t create the file.*/
#define RLIMIT_CPU /*The maximum CPU time for the process, in seconds. If the process exceeds the limit, the kernel
sends it a SIGXCPU signal, and then, if the process doesn’t terminate, a SIGKILL signal
(see Chapter 11).*/
#define RLIMIT_DATA /*The maximum heap size, in bytes. The kernel checks this value before expanding the heap of
the process (see the section “Managing the Heap” in Chapter 9).*/
/*
RLIMIT_FSIZE The maximum file size allowed, in bytes. If the process tries to enlarge a file to a size greater
than this value, the kernel sends it a SIGXFSZ signal.
RLIMIT_LOCKS Maximum number of file locks (currently, not enforced).
RLIMIT_MEMLOCK The maximum size of nonswappable memory, in bytes. The kernel checks this value when
the process tries to lock a page frame in memory using the mlock( ) or mlockall( ) system
calls (see the section “Allocating a Linear Address Interval” in Chapter 9).
RLIMIT_MSGQUEUE Maximum number of bytes in POSIX message queues (see the section “POSIX Message
Queues” in Chapter 19).
RLIMIT_NOFILE The maximum number of open file descriptors. The kernel checks this value when opening a
new file or duplicating a file descriptor (see Chapter 12).
RLIMIT_NPROC The maximum number of processes that the user can own (see the section “The clone( ),
fork( ), and vfork( ) System Calls” later in this chapter).
RLIMIT_RSS The maximum number of page frames owned by the process (currently, not enforced).
RLIMIT_SIGPENDING The maximum number of pending signals for the process (see Chapter 11).
RLIMIT_STACK The maximum stack size, in bytes. The kernel checks this value before expanding the User
*/

struct rlimit {
unsigned long rlim_cur;
unsigned long rlim_max;
};


/*
Process Switch:
To control the execution of processes, the kernel must be able to suspend the execution
of the process running on the CPU and resume the execution of some other process
previously suspended. This activity goes variously by the names process switch,
task switch, or context switch. 
*/

/*
Hardware Context:
The set of data that must be loaded into the registers before the process resumes its
execution on the CPU is called the hardware context

Task State Segment
The 80×86 architecture includes a specific segment type called the Task State Segment
(TSS), to store hardware contexts. Although Linux doesn’t use hardware
context switches, it is nonetheless forced to set up a TSS for each distinct CPU in the
system. This is done for two main reasons:
• When an 80×86 CPU switches from User Mode to Kernel Mode, it fetches the
address of the Kernel Mode stack from the TSS (see the sections “Hardware
Handling of Interrupts and Exceptions” in Chapter 4 and “Issuing a System Call
via the sysenter Instruction” in Chapter 10).
• When a User Mode process attempts to access an I/O port by means of an in or
out instruction, the CPU may need to access an I/O Permission Bitmap stored in
the TSS to verify whether the process is allowed to address the port.
More precisely, when a process executes an in or out I/O instruction in User
Mode, the control unit performs the following operations:
1. It checks the 2-bit IOPL field in the eflags register. If it is set to 3, the control
unit executes the I/O instructions. Otherwise, it performs the next
check.
2. It accesses the tr register to determine the current TSS, and thus the proper I/O
Permission Bitmap.
3. It checks the bit of the I/O Permission Bitmap corresponding to the I/O port
specified in the I/O instruction. If it is cleared, the instruction is executed;
otherwise, the control unit raises a “General protection” exception.
*/


struct tss_struct // in memory managment
{
    /* data */
};

/*
describe what the switch_to macro typically does on an 80×86
microprocessor by using standard assembly language:
1. Saves the values of prev and next in the eax and edx registers, respectively:
movl prev, %eax
movl next, %edx
2. Saves the contents of the eflags and ebp registers in the prev Kernel Mode stack.
They must be saved because the compiler assumes that they will stay unchanged
until the end of switch_to:
pushfl
pushl %ebp
3. Saves the content of esp in prev->thread.esp so that the field points to the top of
the prev Kernel Mode stack:
movl %esp,484(%eax)
The 484(%eax) operand identifies the memory cell whose address is the contents
of eax plus 484.
4. Loads next->thread.esp in esp. From now on, the kernel operates on the Kernel
Mode stack of next, so this instruction performs the actual process switch from
prev to next. Because the address of a process descriptor is closely related to that
of the Kernel Mode stack (as explained in the section “Identifying a Process” earlier
in this chapter), changing the kernel stack means changing the current
process:
movl 484(%edx), %esp
5. Saves the address labeled 1 (shown later in this section) in prev->thread.eip.
When the process being replaced resumes its execution, the process executes the
instruction labeled as 1:
movl $1f, 480(%eax)
6. On the Kernel Mode stack of next, the macro pushes the next->thread.eip
value, which, in most cases, is the address labeled as 1:
pushl 480(%edx)
7. Jumps to the _ _switch_to( ) C function (see next):
jmp __switch_to
8. Here process A that was replaced by B gets the CPU again: it executes a few
instructions that restore the contents of the eflags and ebp registers. The first of
these two instructions is labeled as 1:
1:
popl %ebp
popfl
Notice how these pop instructions refer to the kernel stack of the prev process.
They will be executed when the scheduler selects prev as the new process to be
executed on the CPU, thus invoking switch_to with prev as the second parameter.
Therefore, the esp register points to the prev’s Kernel Mode stack.
9. Copies the content of the eax register (loaded in step 1 above) into the memory
location identified by the third parameter last of the switch_to macro:
movl %eax, last
As discussed earlier, the eax register points to the descriptor of the process that
has just been replaced.*
*/

#define The _ _switch_to( ) function does the bulk of the process switch started by the
switch_to( ) macro

//CReatting light weight process by cloning

//destroying processes

struct task{
    state;
//1.thread info, low level information for the process
thread info;
ugas;
flags.

run_list;
task;


mm;
//2.mm struct, pointers to memory area descriptor

real_paranet;
paranet;



tty;
//3.tty_struct, tty acssociate with process 

thread;


fs;
//4.fs_struct, current directory
//5.files_struct, point ot file descriptor
files;

signal;
//6.signal_struct, signals recieved
pending;
};


#endif