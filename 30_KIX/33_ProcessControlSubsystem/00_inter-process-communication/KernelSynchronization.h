#ifndef __KERNEL_SYNCHRONIZATION__H
#define __KERNEL_SYNCHRONIZATION__H

/*
You could think of the kernel as a server that answers requests; these requests can
come either from a process running on a CPU or an external device issuing an interrupt
request. We make this analogy to underscore that parts of the kernel are not run
serially, but in an interleaved way. Thus, they can give rise to race conditions, which
must be controlled through proper synchronization techniques.

Boss requests correspond to interrupts, while customer requests correspond to system
calls or exceptions raised by User Mode processes.
*/

#define How the Kernel Services Requests

#define Kernel Preemption // this is fourth rule and first three in InterruptsNexceptions.h under #define Nested Execution of Exception and Interrupt Handlers
#ifdef  Kernel Preemption

/*
While process A executes an exception handler (necessarily in Kernel Mode), a higher
priority process B becomes runnable. This could happen, for instance, if an IRQ occurs
and the corresponding handler awakens process B. If the kernel is preemptive, a forced
process switch replaces process A with B. The exception handler is left unfinished and
will be resumed only when the scheduler selects again process A for execution. Conversely,
if the kernel is nonpreemptive, no process switch occurs until process A either
finishes handling the exception handler or voluntarily relinquishes the CPU.

The main motivation for making a kernel preemptive is to reduce the dispatch latency
of the User Mode processes, that is, the delay between the time they become runnable
and the time they actually begin running. Processes performing timely scheduled
tasks (such as external hardware controllers, environmental monitors, movie players,
and so on) really benefit from kernel preemption, because it reduces the risk of
being delayed by another process running in Kernel Mode.
*/
#define PREEMPTIVE // a kernel is preemptive if a process switch may occur while the replaced process is executing a kernel function, that is, while it runs in Kernel Mode.
#define NONPREEMPTIVE //in nonpreemptive kernels, the current process cannot be replaced unless it is about to switch to User Mode
#define PLAIN

#define switch_to   // used in both preemptive and nonpreemptive kernels

#define current_thread_info() //kernel preemption is disabled when the preempt_count field in the thread_info descriptor referenced


#define preempt_count()    //Selects the preempt_count field in the thread_info descriptor
#define preempt_disable() //Increases by one the value of the preemption counter
#define preempt_enable_no_resched() //Decreases by one the value of the preemption counter
#define preempt_enable() //Decreases by one the value of the preemption counter, and invokes
#define preempt_schedule() //if the TIF_NEED_RESCHED flag in the thread_info descriptor is set
#define get_cpu() //Similar to preempt_disable(), but also returns the number of the local CPU
#define put_cpu() //Same as preempt_enable()
#define put_cpu_no_resched() //Same as preempt_enable_no_resched()

#endif // end of  Kernel Preemption



#define Synchronization Primitives //examine how kernel control paths can be interleaved while avoiding race conditions among shared data.
#ifdef Synchronization Primitives

/*
examine how kernel control paths can be interleaved while avoiding race
conditions among shared data. Table 5-2 lists the synchronization techniques used by
the Linux kernel. The “Scope” column indicates whether the synchronization technique
applies to all CPUs in the system or to a single CPU
*/
/*
Table 5-2. Various types of synchronization techniques used by the kernel
Technique Description Scope
Per-CPU variables Duplicate a data structure among the CPUs All CPUs
Atomic operation Atomic read-modify-write instruction to a counter All CPUs
Memory barrier Avoid instruction reordering Local CPU or All CPUs
Spin lock Lock with busy wait All CPUs
Semaphore Lock with blocking wait (sleep) All CPUs
Seqlocks Lock based on an access counter All CPUs
Local interrupt disabling Forbid interrupt handling on a single CPU Local CPU
Local softirq disabling Forbid deferrable function handling on a single CPU Local CPU
Read-copy-update (RCU) Lock-free access to shared data structures through pointers All CPUs
*/

#define Per-CPU variable
#ifdef Per-CPU variable
/*
Table 5-3. Functions and macros for the per-CPU variables
Macro or function name Description
DEFINE_PER_CPU(type, name) Statically allocates a per-CPU array called name of type data structures
per_cpu(name, cpu) Selects the element for CPU cpu of the per-CPU array name
_ _get_cpu_var(name) Selects the local CPU’s element of the per-CPU array name
get_cpu_var(name) Disables kernel preemption, then selects the local CPU’s element of the
per-CPU array name
put_cpu_var(name) Enables kernel preemption (name is not used)
alloc_percpu(type) Dynamically allocates a per-CPU array of type data structures and returns
its address
free_percpu(pointer) Releases a dynamically allocated per-CPU array at address pointer
per_cpu_ptr(pointer, cpu) Returns the address of the element for CPU cpu of the per-CPU array at
address pointer
*/
#endif // end of Per-CPU variable

#define Atomic operation
#ifdef Atomic operation
/*
Several assembly language instructions are of type “read-modify-write”—that is,
they access a memory location twice, the first time to read the old value and the second
time to write a new value.
*/
/*
Table 5-4. Atomic operations in Linux
Function Description
atomic_read(v) Return *v
atomic_set(v,i) Set *v to i
atomic_add(i,v) Add i to *v
atomic_sub(i,v) Subtract i from *v
atomic_sub_and_test(i, v) Subtract i from *v and return 1 if the result is zero; 0 otherwise
atomic_inc(v) Add 1 to *v
atomic_dec(v) Subtract 1 from *v
atomic_dec_and_test(v) Subtract 1 from *v and return 1 if the result is zero; 0 otherwise
atomic_inc_and_test(v) Add 1 to *v and return 1 if the result is zero; 0 otherwise
atomic_add_negative(i, v) Add i to *v and return 1 if the result is negative; 0 otherwise
atomic_inc_return(v) Add 1 to *v and return the new value of *v
atomic_dec_return(v) Subtract 1 from *v and return the new value of *v
atomic_add_return(i, v) Add i to *v and return the new value of *v
atomic_sub_return(i, v) Subtract i from *v and return the new value of *v
*/
/*
Table 5-5. Atomic bit handling functions in Linux
Function Description
test_bit(nr, addr) Return the value of the nrth bit of *addr
set_bit(nr, addr) Set the nrth bit of *addr
clear_bit(nr, addr) Clear the nrth bit of *addr
change_bit(nr, addr) Invert the nrth bit of *addr
test_and_set_bit(nr, addr) Set the nrth bit of *addr and return its old value
test_and_clear_bit(nr, addr) Clear the nrth bit of *addr and return its old value
test_and_change_bit(nr, addr) Invert the nrth bit of *addr and return its old value
atomic_clear_mask(mask, addr) Clear all bits of *addr specified by mask
atomic_set_mask(mask, addr) Set all bits of *addr specified by mask
*/

#endif // end of Atomic operation

#define Memory barrier //Optimization and Memory Barriers
#ifdef Memory barrier
/*
When using optimizing compilers, you should never take for granted that instructions
will be performed in the exact order in which they appear in the source code.
*/
/*
Table 5-6. Memory barriers in Linux
Macro Description
mb() Memory barrier for MP and UP
rmb() Read memory barrier for MP and UP
wmb() Write memory barrier for MP and UP
smp_mb() Memory barrier for MP only
smp_rmb() Read memory barrier for MP only
smp_wmb() Write memory barrier for MP only
*/
#endif // end of Memory barrier

#define Spin lock
#ifdef Spin lock
/*
A widely used synchronization technique is locking. When a kernel control path
must access a shared data structure or enter a critical region, it needs to acquire a
“lock” for it. A resource protected by a locking mechanism is quite similar to a
resource confined in a room whose door is locked when someone is inside. If a kernel
control path wishes to access the resource, it tries to “open the door” by acquiring
the lock. It succeeds only if the resource is free. Then, as long as it wants to use
the resource, the door remains locked. When the kernel control path releases the
lock, the door is unlocked and another kernel control path may enter the room.
*/

struct spinlock{
    slock; //Encodes the spin lock state: the value 1 corresponds to the unlocked state, while every negative value and 0 denote the locked state
    break_lock; //Flag signaling that a process is busy waiting for the lock (present only if the kernelsupports both SMP and kernel preemption)
};
/*
Table 5-7. Spin lock macros
Macro Description
spin_lock_init() Set the spin lock to 1 (unlocked)
spin_lock() Cycle until spin lock becomes 1 (unlocked), then set it to 0 (locked)
spin_unlock() Set the spin lock to 1 (unlocked)
spin_unlock_wait() Wait until the spin lock becomes 1 (unlocked)
spin_is_locked() Return 0 if the spin lock is set to 1 (unlocked); 1 otherwise
spin_trylock() Set the spin lock to 0 (locked), and return 1 if the previous value of the lock was 1; 0 otherwise
*/

#define spin_lock // with and without kernel preemption
#define spin_unlock

#define Read/Write Spin Locks
#define Seqlocks

#endif // end of Spin lock



#define Semaphore //a locking primitive that allows waiters to sleep until the desired resource becomes free
#ifdef Semaphore
/*
Actually, Linux offers two kinds of semaphores:
• Kernel semaphores, which are used by kernel control paths
• System V IPC semaphores, which are used by User Mode processes
In this section, we focus on kernel semaphores, while IPC semaphores are described
in Chapter 19.

A kernel semaphore is similar to a spin lock, in that it doesn’t allow a kernel control
path to proceed unless the lock is open. However, whenever a kernel control path
tries to acquire a busy resource protected by a kernel semaphore, the corresponding
process is suspended. It becomes runnable again when the resource is released.
Therefore, kernel semaphores can be acquired only by functions that are allowed to
sleep; interrupt handlers and deferrable functions cannot use them.
*/

struct semaphore{
    count; /*Stores an atomic_t value. If it is greater than 0, the resource is free—that is, it is currently available. If count is equal to 0, the semaphore is busy but no other
process is waiting for the protected resource. Finally, if count is negative, the
resource is unavailable and at least one process is waiting for it.*/
    wait; /*Stores the address of a wait queue list that includes all sleeping processes that are
currently waiting for the resource. Of course, if count is greater than or equal to
0, the wait queue is empty.*/
    sleepers;/*Stores a flag that indicates whether some processes are sleeping on the semaphore.
We’ll see this field in operation soon.*/

};

init_MUTEX();
nit_MUTEX_LOCKED();

#define DECLARE_MUTEX 
#define DECLARE_MUTEX_LOCKED


#define Read/Write Semaphores
#ifdef Read/Write Semaphores
struct rw_semaphore{
    count; //Stores two 16-bit counters
    wait_list; //Points to a list of waiting processes
    wait_lock; //A spin lock used to protect the wait queue list and the rw_semaphore structure itself.
};

#endif // end of Read/Write Semaphores

#define Completions //have been introduced to solve a subtle race condition that occurs in multiprocessor systems
struct completion {
    unsigned int done;
    wait_queue_head_t wait;
};


#define Local Interrupt Disabling
/*
Interrupt disabling is one of the key mechanisms used to ensure that a sequence of
kernel statements is treated as a critical section. It allows a kernel control path to
continue executing even when hardware devices issue IRQ signals, thus providing an
effective way to protect data structures that are also accessed by interrupt handlers.
*/
#define local_irq_disable( )


#define Disabling and Enabling Deferrable Functions

#endif // end of Semaphore

#define Seqlocks
#ifdef Seqlocks
#endif // end of Seqlocks

#define Local interrupt disabling
#ifdef Local interrupt disabling
#endif // end of Local interrupt disabling

#define Local softirq disabling
#ifdef Local softirq disabling
#endif // end of Local softirq disabling

#define Read-copy-update //(RCU). an improvement over seqlocks, which allow only one writer to proceed
#ifdef Read-copy-update

#endif // Read-copy-update (RCU)



#endif //end of Synchronization Primitives

#define Synchronizing Accesses to Kernel Data Structures
#ifdef Synchronizing Accesses to Kernel Data Structures

/*
A shared data structure can be protected against race conditions by using some of
the synchronization primitives shown in the previous section. Of course, system performance
may vary considerably, depending on the kind of synchronization primitive
selected. Usually, the following rule of thumb is adopted by kernel developers:
always keep the concurrency level as high as possible in the system.
In turn, the concurrency level in the system depends on two main factors:
• The number of I/O devices that operate concurrently
• The number of CPUs that do productive work
To maximize I/O throughput, interrupts should be disabled for very short periods of
time. As described in the section “IRQs and Interrupts” in Chapter 4, when interrupts
are disabled, IRQs issued by I/O devices are temporarily ignored by the PIC,
and no new activity can start on such devices.

To use CPUs efficiently, synchronization primitives based on spin locks should be
avoided whenever possible.

Let’s illustrate a couple of cases in which synchronization can be achieved while still
maintaining a high concurrency level:
• A shared data structure consisting of a single integer value can be updated by
declaring it as an atomic_t type and by using atomic operations. An atomic operation
is faster than spin locks and interrupt disabling, and it slows down only
kernel control paths that concurrently access the data structure.
• Inserting an element into a shared linked list is never atomic, because it consists
of at least two pointer assignments. Nevertheless, the kernel can sometimes perform
this insertion operation without using locks or disabling interrupts.
*/


/*
//Choosing Among Spin Locks, Semaphores, and Interrupt Disabling

Generally speaking, choosing
the synchronization primitives depends on what kinds of kernel control paths access
the data structure, as shown in Table 5-8. Remember that whenever a kernel control
path acquires a spin lock (as well as a read/write lock, a seqlock, or a RCU “read
lock”), disables the local interrupts, or disables the local softirqs, kernel preemption
is automatically disabled.

Table 5-8. Protection required by data structures accessed by kernel control paths
Kernel control paths accessing the data structure UP protection MP further protection
Exceptions Semaphore None
Interrupts Local interrupt disabling Spin lock
Deferrable functions None None or spin lock (see Table 5-9)
Exceptions + Interrupts Local interrupt disabling Spin lock
Exceptions + Deferrable functions Local softirq disabling Spin lock
Interrupts + Deferrable functions Local interrupt disabling Spin lock
Exceptions + Interrupts + Deferrable functions Local interrupt disabling Spin lock

*/

#define Protecting a data structure accessed by exceptions

#define Protecting a data structure accessed by interrupts
/*
Table 5-9. Interrupt-aware spin lock macros
Macro Description
spin_lock_irq(l) local_irq_disable(); spin_lock(l)
spin_unlock_irq(l) spin_unlock(l); local_irq_enable()
spin_lock_bh(l) local_bh_disable(); spin_lock(l)
spin_unlock_bh(l) spin_unlock(l); local_bh_enable()
spin_lock_irqsave(l,f) local_irq_save(f); spin_lock(l)
spin_unlock_irqrestore(l,f) spin_unlock(l); local_irq_restore(f)
read_lock_irq(l) local_irq_disable( ); read_lock(l)
read_unlock_irq(l) read_unlock(l); local_irq_enable( )
read_lock_bh(l) local_bh_disable( ); read_lock(l)
read_unlock_bh(l) read_unlock(l); local_bh_enable( )
write_lock_irq(l) local_irq_disable(); write_lock(l)
write_unlock_irq(l) write_unlock(l); local_irq_enable( )
write_lock_bh(l) local_bh_disable(); write_lock(l)
write_unlock_bh(l) write_unlock(l); local_bh_enable( )
read_lock_irqsave(l,f) local_irq_save(f); read_lock(l)
read_unlock_irqrestore(l,f) read_unlock(l); local_irq_restore(f)
write_lock_irqsave(l,f) local_irq_save(f); write_lock(l)
write_unlock_irqrestore(l,f) write_unlock(l); local_irq_restore(f)
read_seqbegin_irqsave(l,f) local_irq_save(f); read_seqbegin(l)
read_seqretry_irqrestore(l,v,f) read_seqretry(l,v); local_irq_restore(f)
write_seqlock_irqsave(l,f) local_irq_save(f); write_seqlock(l)
write_sequnlock_irqrestore(l,f) write_sequnlock(l); local_irq_restore(f)
write_seqlock_irq(l) local_irq_disable(); write_seqlock(l)
write_sequnlock_irq(l) write_sequnlock(l); local_irq_enable()
write_seqlock_bh(l) local_bh_disable(); write_seqlock(l);
write_sequnlock_bh(l) write_sequnlock(l); local_bh_enable()
*/



#define Protecting a data structure accessed by deferrable functions
/*
Table 5-10. Protection required by data structures accessed by deferrable functions in SMP
Deferrable functions accessing the data structure Protection
Softirqs Spin lock
One tasklet None
Many tasklets Spin lock
*/

#define Protecting a data structure accessed by exceptions and interrupts
#define Protecting a data structure accessed by exceptions and deferrable functions
#define Protecting a data structure accessed by interrupts and deferrable functions
#define Protecting a data structure accessed by exceptions, interrupts, and deferrable functions

#endif //end of Synchronizing Accesses to Kernel Data Structures


//Examples of Race Condition Prevention
#define Reference Counters
#define The Big Kernel Lock
#define Memory Descriptor Read/Write Semaphore
#define Slab Cache List Semaphore
#define Inode Semaphore


#endif //end of __KERNEL_SYNCHRONIZATION__H