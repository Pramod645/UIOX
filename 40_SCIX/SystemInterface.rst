#define __SYSTEMINTERFACE__H
#ifdef __SYSTEMINTERFACE__H
/*
Operating systems offer processes running in User Mode a set of interfaces to interact
with hardware devices such as the CPU, disks, and printers.Putting an extra
layer between the application and the hardware has several advantages.

First, it makes programming easier by freeing users from studying low-level programming
characteristics of hardware devices.

Second, it greatly increases system security,
because the kernel can check the accuracy of the request at the interface level before
attempting to satisfy it.

Last but not least, these interfaces make programs more portable,
because they can be compiled and executed correctly on every kernel that
offers the same set of interfaces.

Unix systems implement most interfaces between User Mode processes and hardware
devices by means of system calls issued to the kernel.
*/

#define POSIX APIs and System Calls
#ifdef POSIX APIs and System Calls
/*
Let’s start by stressing the difference between an application programmer interface
(API) and a system call. The former is a function definition that specifies how to
obtain a given service, while the latter is an explicit request to the kernel made via a
software interrupt.

Unix systems include several libraries of functions that provide APIs to programmers.
Some of the APIs defined by the libc standard C library refer to wrapper routines
(routines whose only purpose is to issue a system call). Usually, each system call
has a corresponding wrapper routine, which defines the API that application programs
should employ.
The converse is not true, by the way—an API does not necessarily correspond to a
specific system call. First of all, the API could offer its services directly in User Mode.
(For something abstract such as math functions, there may be no reason to make system
calls.) Second, a single API function could make several system calls. Moreover,
several API functions could make the same system call, but wrap extra functionality
around it. For instance, in Linux, the malloc( ), calloc( ), and free( ) APIs are implemented
in the libc library. The code in this library keeps track of the allocation and
deallocation requests and uses the brk( ) system call to enlarge or shrink the process
heap
From the programmer’s point of view, the distinction between an API and a system
call is irrelevant—the only things that matter are the function name, the parameter
types, and the meaning of the return code. From the kernel designer’s point of view,
however, the distinction does matter because system calls belong to the kernel, while
User Mode libraries don’t.
Most wrapper routines return an integer value, whose meaning depends on the corresponding
system call. A return value of –1 usually indicates that the kernel was
unable to satisfy the process request. A failure in the system call handler may be
caused by invalid parameters, a lack of available resources, hardware problems, and
so on. The specific error code is contained in the errno variable, which is defined in
the libc library.
*/
//POSIX standard: include/asm-i386/errno.h.
//standard /usr/include/errno.h C library header file.

#endif // end of POSIX APIs and System Calls



#define System Call Handler and Service Routines
#ifdef System Call Handler and Service Routines
/*
When a User Mode process invokes a system call, the CPU switches to Kernel Mode
and starts the execution of a kernel function.


nux system call can be invoked in two different ways. The net
result of both methods, however, is a jump to an assembly language function called
the system call handler.

Because the kernel implements many different system calls, the User Mode process
must pass a parameter called the system call number to identify the required system
call; the eax register is used by Linux for this purpose

All system calls return an integer value. The conventions for these return values are
different from those for wrapper routines. In the kernel, positive or 0 values denote a
successful termination of the system call, while negative values denote an error condition.

error code that must be
returned to the application program in the errno variable

The system call handler, which has a structure similar to that of the other exception
handlers, performs the following operations:
• Saves the contents of most registers in the Kernel Mode stack (this operation is
common to all system calls and is coded in assembly language).
• Handles the system call by invoking a corresponding C function called the system
call service routine.
• Exits from the handler: the registers are loaded with the values saved in the Kernel
Mode stack, and the CPU is switched back from Kernel Mode to User Mode
(this operation is common to all system calls and is coded in assembly language).



To associate each system call number with its corresponding service routine, the kernel
uses a system call dispatch table, which is stored in the sys_call_table array and
has NR_syscalls entries (289 in the Linux 2.6.11 kernel). The nth entry contains the
service routine address of the system call having number n.

The NR_syscalls macro is just a static limit on the maximum number of implementable
system calls; it does not indicate the number of system calls actually
implemented. Indeed, each entry of the dispatch table may contain the address of the
sys_ni_syscall( ) function, which is the service routine of the “nonimplemented”
system calls; it just returns the error code -ENOSYS.
*/

#endif // end of System Call Handler and Service Routines
/* -- --------------------------------------------------------------------    */



#define Entering and Exiting a System Call
#ifdef Entering and Exiting a System Call
/*
Native applications* can invoke a system call in two different ways:
• By executing the int $0x80 assembly language instruction; in older versions of
the Linux kernel, this was the only way to switch from User Mode to Kernel
Mode.
• By executing the sysenter assembly language instruction, introduced in the Intel
Pentium II microprocessors; this instruction is now supported by the Linux 2.6
kernel.
Similarly, the kernel can exit from a system call—thus switching the CPU back to
User Mode—in two ways:
• By executing the iret assembly language instruction.
• By executing the sysexit assembly language instruction, which was introduced
in the Intel Pentium II microprocessors together with the sysenter instruction.
However, supporting two different ways to enter the kernel is not as simple as it
might look, because:
• The kernel must support both older libraries that only use the int $0x80 instruction
and more recent ones that also use the sysenter instruction.
• A standard library that makes use of the sysenter instruction must be able to
cope with older kernels that support only the int $0x80 instruction.
• The kernel and the standard library must be able to run both on older processors
that do not include the sysenter instruction and on more recent ones that
include it.
*/

//Issuing a System Call via the int $0x80 Instruction
set_system_gate(0x80, &system_call);
//The system_call( ) function
/*
system_call:
pushl %eax
SAVE_ALL
movl $0xffffe000, %ebx // or 0xfffff000 for 4-KB stacks 
andl %esp, %ebx
*/
//Exiting from the system call


//Issuing a System Call via the sysenter Instruction

//The vsyscall page
//Entering the system call
/*
1. The wrapper routine in the standard library loads the system call number into
the eax register and calls the _ _kernel_vsyscall() function.
*/
//Exiting from the system call
/*
When the system call service routine terminates, the sysenter_entry( ) function executes
essentially the same operations as the system_call() function
*/
//The sysexit instruction
#endif // end of Entering and Exiting a System Call
/* --------------------------------------------------------------------------- */

#define Parameter Passing
#ifdef Parameter Passing
/*
Like ordinary functions, system calls often require some input/output parameters,
which may consist of actual values (i.e., numbers), addresses of variables in the
address space of the User Mode process, or even addresses of data structures including
pointers to User Mode functions

Because the system_call( ) and the sysenter_entry( ) functions are the common
entry points for all system calls in Linux, each of them has at least one parameter: the
system call number passed in the eax register. For instance, if an application program
invokes the fork( ) wrapper routine, the eax register is set to 2 (i.e., __NR_fork)
before executing the int $0x80 or sysenter assembly language instruction. Because
the register is set by the wrapper routines included in the libc library, programmers
do not usually care about the system call number.
The fork( ) system call does not require other parameters. However, many system
calls do require additional parameters, which must be explicitly passed by the application
program. For instance, the mmap( ) system call may require up to six additional
parameters (besides the system call number).

The parameters of ordinary C functions are usually passed by writing their values in
the active program stack (either the User Mode stack or the Kernel Mode stack).
Because system calls are a special kind of function that cross over from user to kernel
land, neither the User Mode or the Kernel Mode stacks can be used. Rather, system
call parameters are written in the CPU registers before issuing the system call.
The kernel then copies the parameters stored in the CPU registers onto the Kernel
Mode stack before invoking the system call service routine, because the latter is an
ordinary C function.

Why doesn’t the kernel copy parameters directly from the User Mode stack to the
Kernel Mode stack? First of all, working with two stacks at the same time is complex;
second, the use of registers makes the structure of the system call handler similar to
that of other exception handlers.
However, to pass parameters in registers, two conditions must be satisfied:
• The length of each parameter cannot exceed the length of a register (32 bits).*
• The number of parameters must not exceed six, besides the system call number
passed in eax, because 80 × 86 processors have a very limited number of registers.
The first condition is always true because, according to the POSIX standard, large
parameters that cannot be stored in a 32-bit register must be passed by reference. A
typical example is the settimeofday( ) system call, which must read a 64-bit structure.

Let’s look at an example. The sys_write( ) service routine, which handles the write( )
system call, is declared as:
int sys_write (unsigned int fd, const char * buf, unsigned int count)
The C compiler produces an assembly language function that expects to find the fd,
buf, and count parameters on top of the stack, right below the return address, in the
locations used to save the contents of the ebx, ecx, and edx registers, respectively.
*/


//Verifying the Parameters

/*
All system call parameters must be carefully checked before the kernel attempts to
satisfy a user request. The type of check depends both on the system call and on the
specific parameter. Let’s go back to the write( ) system call introduced before: the fd
parameter should be a file descriptor that identifies a specific file, so sys_write( )
must check whether fd really is a file descriptor of a file previously opened and
whether the process is allowed to write into it (see the section “File-Handling System
Calls” in Chapter 1). If any of these conditions are not true, the handler must
return a negative value—in this case, the error code –EBADF.
One type of checking, however, is common to all system calls. Whenever a parameter
specifies an address, the kernel must check whether it is inside the process
address space. There are two possible ways to perform this check:
• Verify that the linear address belongs to the process address space and, if so, that
the memory region including it has the proper access rights.
• Verify just that the linear address is lower than PAGE_OFFSET (i.e., that it doesn’t
fall within the range of interval addresses reserved to the kernel).
*/

//Accessing the Process Address Space
/*
System call service routines often need to read or write data contained in the process’s
address space. Linux includes a set of macros that make this access easier.
We’ll describe two of them, called get_user( ) and put_user( ). The first can be used
to read 1, 2, or 4 consecutive bytes from an address, while the second can be used to
write data of those sizes into an address.
Each function accepts two arguments, a value x to transfer and a variable ptr. The second
variable also determines how many bytes to transfer. Thus, in get_user(x,ptr),
the size of the variable pointed to by ptr causes the function to expand into a __get_
user_1( ), __get_user_2( ), or __get_user_4( ) assembly language function.
*/

//Table 10-1. Functions and macros that access the process address space
/*
Function Action
get_user __get_user Reads an integer value from user space (1, 2, or 4 bytes)
put_user __put_user Writes an integer value to user space (1, 2, or 4 bytes)
copy_from_user __copy_from_user Copies a block of arbitrary size from user space
copy_to_user __copy_to_user Copies a block of arbitrary size to user space
strncpy_from_user __strncpy_from_user Copies a null-terminated string from user space
strlen_user strnlen_user Returns the length of a null-terminated string in user space
clear_user __clear_user Fills a memory area in user space with zeros
*/

//Dynamic Address Checking: The Fix-up Code


#endif // end of Parameter Passing
/* -------------------------------------------------------------- */


#define  Exception Tables
#ifdef  Exception Tables
/*
The key to determining the source of a Page Fault lies in the narrow range of calls
that the kernel uses to access the process address space.
if the exception is caused by an invalid parameter, the instruction
that caused it must be included in one of the functions or else be generated by
expanding one of the macros. The number of the instructions that address user space
is fairly small.

Therefore, it does not take much effort to put the address of each kernel instruction
that accesses the process address space into a structure called the exception table. If
we succeed in doing this, the rest is easy. When a Page Fault exception occurs in Kernel
Mode, the do_page_fault( ) handler examines the exception table: if it includes
the address of the instruction that triggered the exception, the error is caused by a
bad system call parameter; otherwise, it is caused by a more serious bug.

The main exception table is automatically
generated by the C compiler when building the kernel program image. It is stored
in the __ex_table section of the kernel code segment, and its starting and ending
addresses are identified by two symbols produced by the C compiler: __start___
ex_table and __stop___ex_table.

Moreover, each dynamically loaded module of the kernel  includes
its own local exception table. This table is automatically generated by the C compiler
when building the module image, and it is loaded into memory when the module
is inserted in the running kernel.
Each entry of an exception table is an exception_table_entry structure that has two
fields:
insn
The linear address of an instruction that accesses the process address space
fixup
The address of the assembly language code to be invoked when a Page Fault
exception triggered by the instruction located at insn occurs
*/


//Generating the Exception Tables and the Fixup Code


/*
The GNU Assembler .section directive allows programmers to specify which section
of the executable file contains the code that follows. As we will see in
Chapter 20, an executable file includes a code segment, which in turn may be subdivided
into sections. Thus, the following assembly language instructions add an entry
into an exception table; the "a" attribute specifies that the section must be loaded
into memory together with the rest of the kernel image:
.section _ _ex_table, "a"
.long faulty_instruction_address, fixup_code_address
.previous
The .previous directive forces the assembler to insert the code that follows into the
section that was active when the last .section directive was encountered.
*/

#endif // end of Exception Tables
/* ------------------------------------------------------------------------- */

#define Kernel Wrapper Routines
#ifdef Kernel Wrapper Routines
/*
Although system calls are used mainly by User Mode processes, they can also be
invoked by kernel threads, which cannot use library functions. To simplify the declarations
of the corresponding wrapper routines, Linux defines a set of seven macros
called _syscall0 through _syscall6.
In the name of each macro, the numbers 0 through 6 correspond to the number of
parameters used by the system call (excluding the system call number). The macros
are used to declare wrapper routines that are not already included in the libc standard
library (for instance, because the Linux system call is not yet supported by the
library); however, they cannot be used to define wrapper routines for system calls
that have more than six parameters (excluding the system call number) or for system
calls that yield nonstandard return values.
Each macro requires exactly 2 + 2 × n parameters, with n being the number of
parameters of the system call. The first two parameters specify the return type and
the name of the system call; each additional pair of parameters specifies the type and
the name of the corresponding system call parameter. Thus, for instance, the wrapper
routine of the fork( ) system call may be generated by:
_syscall0(int,fork)
while the wrapper routine of the write( ) system call may be generated by:
_syscall3(int,write,int,fd,const char *,buf,unsigned int,count)
In the latter case, the macro yields the following code:
int write(int fd,const char * buf,unsigned int count)
{
long __res;
asm("int $0x80"
: "=a" (__res)
: "0" (__NR_write), "b" ((long)fd),
"c" ((long)buf), "d" ((long)count));
if ((unsigned long)__res >= (unsigned long)-129) {
errno = -__res;
__res = -1;
}
return (int) __res;
}
The __NR_write macro is derived from the second parameter of _syscall3; it expands
into the system call number of write( ). When compiling the preceding function, the
following assembly language code is produced:

write:
pushl %ebx ; push ebx into stack
movl 8(%esp), %ebx ; put first parameter in ebx
movl 12(%esp), %ecx ; put second parameter in ecx
movl 16(%esp), %edx ; put third parameter in edx
movl $4, %eax ; put __NR_write in eax
int $0x80 ; invoke system call
cmpl $-125, %eax ; check return code
jbe .L1 ; if no error, jump
negl %eax ; complement the value of eax
movl %eax, errno ; put result in errno
movl $-1, %eax ; set eax to -1
.L1: popl %ebx ; pop ebx from stack
ret ; return to calling program
Notice how the parameters of the write( ) function are loaded into the CPU registers
before the int $0x80 instruction is executed. The value returned in eax must be
interpreted as an error code if it lies between –1 and –129 (the kernel assumes that
the largest error code defined in include/generic/errno.h is 129). If this is the case, the
wrapper routine stores the value of –eax in errno and returns the value –1; otherwise,
it returns the value of eax.
*/


#endif // end of Kernel Wrapper Routines

sys_read( );



SYS_XYZ_call{ // system call handler


    sys_xyz(); // system call service reoutine

    SYSEXIT;
}
#endif // end of __SYSTEMINTERFACE__H
