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

Clean, categorized list of all the standard library header files in C, as defined by the ISO C17 / C23 standard (a.k.a. Standard C Library or “libc”) — not system‑specific POSIX/GNU headers.  

These are the “core” portable headers every C implementation provides.

📚 1. Input / Output (stdio)

| Header | Purpose | Key Features |
|--------|----------|---------------|
| <stdio.h> | Standard input/output | printf, scanf, fopen, fread, fwrite, FILE, stderr, stdin, stdout |

🧮 2. General Utilities

| Header | Purpose | Key Functions / Macros |
|--------|----------|------------------------|
| <stdlib.h> | General utilities | malloc, free, atoi, exit, rand, qsort, abort |
| <stddef.h> | Common definitions | sizet, ptrdifft, NULL, offsetof |
| <stdbool.h> | Boolean type | Defines bool, true, false |
| <stdint.h> | Fixed-width integer types | uint8t, int64t, UINT32MAX |
| <inttypes.h> | Integer format macros | PRIu64, strtoimax |
| <limits.h> | Implementation constants | INTMAX, CHARBIT, etc. |
| <float.h> | Floating‑point limits | FLTMAX, DBLEPSILON |
| <assert.h> | Runtime diagnostics | assert() macro |
| <errno.h> | Error codes | errno, EIO, ENOMEM, etc. |
| <ctype.h> | Character classification | isalpha, isdigit, toupper, etc. |
| <string.h> | String and memory manipulation | strlen, strcpy, memcpy, strcmp |
| <wchar.h> | Wide characters | wprintf, wcslen, mbstowcs |
| <wctype.h> | Wide character classification | iswalpha, iswspace |
| <locale.h> | Locale / internationalization | setlocale, LCALL |

🧠 3. Mathematics

| Header | Purpose | Key Features |
|--------|----------|--------------|
| <math.h> | Math functions | sin, cos, sqrt, pow, fabs |
| <complex.h> | Complex numbers | cexp, cabs, I |
| <fenv.h> | Floating‑point environment | rounding mode, exceptions |
| <tgmath.h> | Type‑generic math macros | Automatically picks correct type overload (float, double, long double) |

⚙️ 4. Time and Date

| Header | Purpose | Key Elements |
|--------|----------|--------------|
| <time.h> | Date/time functions | clock(), time(), strftime, struct tm |

🧵 5. Multithreading (C11 and later)

| Header | Purpose | Key Features |
|--------|----------|--------------|
| <threads.h> | C11 threads | thrdcreate, mtxlock, cndwait |
| <stdatomic.h> | Atomic operations | atomicint, atomicload, atomicstore |

(These are optional on older compilers but required in C11/C23 conforming ones.)

🧰 6. Nonlocal Jumps and Signals

| Header | Purpose | Key Features |
|--------|----------|--------------|
| <setjmp.h> | Nonlocal jumps | setjmp, longjmp |
| <signal.h> | Signal handling | raise, signal, SIGINT, SIGTERM |

🧾 7. Variable Arguments and Utilities

| Header | Purpose | Example |
|--------|----------|---------|
| <stdarg.h> | Variable argument macros | vastart, vaarg, vaend |
| <stdnoreturn.h> | Functions that never return | noreturn keyword for C11+ |

🧑‍💻 8. C11 / C23 Enhancements and Type Utilities

| Header | Purpose | Key Items |
|--------|----------|-----------|
| <stdalign.h> | Alignment control | Alignas, Alignof keywords |
| <uchar.h> | UTF‑8/16/32 char types | char16t, char32t, conversions |
| <stdckdint.h> (C23) | Checked integer arithmetic | ckdadd, ckdsub |

⚙️ 9. Optional / Legacy or Implementation‑Defined

| Header | Note |
|--------|------|
| <iso646.h> | Alternate operator spellings (and, or, etc.) |
| <stdbit.h> (C23) | Bit operations (stdcfirstleadingone) |
| <stdcompat.h> (GNU extension) | Cross‑standard helpers (not ISO) |

🧾 Summary Table

| Category | Headers |
|-----------|----------|
| Core I/O & Utility | stdio.h, stdlib.h, string.h, errno.h, assert.h |
| Numbers & Math | math.h, complex.h, fenv.h, float.h, tgmath.h, limits.h |
| Types & Meta | stdbool.h, stddef.h, stdint.h, inttypes.h |
| Characters & Locales | ctype.h, wchar.h, wctype.h, locale.h, uchar.h |
| Time & Date | time.h |
| Program Control | signal.h, setjmp.h, stdarg.h, stdnoreturn.h |
| Modern C11 Threads | threads.h, stdatomic.h, stdalign.h |

