#ifndef __PROCESSS_SCHEDULING_H
#define __PROCESSS_SCHEDULING_H


#define scheduling_policy // choice make to abstract to schedule process
#define scheduling_Algorithm // data structure used to implement scheduling and the corresponding algorithm
#define system_call_for_scheduling // system calls that affact process scheduling




#ifdef scheduling_policy
/*
operating systems must fulfill several
conflicting objectives: fast process response time, good throughput for background
jobs, avoidance of process starvation, reconciliation of the needs of low- and highpriority
processes, and so on. The set of rules used to determine when and how to
select a new process to run is called scheduling policy
*/

#define I/O-bound_or_CPU-bound 

/* Interactive processes: 1. These interact constantly with their users, and therefore spend a lot of time waiting
for keypresses and mouse operations.
2. Batch processes
These do not need user interaction, and hence they often run in the background.
3.Real-time processes
These have very stringent scheduling requirements.
*/
#define PROCESS Interactive_processes // 1. Interactive processes 2. Batch processes 3.Real-time processes

/*
//System calls related to scheduling
nice( ) Change the static priority of a conventional process
getpriority( ) Get the maximum static priority of a group of conventional processes
setpriority( ) Set the static priority of a group of conventional processes
sched_getscheduler( ) Get the scheduling policy of a process
sched_setscheduler( ) Set the scheduling policy and the real-time priority of a process
sched_getparam( ) Get the real-time priority of a process
sched_setparam( ) Set the real-time priority of a process
sched_yield( ) Relinquish the processor voluntarily without blocking
sched_get_priority_min( ) Get the minimum real-time priority value for a policy
sched_get_priority_max( ) Get the maximum real-time priority value for a policy
sched_rr_get_interval( ) Get the time quantum value for the Round Robin policy
sched_setaffinity() Set the CPU affinity mask of a process
sched_getaffinity() Get the CPU affinity mask of a process
*/

#define ProcessPreemtion


#endif // end of scheduling_policy





#ifdef scheduling_Algorithm

/*
1.A First-In, First-Out real-time process. When the scheduler assigns the CPU to
the process, it leaves the process descriptor in its current position in the runqueue
list.
2.A Round Robin real-time process. When the scheduler assigns the CPU to the
process, it puts the process descriptor at the end of the runqueue list.
3.The scheduling algorithm behaves quite differently depending on whether the process
is conventional or real-time.
*/

#define SCHEDULING_CLASS SCHED_FIFO // 1.SCHED_FIFO 2.SCHED_RR 3.SCHED_NORMAL(time sharing)

/*
Static Priorties:
base time quantum
(in milliseconds) = 
(140 – static priority) × 20 if static priority < 120
140 static priority – ( ) 5 × if static priority 120

Besides a static priority, a conventional process also has a dynamic priority, which is
a value ranging from 100 (highest priority) to 139 (lowest priority).

dynamic priority = max( 100, min( static priority − bonus + 5, 139 ))
*/

#define The runqueue Data Structure
/*
all runqueue structures are stored in
the runqueues per-CPU variable
*/

#define this_rq() //the address of the runqueue of the local CPU
#define cpu_rq(n) // address of the runqueue of the CPU having index n

struct runqueue{

    spinlock_t lock //Spin lock protecting the lists of processes
    unsigned long nr_running //Number of runnable processes in the runqueue lists
    nsigned long cpu_load CPU //load factor based on the average number of processes in the runqueue
    unsigned long nr_switches //Number of process switches performed by the CPU
    unsigned long nr_uninterruptible //Number of processes that were previously in the runqueue lists and are now sleeping in TASK_UNINTERRUPTIBLE state (only the sum of these fields across all runqueues is meaningful)
    unsigned long expired_timestamp //Insertion time of the eldest process in the expired lists
    unsigned long long timestamp_last_tick// Timestamp value of the last timer interrupt task_t * curr Process descriptor pointer of the currently running process (same as current for the local CPU)
    task_t * idle //Process descriptor pointer of the swapper process for this CPU
    struct mm_struct * prev_mm //Used during a process switch to store the address of the memory descriptor of the process being replaced
    prio_array_t * active //Pointer to the lists of active processes
    prio_array_t * expired //Pointer to the lists of expired processes
    prio_array_t [2] arrays //The two sets of active and expired processes
    int best_expired_prio //The best static priority (lowest value) among the expired processes
    atomic_t nr_iowait// Number of processes that were previously in the runqueue lists and are now waiting for a disk I/O operation to complete
    struct sched_domain *sd// Points to the base scheduling domain of this CPU (see the section “Scheduling Domains” later in this chapter)
    int active_balance //Flag set if some process shall be migrated from this runqueue to another (runqueue balancing)
    int push_cpu //Not used
    task_t * migration_thread //Process descriptor pointer of the migration kernel thread
    struct list_head migration_queue //List of processes to be removed from the runqueue
};

/*
Periodically, the role of the two data structures in arrays changes: the active processes
suddenly become the expired processes, and the expired processes become the
active ones. To achieve this change, the scheduler simply exchanges the contents of
the active and expired fields of the runqueue.
*/

struct process
{
    unsigned long thread_info->flags //Stores the TIF_NEED_RESCHED flag, which is set if the scheduler must be invoked (see the section “Returning from Interrupts and Exceptions” in Chapter 4)
    unsigned int thread_info->cpu Logical number of the CPU owning the runqueue to which the runnable process belongs
    unsigned long state The current state of the process (see the section “Process State” in Chapter 3)
    int prio Dynamic priority of the process
    int static_prio Static priority of the process
    struct list_head run_list Pointers to the next and previous elements in the runqueue list to which the process belongs
    prio_array_t * array //Pointer to the runqueue’s prio_array_t set that includes the process
    unsigned long sleep_avg //Average sleep time of the process
    unsigned long long timestamp //Time of last insertion of the process in the runqueue, or time of last process switch involving the process
    unsigned long long last_ran //Time of last process switch that replaced the process
    int activated Condition// code used when the process is awakened
    unsigned long policy//The scheduling class of the process (SCHED_NORMAL, SCHED_RR, or SCHED_FIFO)
    cpumask_t cpus_allowed// Bit mask of the CPUs that can execute the process
    unsigned int time_slice //Ticks left in the time quantum of the process
    unsigned int first_time_slice //Flag set to 1 if the process never exhausted its timequantum
    unsigned long rt_priority// Real-time priority of the process
};

#define Functions Used by the Scheduler
scheduler_tick();
try_to_wake_up();
recalc_task_prio();
schedule();
load_balance();


#define Runqueue Balancing in Multiprocessor Systems
//Symmetric Multiprocessing model (SMP)
//will consider the following three types of multiprocessor machines:

#define Classic multiprocessor architecture //machines have a common set of RAM chips shared by all CPUs

#define Hyper-threading 
/*
A hyper-threaded chip is a microprocessor that executes several threads of execution
at once; it includes several copies of the internal registers and quickly
switches between them.
*/

#define NUMA // in chapter 8
/*
CPUs and RAM chips are grouped in local “nodes” (usually a node includes one
CPU and a few RAM chips). The memory arbiter (a special circuit that serializes
the accesses to RAM performed by the CPUs in the system
*/


#define Scheduling Domains //a scheduling domain is a set of CPUs whose workloads should be kept balanced by the kernel.
/*
Every scheduling domain is partitioned, in turn, in one or more groups, each of which
represents a subset of the CPUs of the scheduling domain. Workload balancing is
always done between groups of a scheduling domain.
*/

struct sched_group{
//either NUMA, Hyper-threading  or Classic multiprocessor architecture

};

struct sched_domain{


};

#define phys_domains

rebalance_tick();
load_balance();
move_tasks();


#endif  // end of scheduling_Algorithm







#ifdef system_call_for_scheduling

nice( );
getpriority( );
setpriority( );
sched_getaffinity();
sched_setaffinity( );
sched_getscheduler( );
sched_setscheduler( );
sched_getparam( );
sched_setparam( );
sched_yield( );
sched_get_priority_min( );
sched_get_priority_max( );
sched_rr_get_interval( );

#endif // end of system_call_for_scheduling



#endif // end of __PROCESSS_SCHEDULING_H
