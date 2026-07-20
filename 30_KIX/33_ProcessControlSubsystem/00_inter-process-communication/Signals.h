#ifndef __SIGNALS__H
#define __SIGNALS__H

/*
Signals were introduced by the first Unix systems to allow interactions between User
Mode processes; the kernel also uses them to notify processes of system events. Signals
have been around for 30 years with only minor changes.

A signal is a very short message that may be sent to a process or a group of processes.
The only information given to the process is usually a number identifying the
signal; there is no room in standard signals for arguments, a message, or other
accompanying information.

A set of macros whose names start with the prefix SIG is used to identify signals.

Signals serve two main purposes:
• To make a process aware that a specific event has occurred
• To cause a process to execute a signal handler function included in its code

such signals  are architecture-dependent;
*/

/*
Table 11-1. The first 31 signals in Linux/i386
# Signal name Default action Comment POSIX
1 SIGHUP Terminate Hang up controlling terminal or process Yes
2 SIGINT Terminate Interrupt from keyboard Yes
3 SIGQUIT Dump Quit from keyboard Yes
4 SIGILL Dump Illegal instruction Yes
5 SIGTRAP Dump Breakpoint for debugging No
6 SIGABRT Dump Abnormal termination Yes
6 SIGIOT Dump Equivalent to SIGABRT No
7 SIGBUS Dump Bus error No
8 SIGFPE Dump Floating-point exception Yes
9 SIGKILL Terminate Forced-process termination Yes
10 SIGUSR1 Terminate Available to processes Yes
11 SIGSEGV Dump Invalid memory reference Yes
12 SIGUSR2 Terminate Available to processes Yes
13 SIGPIPE Terminate Write to pipe with no readers Yes
14 SIGALRM Terminate Real-timerclock Yes
15 SIGTERM Terminate Process termination Yes
16 SIGSTKFLT Terminate Coprocessor stack error No
17 SIGCHLD Ignore Child process stopped or terminated, or got signal if
traced
Yes
18 SIGCONT Continue Resume execution, if stopped Yes
19 SIGSTOP Stop Stop process execution Yes
20 SIGTSTP Stop Stop process issued from tty Yes
21 SIGTTIN Stop Background process requires input Yes
22 SIGTTOU Stop Background process requires output Yes
23 SIGURG Ignore Urgent condition on socket No
24 SIGXCPU Dump CPU time limit exceeded No
25 SIGXFSZ Dump File size limit exceeded No
26 SIGVTALRM Terminate Virtual timer clock No
27 SIGPROF Terminate Profile timer clock No
28 SIGWINCH Ignore Window resizing No
29 SIGIO Terminate I/O now possible No
29 SIGPOLL Terminate Equivalent to SIGIO No
30 SIGPWR Terminate Power supply failure No
31 SIGSYS Dump Bad system call No
31 SIGUNUSED Dump Equivalent to SIGSYS No
*/

/*
POSIX standard has introduced
a new class of signals denoted as real-time signals; their signal numbers range
from 32 to 64 on Linux. They mainly differ from regular signals because they are
always queued so that multiple signals sent will be received. On the other hand, regular
signals of the same kind are not queued: if a regular signal is sent many times in
a row, just one of them is delivered to the receiving process. Although the Linux kernel
does not use real-time signals, it fully supports the POSIX standard by means of
several specific system calls.
*/

/*
A number of system calls allow programmers to send signals and determine how
their processes respond to the signals they receive. Table 11-2 summarizes these
calls; their behavior is described in detail in the later section “System Calls Related to
Signal Handling.”
*/
/*
Table 11-2. The most significant system calls related to signals
System call Description
kill( ) Send a signal to a thread group
tkill() Send a signal to a process
tgkill() Send a signal to a process in a specific thread group
sigaction( ) Change the action associated with a signal
signal( ) Similar to sigaction( )
sigpending( ) Check whether there are pending signals
sigprocmask( ) Modify the set of blocked signals
sigsuspend( ) Wait for a signal
rt_sigaction( ) Change the action associated with a real-time signal
rt_sigpending( ) Check whether there are pending real-time signals
rt_sigprocmask( ) Modify the set of blocked real-time signals
rt_sigqueueinfo( ) Send a real-time signal to a thread group
rt_sigsuspend( ) Wait for a real-time signal
rt_sigtimedwait( ) Similar to rt_sigsuspend( )
*/

/*
An important characteristic of signals is that they may be sent at any time to a process
whose state is usually unpredictable. Signals sent to a process that is not currently
executing must be saved by the kernel until that process resumes execution.
*/

/*
Therefore, the kernel distinguishes two different phases related to signal transmission:
Signal generation
The kernel updates a data structure of the destination process to represent that a
new signal has been sent.
Signal delivery
The kernel forces the destination process to react to the signal by changing its
execution state, by starting the execution of a specified signal handler, or both.
*/


/*
Actions Performed upon Delivering a Signal
There are three ways in which a process can respond to a signal:
1. Explicitly ignore the signal.
2. Execute the default action associated with the signal (see Table 11-1). This
action, which is predefined by the kernel, depends on the signal type and may be
any one of the following:
Terminate
The process is terminated (killed).
Dump
The process is terminated (killed) and a core file containing its execution
context is created, if possible; this file may be used for debug purposes.
Ignore
The signal is ignored.
Stop
The process is stopped—i.e., put in the TASK_STOPPED state (see the section
“Process State” in Chapter 3).
Continue
If the process was stopped (TASK_STOPPED), it is put into the TASK_RUNNING
state.
3. Catch the signal by invoking a corresponding signal-handler function.
*/


#define POSIX Signals and Multithreaded Applications
/*
The POSIX 1003.1 standard has some stringent requirements for signal handling of
multithreaded applications:
• Signal handlers must be shared among all threads of a multithreaded application;
however, each thread must have its own mask of pending and blocked signals.
• The kill() and sigqueue() POSIX library functions (see the later section “System
Calls Related to Signal Handling”) must send signals to whole multithreaded
applications, not to a specific thread. The same holds for all signals
(such as SIGCHLD, SIGINT, or SIGQUIT) generated by the kernel.
• Each signal sent to a multithreaded application will be delivered to just one
thread, which is arbitrarily chosen by the kernel among the threads that are not
blocking that signal.
• If a fatal signal is sent to a multithreaded application, the kernel will kill all
threads of
*/


#define Data Structures Associated with Signals
/*
For each process in the system, the kernel must keep track of what signals are currently
pending or masked; the kernel must also keep track of how every thread group
is supposed to handle every signal. To do this, the kernel uses several data structures
accessible from the process descriptor.
*/
/*
Table 11-3. Process descriptor fields related to signal handling
Type Name Description
struct signal_struct * signal Pointer to the process’s signal descriptor
struct sighand_struct * sighand Pointer to the process’s signal handler descriptor
sigset_t blocked Mask of blocked signals
sigset_t real_blocked Temporary mask of blocked signals (used by the
rt_sigtimedwait() system call)
struct sigpending pending Data structure storing the private pending signals
unsigned long sas_ss_sp Address of alternative signal handler stack
size_t sas_ss_size Size of alternative signal handler stack
int (*) (void *) notifier Pointer to a function used by a device driver to block
some signals of the process
void * notifier_data Pointer to data that might be used by the notifier
function (previous field of table)
sigset_t * notifier_mask Bit mask of signals blocked by a device driver
through a notifier function
*/
typedef struct {
unsigned long sig[2];
} sigset_t;

#define The signal descriptor and the signal handler descriptor
/*
Table 11-4. The fields of the signal descriptor related to signal handling
Type Name Description
atomic_t count Usage counter of the signal descriptor
atomic_t live Number of live processes in the thread group
wait_queue_head_t wait_chldexit Wait queue for the processes sleeping in a wait4()
system call
struct task_struct * curr_target Descriptor of the last process in the thread group
that received a signal
struct sigpending shared_pending Data structure storing the shared pending signals
int group_exit_code Process termination code for the thread group
struct task_struct * group_exit_task Used when killing a whole thread group
int notify_count Used when killing a whole thread group
int group_stop_count Used when stopping a whole thread group
unsigned int flags Flags used when delivering signals that modify the
status of the process
*/
/*
Table 11-5. The fields of the signal handler descriptor
Type Name Description
atomic_t count Usage counter of the signal handler descriptor
struct k_sigaction [64] action Array of structures specifying the actions to be performed upon
delivering the signals
spinlock_t siglock Spin lock protecting both the signal descriptor and the signal
handler descriptor
*/


#define The sigaction data structure //Some architectures assign properties to a signal that are visible only to the kernel. properties of a signal are stored in a k_sigaction structure
/*
Thus the k_sigaction structure simply reduces to a single sa structure of type
sigaction, which includes the following fields:*
sa_handler
This field specifies the type of action to be performed; its value can be a pointer
to the signal handler, SIG_DFL (that is, the value 0) to specify that the default
action is performed, or SIG_IGN (that is, the value 1) to specify that the signal is
ignored.
sa_flags
This set of flags specifies how the signal must be handled; some of them are
listed in Table 11-6.†
sa_mask
This sigset_t variable specifies the signals to be masked when running the signal
handler.
*/

/*
Table 11-6. Flags specifying how to handle a signal
Flag Name Description
SA_NOCLDSTOP Applies only to SIGCHLD; do not send SIGCHLD to the parent when the process is stopped
SA_NOCLDWAIT Applies only to SIGCHLD; do not create a zombie when the process terminates
SA_SIGINFO Provide additional information to the signal handler (see the later section “Changing a Signal
Action”)
SA_ONSTACK Use an alternative stack for the signal handler (see the later section “Catching the Signal”)
SA_RESTART Interrupted system calls are automatically restarted (see the later section “Reexecution of System
Calls”)
SA_NODEFER, SA_
NOMASK
Do not mask the signal while executing the signal handler
SA_RESETHAND,
SA_ONESHOT
Reset to default action after executing the signal handler
*/


#define The pending signal queues
/*
Thus, in order to keep track of what signals are currently pending, the kernel associates
two pending signal queues to each process:
• The shared pending signal queue, rooted at the shared_pending field of the signal
descriptor, stores the pending signals of the whole thread group.
• The private pending signal queue, rooted at the pending field of the process
descriptor, stores the pending signals of the specific (lightweight) process.
A pending signal queue consists of a sigpending data structure, which is defined as
follows:
struct sigpending {
struct list_head list;
sigset_t signal;
}
*/
/*
The signal field is a bit mask specifying the pending signals, while the list field is
the head of a doubly linked list containing sigqueue data structures; the fields of this
structure are shown in Table 11-7.

Table 11-7. The fields of the sigqueue data structure
Type Name Description
struct list_head list Links for the pending signal queue’s list
spinlock_t * lock Pointer to the siglock field in the signal handler descriptor corresponding to
the pending signal
int flags Flags of the sigqueue data structure
siginfo_t info Describes the event that raised the signal
struct
user_struct *
user Pointer to the per-user data structure of the process’s owner (see the section
“The clone( ), fork( ), and vfork( ) System Calls” in Chapter 3)

The siginfo_t data structure is a 128-byte data structure that stores information
about an occurrence of a specific signal; it includes the following fields:
si_signo
The signal number
si_errno
The error code of the instruction that caused the signal to be raised, or 0 if there
was no error
si_code
A code identifying who raised the signal (see Table 11-8)

Table 11-8. The most significant signal sender codes
Code Name Sender
SI_USER kill() and raise() (see the later section “System Calls Related to Signal Handling”)
SI_KERNEL Generic kernel function
SI_QUEUE sigqueue() (see the later section “System Calls Related to Signal Handling”)
SI_TIMER Timer expiration
SI_ASYNCIO Asynchronous I/O completion
SI_TKILL tkill() and tgkill() (see the later section “System Calls Related to Signal Handling”)
*/


#define Operations on Signal Data Structures
/*
Several functions and macros are used by the kernel to handle signals. In the following
description, set is a pointer to a sigset_t variable, nsig is the number of a signal,
and mask is an unsigned long bit mask.
*/

sigemptyset(set);
sigfillset(set);
sigaddset(set,nsig);
sigdelset(set,nsig);
sigaddsetmask(set,mask);
sigdelsetmask(set,mask);
sigismember(set,nsig);
sigmask(nsig);
sigandsets(d,s1,s2);
sigorsets(d,s1,s2);
signandsets(d,s1,s2);
sigtestsetmask(set,mask);
siginitset(set,mask);
siginitsetinv(set,mask);
signal_pending(p);
recalc_sigpending_tsk(t);
recalc_sigpending();
rm_from_queue(mask,q);
flush_sigqueue(q);
flush_signals(t);


#define Generating a Signal
/*
When a signal is sent to a process, either from the kernel or from another process,
the kernel generates it by invoking one of the functions listed in Table 11-9.
Table 11-9. Kernel functions that generate a signal for a process
Name Description
send_sig() Sends a signal to a single process
send_sig_info() Like send_sig(), with extended information in a siginfo_t structure
force_sig() Sends a signal that cannot be explicitly ignored or blocked by the process
force_sig_info() Like force_sig(), with extended information in a siginfo_t structure
force_sig_specific() Like force_sig(), but optimized for SIGSTOP and SIGKILL signals
sys_tkill() System call handler of tkill() (see the later section “System Calls Related to Signal Handling”)
sys_tgkill() System call handler of tgkill()


When a signal is sent to a whole thread group, either from the kernel or from
another process, the kernel generates it by invoking one of the functions listed in
Table 11-10.

Table 11-10. Kernel functions that generate a signal for a thread group
Name Description
send_group_sig_info() Sends a signal to a single thread group identified by the process descriptor of one of
its members
kill_pg() Sends a signal to all thread groups in a process group (see the section “Process
Management” in Chapter 1)
kill_pg_info() Like kill_pg(), with extended information in a siginfo_t structure
kill_proc() Sends a signal to a single thread group identified by the PID of one of its members
kill_proc_info() Like kill_proc(), with extended information in a siginfo_t structure
sys_kill() System call handler of kill() (see the later section “System Calls Related to Signal
Handling”)
sys_rt_sigqueueinfo() System call handler of rt_sigqueueinfo()

*/

specific_send_sig_info();
send_signal();
group_send_sig_info();



#define Delivering a Signal

#define Executing the Default Action for the Signal



#define Catching the Signal
setup_frame( );
//Evaluating the signal flags
//Starting the signal handler
//Terminating the signal handler



#define Reexecution of System Calls
#ifdef Reexecution of System Calls

#define Terminate
#define Reexecute
#define Depends
/*
Table 11-11. Reexecution of system calls
Error codes and their impact on system call execution

SignalAction EINTR ERESTARTSYS ERESTARTNOHANDERESTART_RESTARTBLOCK ERESTARTNOINTR
Default Terminate Reexecute Reexecute Reexecute
Ignore Terminate Reexecute Reexecute Reexecute
Catch Terminate Depends Terminate Reexecute
*/



#endif // end of Reexecution of System Calls


#define Restarting a system call interrupted by a non-caught signal


#define Restarting a system call for a caught signal


#define System Calls Related to Signal Handling
#ifdef System Calls Related to Signal Handling

kill( );
tkill( );
tgkill( );

#define Changing a Signal Action
#define Examining the Pending Blocked Signals
#define Modifying the Set of Blocked Signals
#define Suspending the Process

#define System Calls for Real-Time Signals
rt_sigqueueinfo( );
rt_sigtimedwait( );


#endif // end of System Calls Related to Signal Handling


#endif //end of __SIGNALS__H