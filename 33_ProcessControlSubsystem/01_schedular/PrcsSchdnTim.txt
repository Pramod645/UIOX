/*
On a time sharing system, the kernel allocates the CPU to a process for a period of
time called a time slice or time quantum, preempts the process and schedules
another one when the time slice expires, and reschedules the process to continue
execution at a later time.
The scheduler function on the system uses
relative time of execution as a parameter to determine which process to schedule
next. Every active process has a scheduling priority; the kernel switches context to
that of the process with the highest priority when it does a context switch. The
kernel recalculates the priority of the running process when it returns from kernel
mode to user mode, and it periodically readjusts the priority of every "ready-to·
run" process in user mode.
//Each occurrence of a clock interrupt is called a clock tick .
*/
/*
//The scheduler on the UNIX system belongs to the general class of operating system
schedulers known as round robin with multilevel feedback, meaning that the kernel
allocates the CPU to a process for a time quantum, preempts a process that exceeds
its time quantum, and feeds it back into one of several priority queues. A process
may need many iterations through the "feedback loop" before it finishes. When
the kernel does a context switch and restores the context of a process, the process
resumes execution from the point where it had been suspended.

1. Algorithm schedule_process
input: none
output: none
{
	while (no process picked to execute)
	{
		for (every process on run queue)
			pick highest priority process that is loaded in memory;
	if (no process eligible to execute)
		idle the machine;
		//interrupt takes machine out of idle state
	}
	remove chosen process from run queue;
	switch context to that of chosen process, resume its execution;
}

1.algorithm
2. scheduling parameters
3.controlling process scheduling
4.fair share schedular
5.real time processing
*/

/*
struct tms
{
	//tiipe_t is the data structure for time
	time_t tms_utime;     // user time of process 
	time_t tms_stime;     //kernel time of process 
	time t tms_cutime;    //user time of children 
    time _t tms cstime    //kernel time of children	
}
*/

/*
2. Algorithm clock
input: none
output: none
{
	restart clock;        //so that it will interrupt again
	if (callout table not empty)
	{
		adjust callout times;
		schedule callout function if time elapsed;
	}
	if (kernel profiling on)
		note program counter at time of interrupt;
	if (user profiling on)
		note program counter at time of interrupt;
	gather system statistics;
	gather statistics per process;
	adjust measure of propess CPU utilitization;
	if (1 second or more since last here and interrupt not in critical region of code)
	{
		for (all processes in the system)
		{
			adjust alarm time if active;
			adjust measure of CPU utilization;
			if (process to execute in user mode)
				adjust process priority;
		}
		wakeup swapper process is necessary;
	}
}
*/
/*
3. Algorithm Profilling


*/