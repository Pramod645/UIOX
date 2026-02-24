#ifndef __TIMING_MEASUREMENTS_H
#define __TIMING_MEASUREMENTS_H

/*
We can distinguish two main kinds of timing measurement that must be performed
by the Linux kernel:
• Keeping the current time and date so they can be returned to user programs
through the time( ), ftime( ), and gettimeofday( ) APIs (see the section “The
time( ) and gettimeofday( ) System Calls” ) and used by the
kernel itself as timestamps for files and network packets
• Maintaining timers—mechanisms that are able to notify the kernel (see the later
 “Software Timers and Delay Functions”) or a user program (see the later
 “The setitimer( ) and alarm( ) System Calls” and “System Calls for
POSIX Timers”) that a certain interval of time has elapsed
*/
/*
Timing measurements are performed by several hardware circuits based on fixedfrequency
oscillators and counters. This  consists of four different parts. The
first two sections describe the hardware devices that underly timing and give an overall
picture of Linux timekeeping architecture. The following sections describe the
main time-related duties of the kernel: implementing CPU time sharing, updating
system time and resource usage statistics, and maintaining software timers. The last
 discusses the system calls related to timing measurements and the corresponding
service routines.
*/

#define fixedFrequency
#define counters

#define HARDWARE_DEVICE
#define 2
#define CPU_TIME_SHARING
#define SYSTEM_TIME
#define SOFTWARE_TIMERS



//1.
#define Clock and Timer Circuits
#ifdef Clock and Timer Circuits

#define Real Time Clock //(RTC) // RTC is capable of issuing periodic interrupts on IRQ8 , It can also be programmed to activate the IRQ8 line when the RTC reaches a specific value, thus working as an alarm clock.
/*
Linux uses the RTC only to derive the time and date
it allows processes to program the RTC by acting on the /dev/rtc device file
The kernel accesses the RTC through the 0x70 and 0x71 I/O ports.
*/

#define Time Stamp Counter //(TSC)
/*
microprocessors include a CLK input pin, which receives the clock signal of an external oscillator
counter is accessible through the 64-bit Time Stamp Counter (TSC) register
the kernel has to take into consideration the frequency of the clock signal: if, for instance, the clock ticks at 1 GHz, the Time Stamp Counter is increased once every nanosecond.
*/
#define COUNETR 0x00000000
calibrate_tsc( );



#define Programmable Interval Timer //(PIT)
/*
role of a PIT is similar to the alarm clock of a microwave oven: it makes
the user aware that the cooking time interval has elapsed. Instead of ringing a bell,
this device issues a special interrupt called timer interrupt, which notifies the kernel
that one more time interval has elapse

interrupts on the IRQ0 at a (roughly) 1000-Hz frequency—
that is, once every 1 millisecond. This time interval is called a tick, and its length in
nanoseconds is stored in the tick_nsec variable.
*/
#define HZ
#define CLOCK_TICK_RATE
#define LATCH // ratio between CLOCK_TICK_RATE and HZ,




#define CPU Local Timer
/*
The CPU local timer is a device similar to the Programmable Interval Timer just
described that can issue one-shot or periodic interrupts.

• The APIC’s timer counter is 32 bits long, while the PIT’s timer counter is 16 bits
long; therefore, the local timer can be programmed to issue interrupts at very
low frequencies
• The local APIC timer sends an interrupt only to its processor, while the PIT
raises a global interrupt, which may be handled by any CPU in the system.
• The APIC’s timer is based on the bus clock signal (or the APIC bus signal, in
older machines).
*/


#define High Precision Event Timer //(HPET)
/*

*/



#define ACPI Power Management Timer
/*
The ACPI Power Management Timer is preferable to the TSC if the operating system
or the BIOS may dynamically lower the frequency or voltage of the CPU to save
battery power.

*/

#endif //Clock and Timer Circuits





//2.
#define The Linux Timekeeping Architecture
#ifdef The Linux Timekeeping Architecture

#define Data Structures of the Timekeeping Architecture
#ifdef Data Structures of the Timekeeping Architecture
//
/*
In order to handle the possible timer sources in a uniform way, the kernel makes use
of a “timer object,” which is a descriptor of type timer_opts consisting of the timer
name and of four standard methods

Table 6-1. The fields of the timer_opts data structure
Field name Description
name A string identifying the timer source
mark_offset Records the exact time of the last tick; it is invoked by the timer interrupt handler
get_offset Returns the time elapsed since the last tick
monotonic_clock Returns the number of nanoseconds since the kernel initialization
delay Waits for a given number of “loops” (see the later section “Delay Functions”)
*/
select_timer();
/*
Table 6-2. Typical timer objects of the 80x86 architecture, in order of preference
Timer object name Description Time interpolation Delay
timer_hpet High Precision Event Timer (HPET) HPET HPET
timer_pmtmr ACPI Power Management Timer (ACPI PMT) ACPI PMT TSC
timer_tsc Time Stamp Counter (TSC) TSC TSC
timer_pit Programmable Interval Timer (PIT) PIT Tight loop
timer_none Generic dummy timer source
(used during kernel initialization)
(none) Tight loop
*/

unsigned uint jiffies; // The jiffies variable is a counter that stores the number of elapsed ticks since the system was started.
unsigned long long uint jiffies_64;
//jiffies_64 variable must be protected by means of write_seqlock(&xtime_lock) and write_sequnlock(&xtime_lock)

struct  timespec
{
    tv_sec; //Stores the number of seconds that have elapsed since midnight of January 1, 1970 (UTC)
    tv_nsec; //Stores the number of nanoseconds that have elapsed within the last second (its value ranges between 0 and 999,999,999)
}xtime;

#define Timekeeping Architecture in Uniprocessor Systems
time_init( );
get_cmos_time();


#define Timekeeping Architecture in Multiprocessor Systems


#endif // end of Data Structures of the Timekeeping Architecture

#endif //end of The Linux Timekeeping Architecture








//3.
#define Updating the Time and Date
#ifdef Updating the Time and Date

update_times(void);


#endif //end of Updating the Time and Date



//4.
#define Updating System Statistics
#ifdef Updating System Statistics


/*
The kernel, among the other time-related duties, must periodically collect various
data used for:
• Checking the CPU resource limit of the running processes
• Updating statistics about the local CPU workload
• Computing the average system load
• Profiling the kernel code
*/

#define Updating Local CPU Statistics
update_process_times();



#define Keeping Track of System Load
/*
At every tick, update_times() invokes the calc_load() function, which counts the
number of processes in the TASK_RUNNING or TASK_UNINTERRUPTIBLE state and uses this
number to update the average system load.
*/
#define Profiling the Kernel Code
//readprofile: discover where the kernel spends its time in Kernel Mode. The profiler identifies the hot spots of the kernel
profile_tick();
do_timer_interrupt();
smp_local_timer_interrupt();
timer_notify();

#define Checking the NMI Watchdogs
/*
a watchdog system, which might be quite useful to detect kernel bugs that cause a system
freeze. To activate such a watchdog, the kernel must be booted with the nmi_
watchdog parameter.
*/



#endif //end of Updating System Statistics


//5.
#define Software Timers and Delay Functions
#ifdef Software Timers and Delay Functions
/*
A timer is a software facility that allows functions to be invoked at some future
moment, after a given time interval has elapsed;
*/
#define Dynamic Timers
/*
Dynamic timers may be dynamically created and destroyed. No limit is placed on the
number of currently active dynamic timers.
*/
//A dynamic timer is stored in the following timer_list structure:
struct timer_list {
    struct list_head entry;
    unsigned long expires;
    spinlock_t lock;
    unsigned long magic;
    void (*function)(unsigned long); //The function field contains the address of the function to be executed when the timer expires.
    unsigned long data; //The data field specifies a parameter to be passed to this timer function
    tvec_base_t *base;
};
/*
The expires field specifies when the timer expires; the time is expressed as the number
of ticks that have elapsed since the system started up. All timers that have an
expires value smaller than or equal to the value of jiffies are considered to be
expired or decayed.
*/

#define Data structures for dynamic timers
/*
Choosing the proper data structure to implement dynamic timers is not easy. Stringing
together all timers in a single list would degrade system performance, because
scanning a long list of timers at every tick is costly. On the other hand, maintaining a
sorted list would not be much more efficient, because the insertion and deletion
operations would also be costly.
*/
//Each element is a tvec_base_t structure, which includes all data needed to handle the dynamic timers bound to the corresponding CPU:
typedef struct tvec_t_base_s {
spinlock_t lock;
unsigned long timer_jiffies;
struct timer_list *running_timer;
tvec_root_t tv1;
tvec_t tv2;
tvec_t tv3;
tvec_t tv4;
tvec_t tv5;
} tvec_base_t;

#define Dynamic timer handling
#define Delay Functions
udelay(unsigned long usecs);
ndelay(unsigned long nsecs);
calibrate_delay();

#endif //end of Software Timers and Delay Functions









//6.
#define System Calls Related to Timing Measurements
#ifdef System Calls Related to Timing Measurements
//Processes in User Mode can get the current time and date by means of several system calls:
time( ); //Returns the number of elapsed seconds since midnight at the start of January 1, 1970 (UTC).
gettimeofday( ); //Returns, in a data structure named timeval, the number of elapsed seconds since midnight of January 1, 1970 (UTC) and the number of elapsed microseconds in the last second (a second data structure named timezone is not currently used).
adjtimex( );
setitimer( );
alarm( );


#define System Calls for POSIX Timers
/*
Table 6-3. System calls for POSIX timers and clocks
System call Description
clock_gettime() Gets the current value of a POSIX clock
clock_settime() Sets the current value of a POSIX clock
clock_getres() Gets the resolution of a POSIX clock
timer_create() Creates a new POSIX timer based on a specified POSIX clock
timer_gettime() Gets the current value and increment of a POSIX timer
timer_settime() Sets the current value and increment of a POSIX timer
timer_getoverrun() Gets the number of overruns of a decayed POSIX timer
timer_delete() Destroys a POSIX timer
clock_nanosleep() Puts the process to sleep using a POSIX clock as time source
*/
//kernel offers two types of POSIX clocks
CLOCK_REALTIME; // //This virtual clock represents the real-time clock of the system—essentially the value of the xtime variable
CLOCK_MONOTONIC;//This virtual clock represents the real-time clock of the system purged of every time warp due to the synchronization with an external time source.


#endif //end of System Calls Related to Timing Measurements




#endif // end of __TIMING_MEASUREMENTS_H
