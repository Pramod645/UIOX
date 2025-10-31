/*
//lnterprocess communication mechanisms allow arbitrary processes to exchange aata
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