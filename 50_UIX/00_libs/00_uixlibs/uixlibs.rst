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


|********************End of List of the POXIS required files***************************|
|********************End of List of the POXIS required files***************************|
|********************End of List of the POXIS required files***************************|
|********************End of List of the POXIS required files***************************|
|********************End of List of the POXIS required files***************************|
|********************End of List of the POXIS required files***************************|




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
|time.h> | Date/time functions | clock(), time(), strftime, struct tm | <

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


🧾 END



|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|
|********************End of List of the Standderd Lib required files***************************|



Types of files

📚 1. Group 1 — Core Type & Feature Definitions
    FIles:uix_types.h, uix_features.h, uix_stddef.h, uix_stdint.h, uix_uix_stdarg.hlimits.h, uix_float.h
          uix_stdbool.h, uix_stdalign.h, uix_stdnoreturn.h, uix_stdarg.h, uix_stdatomic.h, uix_stdbit.h
          uix_stdckdint.h, uix_stdcompat.h, uix_inttypes.h, uix_iso646.h, uix_assert.h, uix_wchar.h, 
          uix_wctype.h, uix_uchar.h

📚 2. Group 2 — String, Character, and Math
    Files: uix_string.h / uix_string.c, uix_ctype.h / uix_ctype.c, uix_math.h / uix_math.c, 

📚 3. Group 3 — Memory, Standard Library, I/O
    Files: uix_stdlib.h / uix_stdlib.c, uix_stdio.h / uix_stdio.c

📚 4. Group 4 — Time
    Files: uix_time.h / uix_time.c, uix_timeb.h / uix_timeb.c, uix_times.h / uix_times.c, uix_utime.h / uix_utime.c

📚 5. Group 5 — Process, Signal, Threading
    Files: uix_signal.h / uix_signal.c, uix_setjmp.h / uix_setjmp.c, uix_pthread.h / uix_pthread.c, 
            uix_semaphore.h / uix_semaphore.c, uix_threads.h / uix_threads.c (C11 threads), uix_sched.h / uix_sched.c

📚 6. Group 6 — File System
    Files: uix_fcntl.h / uix_fcntl.c, uix_stat.h / uix_stat.c, uix_dirent.h / uix_dirent.c, 
            uix_unistd.h / uix_unistd.c (listed as uix_uinstd), uix_mman.h / uix_mman.c, uix_ioctl.h / uix_ioctl.c
            uix_termios.h / uix_termios.c, uix_select.h / uix_select.c, uix_poll.h / uix_poll.c, uix_wait.h / uix_wait.c, 
            uix_resource.h / uix_resource.c

📚 7. Group 7 — IPC
    Files: uix_ipc.h / uix_ipc.c, uix_msg.h / uix_msg.c, uix_sem.h / uix_sem.c, uix_shm.h / uix_shm.c

📚 8. Group 8 — Network
    Files: uix_socket.h / uix_socket.c, uix_inet.h / uix_inet.c, uix_in.h / uix_in.c, uix_if.h / uix_if.c, 
            uix_un.h / uix_un.c, uix_netdb.h / uix_netdb.c, uix_ifaddrs.h / uix_ifaddrs.c

📚 9. Group 9 — File Matching and Traversal
    Files: uix_fnmatch.h / uix_fnmatch.c, uix_ftw.h / uix_ftw.c, uix_glob.h / uix_glob.c

📚 10. Group 10 — Locale, I18N, Logging
    Files: uix_locale.h / uix_locale.c, uix_syslog.h / uix_syslog.c, uix_mqueue.h / uix_mqueue.c

📚 Group 11 — Complex, FPU, Type-Generic Math
    Files: uix_complex.h, uix_fenv.h / uix_fenv.c, uix_tgmath.h / uix_tgmath.c, uix_errno.h / uix_errno.c, 
            uix_pwd.h / uix_pwd.c, uix_grp.h / uix_grp.c, uix_utsname.h / uix_utsname.c, uix_regex.h / uix_regex.c,


🧾 END


|*****************************************system call descriptions************************************************|

1.File System Calls:
    a. open, create, dup, pipe, close                                                      : return file descriptor
    b. open, create, chdir, chroot, chown, chmod, stat, link, unlink, mknod, mount, unmount: use of namei
    c. creat, mknod, link, unlink                                                          : assign nodes
    d. chown, chmod, stat                                                                  : file attributes
    e. read, write, lseek                                                                  : File I/O
    f. mount, unmount                                                                      : File sys structure
    g. chdir, chown                                                                        : Manupulation
2.Process system Calls:
    a. fork
    b. exec
    c. brk
    d. exit
    e. wait
    f. signal
    g. kill
    h. setpgrp
    i. setuid
