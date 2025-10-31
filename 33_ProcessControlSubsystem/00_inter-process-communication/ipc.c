/*
//lnterprocess communication mechanisms allow arbitrary processes to exchange data
and synchronize execution.
//We have already considered several forms of
interprocess communication, such as pipes, named pipes, and signals. Pipes
(unnamed) suffer from th,e drawback that they are known only to processes which
are descendants of the process that invoked the pipe system call: Unrelated
processes cannot communicate via pipes. Although named pipes allow unrelated
processes to communicate, they cannot generally be used across a network', 
nor do they readily lend themselves to setting up multiple
communications . paths for different sets of communicating processes: it · is
impossible to multiplex a named pipe to provide private channels for pairs of
communicating processes. Arbitrary processes can also communicate by sending
signals via the kill system call, but the "message" consists only of the · signal
number.
*/
/*
ProcesTRacing
//system provides a primitive form of interprocess communication for
tracing processes, useful for debugging. A debugger process, such as sdb , spawns a
process to be traced and controls its execution with the ptrace system call, 1>etting
and clearing break points, and reading and writing data in its virtual address space.
Process tracing thus consists of synchronization of the debugger process and the
traced process and controlling th􁓺 execution of the traced process.
//typical structure of a debugger
program. The debugger spawns a child process, which invokes the ptrace system
call and, as a result, the kernel sets a trace bit in the child process table entry. The
child now execs the program being traced. For example, if a user is debugging the
program a. out , the child would exec a.out . The kernel executes the exec call as
usual, but at the- end notes that the trace- bit is set and sends the child a "trap"
signal. The kernel checks for signals when returning from the exec system call, just
as it checks for signals after any system call, finds the "trap" signal it had just sent
itself, and executes code for process tracing as a special case for handling signals.
Noting that the trace bit is. set in its process table entry, the child awakens the
parent from its sleep in the wait system call (as will be seen) , enters a special trace
state similar to the sleep state , and does a CQntext switch.

• The kernel must do four context switches to transfer a word of data between a
debugger and a traced process: The kernel switches context in the debugger in
the ptrace call until the traced process replies to a query, switches context to
and from the traced process, and switches context back to the debugger process
with the answer to the ptrace call. The overhead is necessary, because a
debugger has no other way to gain access to the
• A debugger process can trace several child processes simultaneously, although
this feature is rarely . used in practice. More critically, a debugger can only
trace child processes: If a traced child forks, the debugger has no control over
the grandchild, a severe handicap when debugging sophisticated programs. If a
traced process execs, the later execed images are still being traced because of
the original ptrace , but the debugger may not know the name of the execed
image, making symbolic debugging difficult.
• A debugger cannot trace a process that is already executing if · the debugged
process had not called ptrace to let the kernel know that it consents to be
traced. This is inconvenient, because a process that needs debugging .must be
killed and restarted in trace mode.
• It is impossible to trace setuid programs, because users could violate security by
writing their address space via ptrace and doing illegal operations. For
example, suppose a setuid program calls exec with file name "privatefile". A
clever user could use ptrace to overwrite the file name with "/bin/sh", executing
the shell (and all programs executed by the shell) with unauthorized permission.
Exec ignores the setuid bit if the process is traced to prevent a user from
overwriting the address space of a setuid program.

*/
/*
//System V IPC package consists of three mechanisms. Messages allow
processes to send formatted data streams to arbitrary processes, shared memory
allows processes to share parts of their virtual address space, and semaphores allow
processes . to synchronize execution.

*/
/*
//There are four system calls for messages: msgget returns (and possibly creates) a
message descriptor that designates a message queue for use in other system calls,
msgctl has options to set and return parameters associated with a message
descriptor and an option to remove descriptors, msgsnd sends a message, and
msgrcv receives a message.

//msgqid = msgget (key, flag) ;
//where msgqid is the descriptor returned by the call, and key and flag have the
semantics described above for the general "get" calls. The kernel stores messages
on a linked list (queue) per descriptor, and it uses msgqid as an index into an array
of message queue headers. In addition to the general IPC permissions field
mentioned above, the queue structure contains the following fields:
• Pointers to the first and last messages on a linked list;
• The number of messages and the total number of data bytes on the linked list;
• The maximum number of bytes of data that can be on the linked list;
• The ,process IDs of the last processes to send and receive messages;
• Time stamps of the last msgsnd , "JSgrcv , and msgctl operations.
When a user calls msgget to create a new descriptor, the kernel searches the array
of message queues \O see if one exists with the given key. If there is no entry for
the specified key, the kernel allocates a new queue structure, initializes it, and
returns an identifier to the user. Otherwise, it checks permissions and returns.
1.Algorithm msgget

2. Algorithm msgctl



//msgsnd (msgqid, msg, count, flag) ;
//where msgqid is the descriptor of a message queue typically returned by a msgget
call, msg is a pointer to a structure consisting of a user􁫥chosen integer type and a
character array, count gives the size of the data array, and flag specifies the action

the kernel should take if it runs out of internal buffer space.
3. Algorithm msgsnd       // send a message 
input: message queue descriptor, address of message structure, size of message, flags
output: number of bytes sent
{
	check legality of descriptor, permissions;
	while (not enough space to store message)
	{
		if (flags specify not to wait)
			return;
		sleep (until event enough space is available) ;
	}
	get message header;
	read message text from user space to kernel;
	adjust data structures: enqueue message header, message header points to data, counts, time stamps, process ID;
	wakeup all processes waiting to read message from queue;
}


//count = msgrcv (id, msg, maxcount, type, flag) ;
//where id is the message descriptor, msg is the address of a user structure to contain
the received message, maxcount is the size of the data array in msg, type specifies
the message type the user wants to read, and flag specifies what the kernel should
do if no messages are on the queue. The return value, count , is the number of
bytes returned to the user.

4. Algorithm msgrcv
input: message descriptor, address of data array for incoming message, size of data array, requested message type, ftags
output: number of bytes in returned message
{
		check permissions;
	loop:
		check legality of message descriptor;
		// find message to return to user
		if (requested message type == 0)
			consider first message on queue;
		else if (requested message type > 0)
			consider first message on queue with given type;
		else // requested message type < 0 
			consider first of the lowest typed messages on queue,such that its type is < - absolute value of requested type;
		if (there is a message)
		{
			adjust message size or return error if user size too small;
			copy message type, text from kernel space to user space;
			unlink message from queue;
			return;
		}
		//no message 
		if (flags specify not to sleep)
			return with error;
		sleep (event message arrives on queue) ;
		goto loop;
}
*/
/*
//Shared memory
//Processes can communicate directly with each other by sharing parts of their
virtual address space and then reading and writing the data stored in the shared
memory . The system calls for manipulating shared memory are similar to the
system calls for messages. The shmget system call creates a new region of shared
memory or returns an existing one, the shmat system call logically attaches all
region to the virtual address space of a process, the shmdt system call detaches a
region from the virtual address space of a process, and the shmctl system call
manipulates various parameters associated with the shared memory. Processes read
and write shared memory using the same machine instructions they use to read and
write regular memory. After attaching shared memory, it becomes part of the
virtual address space of a process, accessible in the same way other virtual
addresses are; no system calls are needed to access data in shared memory.

//shmid = shmget (key, size, flag) ;
//where size is the number of bytes in the region. The kernel searches the shared
memory table for the given key : if it finds an entry and the permission modes are
acceptable, it returns the descriptor for the entry. If it does not find an entry and
the user had set the IPC_CREA T flag tp create a new region, the kernel verifies
that the size is between system-wide minimum and maximum values and then
allocates a region data structure using algorithm al/ocreg (Section 6.5.2) . The
kernel saves the permission modes, size, and a pointer to the region tabk entry in
the shared memory table (Figure 1 1 .9) and sets a flag there to indicate that no
memo,ry is associated with the region. It allocates memory (page tables and so on)
for the region only when a process attaches the region to its address space. The ·
'kernel also sets a flag on the region table entry to indicate that the region should
not be freed when the last process attached to it exits. Thus, data in shared
memory remains intact even though no processes include it as part of their virtual
address space.

5. Algorithm shmget

6. Algorithm shmat   //attach shared memory
input: shared memory descriptor, virtual address to attach memory, flags
output: virtual address where memory was attached
{
	check validity of descriptor, permissions;
	if (user specified virtual address)
	{
		round off virtual address, as specified by flags;
		check legality of virtual address, size of region;
	}
	else // user wants kernel to find good address
		kernel picks virtual address: error if none available;
	attach region to process address space (algorithm attachreg) ;
	if (region being attached for first time)
		allocate page tables, memory for region(algorithm grow reg) ;
	return (virtual address where attached) ;
}


7. Algorithm shmdt




8. Algorithm shmctl

*/
/*
//Semaphores
//The semaphore system calls allow processes to synchronize execution by doing a set
of operations atomically on a set of semaphores. Before the i mplementation of
semaphores, a process would create a lock- file with the creat system call if it
wanted to lock a resource: The creat fails if the file already exists, and the process
would assume that another process had the resource locked . The major
disadvantages of this approach are that the process does not know when to try
again, and lock files may i nadvertently be left behind when the .;ystem crashes or is rebooted.

//Dijkstra published the Dekker algorithm that describes an implementation of
semaphores , integer-valued objects that have two atomic operations defined for
them: P and V (see [Dijkstra 68]) . The P operation decrements the value of a
semaphore if its value is greater than Q, and the V operation increments its value.
Because the operations are atomic, at most one P or V operation can succeed on a
semaphore at any time. The semaphore system calls in System V are a
generalization of Dijkstra's P and V operations, in that several operations can be .
done simultaneously and the increment and decrement operations can be by values
greater than 1 . The kernel does all the operations atomically; no other processes
adjust the semaphore values until all operations are done. If the kernel cannot do
all the operations, it does not · do any ; the process· sleeps until it can do all the
operations, as will be explained.
A semaphore in UNIX System .V consists of the following elements:
• The value of the semaphore,
• The process ID of the last process to manipulate the semaphore,
" The number of processes waiting for the semaphore value to increase,
• The number of processes waiting for the semaphore value to equal 0.

//The semaphore system calls are semget to create and gain access to a set of
semaphores, semctl to do various control operations on the set, and semop to
manipulate the values of semaphores.

//id == semget (key, count, flag) ;
9. Algorithm semget

10. Algorithm semctl

//oldval = semop (id, oplist, count) ;
11. Algorithm semop
algorithm semop // semaphore operations 
inputs: semaphore descriptor, array of semaphore operations, number of elements in array
output: start value of last semaphore operated on
{
	check legality of semaphore descriptor;
	start: read array of semaphore operations from user to kernel space;
	check permissions for all semaphore operations;
	for (each semaphore operation in array)
	{
		if (semaphore operation is positive)
		{
			add "operation" to semaphore value;
			if (UNDO flag set on semaphore operation)
				update process undo structure;
			wakeup all processes sleeping (event semaphore value increases) ;
		}
		else if (semaphore operation is negative )
		{
		if ("operation" + semaphore value > - 0)
		{
			add "operation" to semaphore value;
			if (UNDO flag set)
				update process undo structure;
				if (semaphore value 0)
					wakeup all processes sl􀩁ping (eventsemaphore value becomes O) ;
			continue;
		}
		reverse all semaphore operations already done 
			this system call (previous iterations) ;
		if (flags specify not to sleep)
			return with error;
		sleep (event semaphore value increases) ;
		goto start; // start loop from beginning
		}
		else //semaphore operation is zero 
		{
			if (semaphore value non 0)
			{
				reverse all semaphore operations done
					this system call;
				if (flags specify not to sleep)
					return with error;
				sleep (event semaphore value == 0) ;
				goto start; // restart loop 
			}
		}
	}//for loop ends here 
	//semaphore operations all succeeded 
	update time stamps, process ID's;
	return value of last semaphore operated on before call succeeded;
}
*/

/*
NETWORK COMMUNICATIONS


*/

/*
SOCKETS
Clien Process                  Server Process
-------------                  --------------
     
------------                   --------------
 Socket layer                   Socket Layer
------------                   --------------

------------                   --------------
Protocol Layer: TCP            Protocol Layer: TCP

              : IP                           : IP
------------                   --------------

------------                  ----------------
Eth Driver                    Eth Driver
------------                 ----------------


---------------Network------------------------

//The kernel s􁫀ructure consists of three parts: the socket layer, the protocol layer,
and the device layer (Figure 1 1 . 1 8) . The socket layer provides· the interface
between the system calls and the lower layers, the protocol layer contains the
protocol modules used for communication (TCP and IP in the figure) , and the
device layer contains the device drivers that control the network devices. Legal
combinations of protocols and drivers are specified when configuring the system, a
method that is not as flexible as pushing streams modules. Processes communicate
using the client-server model: a server process listens to a socket , one end point of
a two-way communications path, and client processes communicate to the server
process {JVer another socket, the other end point of the communications path, which
may be on another machine. The kernel maintains internal connections and routes
data from client to server.

//system domain" for processes communicating on one
machine and the "Internet domain" for processes communicating across a network
using the DARPA (Defense Advanced Research Project Agency) communications
protocols (see [Postel 80] and [ Postel 8 1 ]) . Each socket has a type - a virtual
circuit (stream socket in the Berkeley terminology) or datagrqm . A virtual circuit
allows sequenced, reliable delivery of data. Datagrams do not guarantee sequenced,
reliable, or unduplicated delivery, but they are less expensive than virtual circuits,
because they do not require expensive setup operations; henc􁫁. they are useful for
some types of communication. The system contains a default protocol for every
legal domain-socket type combination. For example, the Transport Connect
Protocol (TCP) provides virtual circuit service and the User Datagram Protocol
(UDP) provides datagram service in the Internet domain


//The socket mechanism contains several system calls. The socket system call
establishes the end point of a communications link.
		sd == socket (format, type, protocol) ;
Format specifies the communications domain (the UNIX system domain or the
Internet domain) , type indicates the type of communication over the socket (virtual
circait or datagram) , and protocol indicates a particular protocol to control the
communication. Processes use the socket descriptor sd in other system calls. The
close system call closes sockets.

//The bind system call associates a name with the socket descriptor:
		bind (sd, address, length) ;
Sd is the socket descriptor, and address points to a structure that specifies an
identifier specific to the communications domain and protocol specified in the socket
system call. Length is the length of the address structure; without this parameter,
the kernel would not know how long the address is because it can vary across
domains and protocols. For example, an address in the UNIX system domain is a
file name. Server processes bind addresses to sockets and "advertise" their names
to identify themselves to client processe$.

//The connect system call requests that the kernel make a conQection to an
existing socket:
		connect (sd, address, length) ;
The semantics of the parameters are the same as for bind, but address is the
address of the target socket that will form the other end of the communic􁫃tions
line. Both sockets must use the same communications domain and protocol, and
lhe kernel arranges that the communications links are set up correctly. If tpe type
of the socket is a datagram, the connect call informs the kernel of the address to be
used on subsequent send calls over the socket; no connections are made at the time
of the call.

//When a server process arranges to accept connections over a virtual circuit, the
kernel must queue incoming requests until it can service them. The listen system
call specifies the maximum queue length:
		listen (sd, qlength}
where sd is the socket descriptor and qlength is the maximum number of
outstanding requests.

//The accept call receives incoming requests for a connection to a server process:
	nsd = accept (sd, address, addrlen) ;
where sd is the socket descriptor, address points to a .. user data array that the
kernel fills with the return address of the connecting client, and addrlen indicates
the size of the user array. When accept returns, the kernel overwrites the contents
of addrlen with a number that indicates the amount of space taken up by the
address. Accept returns a new socket descriptor nsd, different from the socket
descriptor sd. A server can continue listening to the advertised socket while
communicating with a client process over a separate communications channel

//The send and recv system calls transmit data over a connected socket:
		count = send (sd, msg, iength, flags) ;
where sd is the socket descriptor, m.!g is a pointer to the data being sent, length is
its length, and count is the number of bytes actually sent. The flags parameter
\fllay be set to the value SOF_OOB to send data "out-of-band," meaning that data
being sent is not considered part of the regular sequence of data exchange between
the communicating processes. A "remote login" program, for 􁓰nstance, may send .
an "out Qf band" message to simulate a user hitting the delete key at a terminal.

//The syntax of the recv system calls is
		count = recv (sd, buf, length, flags) ;
where buf is the data array for incoming data, length is the expected length, and
count is the number of bytes copied to the user program. Flags can be set to
"peek" at an incoming message and examine its contents without removing it from
the queu_e, or to receive "out of band" data. The datagram versions of these system
calls, senqt�'and recvfrom , have additional parameters for addresses. Processes can
use read and l'•rite system calls on stream sockets instead of send and recv after the
connectiop/ls set up. Thus, servers can take care of network-specific protocol
negotiatiOn · and spawn processes that use read and write calls only, as if they are
using regular files.

//The shutdown system call closes a socket connection:
		shutdQwn (sd, mode)
where mode indicates whether the sending side, the receiving side, or both sides no
longer allow data transmission. It informs the underlying protocols to close down
the network communications, but the socket descriptors are still intact. The close
system call frees the socket descriptor.

//The getsockname system call gets the name of a socket bound by a previous
bind call:
		getsockname(sd, name, length) ;
The getsockopt and setsockopt calls retrieve and set various options associated with
the socket, according to the communications domain and protocol of the socket.

*/