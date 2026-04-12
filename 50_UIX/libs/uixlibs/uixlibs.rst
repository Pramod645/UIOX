#define __POXIS__H
#ifdef __POXIS__H


List of the common POSIX standard header files (from the IEEE POSIX.1 specifications). These are headers you'll find on UNIX, Linux, and macOS systems and are part of the Portable Operating System Interface (POSIX) standard.

🧩 Core POSIX Headers

| Header | Description |
|--------|--------------|
| <unistd.h> | Standard symbolic constants and types; defines POSIX API functions (e.g. read, write, fork, exec, sleep, etc.) |
| <fcntl.h> | File control options (e.g. open, fcntl, file locking) |
| <sys/types.h> | Fundamental system data types (e.g. pidt, sizet, ssizet, etc.) |
| <sys/stat.h> | File information and permissions (used with stat, chmod, etc.) |
| <sys/wait.h> | Declarations for process waiting (wait, waitpid) |
| <sys/time.h> | Time and timer functions (gettimeofday, settimeofday, etc.) |
| <sys/times.h> | Process times (times() function) |
| <sys/resource.h> | Resource limits and priorities |
| <sys/select.h> | select() for monitoring multiple file descriptors |
| <sys/socket.h> | Socket interface definitions |
| <sys/un.h> | Definitions for UNIX domain sockets |
| <sys/mman.h> | Memory management (mmap, munmap) |
| <sys/utsname.h> | System information (uname()) |
| <sys/ipc.h> | Inter-process communication mechanisms |
| <sys/msg.h> | Message queue definitions |
| <sys/sem.h> | Semaphore definitions |
| <sys/shm.h> | Shared memory definitions |
| <syslog.h> | System logging functions |

🧵 Threads and Synchronization

| Header | Description |
|--------|--------------|
| <pthread.h> | POSIX threads (thread creation, mutexes, condition vars, etc.) |
| <semaphore.h> | POSIX semaphores |
| <sched.h> | Process scheduling |
| <signal.h> | Signal handling (sigaction, kill, etc.) |
| <mqueue.h> | POSIX message queues |

🔒 File & Directory Management

| Header | Description |
|--------|--------------|
| <dirent.h> | Directory entries (opendir, readdir, closedir) |
| <utime.h> | Setting file access/modification times |
| <ftw.h> | File tree walk utility (nftw, ftw) |
| <glob.h> | Filename pattern matching |
| <fnmatch.h> | Filename matching functions |
| <pwd.h> | User account information |
| <grp.h> | Group information |
| <errno.h> | Error codes (errno, EINTR, etc.) |

⏱ Time and Date

| Header | Description |
|--------|--------------|
| <time.h> | Time functions (C standard + POSIX extensions) |
| <sys/timeb.h> | High-resolution time (non-standard, but POSIX-compatible) |

🌐 Networking and Environment

| Header | Description |
|--------|--------------|
| <arpa/inet.h> | Internet operations (inetpton, inet_ntoa, etc.) |
| <netinet/in.h> | Internet address family definitions |
| <netdb.h> | Network database operations |
| <ifaddrs.h> | Network interface addresses |
| <net/if.h> | Network interface configuration |

🧮 Miscellaneous / Utilities

| Header | Description |
|--------|--------------|
| <termios.h> | Terminal control |
| <ctype.h> | Character classification (C standard + POSIX) |
| <string.h> | String manipulation (standard) |
| <stdlib.h> | General utilities (memory management, process control) |
| <stdio.h> | Standard I/O functions |
| <limits.h> | Implementation-defined limits |
| <locale.h> | Localization support |
| <math.h> | Mathematical functions |
| <unistd.h> | (repeated intentionally) — the most central POSIX header |


#endif // end of __POXIS__H
