/**
 * @file    sysconf_constants.h
 * @brief   POSIX sysconf(3) symbolic constant mapping.
 *
 * Maps POSIX-standard symbolic names to their SC_* codes as passed to
 * sysconf(). Values are system-wide (not per-path) and are fixed for
 * the lifetime of a process.
 *
 * Sections:
 *   1. Numeric resource limits
 *   2. POSIX option/feature flags    (POSIX_* → SC_*)
 *   3. X/Open (XSI) option flags     (XOPEN_* → SC_*)
 *
 * Items marked [DEFERRED] are defined here for completeness but
 * are discussed in the chapters noted; do not rely on them without
 * reading the relevant chapter first.
 *
 * Items marked [REQUIRED] must be supported by any conforming
 * POSIX implementation.
 *
 * Items marked [OBSOLETE] may be absent on modern systems.
 *
 * Reference: POSIX.1-2017 §2.5 / SUSv4
 *
 * @version 1.0.0
 * @date    2026-05-25
 */

 #ifndef SYSCONF_CONSTANTS_H
 #define SYSCONF_CONSTANTS_H
 
 #include <unistd.h>   /* sysconf(), _SC_* */
 
 /* =========================================================================
  * SECTION 1 — Numeric resource limits
  * Query with: sysconf(SC_xxx)
  * ====================================================================== */
 
 /* --- Async I/O (deferred to async-I/O chapter) ------------------------- */
 
 /** [DEFERRED ch.14] Max entries in a listio() call. */
 #define SC_AIO_LISTIO_MAX       _SC_AIO_LISTIO_MAX
 
 /** [DEFERRED ch.14] Max outstanding async I/O operations. */
 #define SC_AIO_MAX              _SC_AIO_MAX
 
 /** [DEFERRED ch.14] Max async I/O priority delta. */
 #define SC_AIO_PRIO_DELTA_MAX   _SC_AIO_PRIO_DELTA_MAX
 
 /* --- Process / argument limits ----------------------------------------- */
 
 /** Maximum total bytes of arguments + environment to exec*(). */
 #define SC_ARG_MAX              _SC_ARG_MAX
 
 /** Maximum number of atexit() handlers. */
 #define SC_ATEXIT_MAX           _SC_ATEXIT_MAX
 
 /** Maximum length of a character class name in a locale. */
 #define SC_CHARCLASS_NAME_MAX   _SC_CHARCLASS_NAME_MAX
 
 /** Maximum number of simultaneous child processes. */
 #define SC_CHILD_MAX            _SC_CHILD_MAX
 
 /** Number of clock ticks (jiffies) per second. See also: sysconf(_SC_CLK_TCK). */
 #define SC_CLK_TCK              _SC_CLK_TCK
 
 /** Maximum number of weights assignable to a collation element. */
 #define SC_COLL_WEIGHTS_MAX     _SC_COLL_WEIGHTS_MAX
 
 /** Maximum number of timer expiration overruns (POSIX timers). */
 #define SC_DELAYTIMER_MAX       _SC_DELAYTIMER_MAX
 
 /** Maximum length of a hostname (gethostname()). */
 #define SC_HOST_NAME_MAX        _SC_HOST_NAME_MAX
 
 /** Maximum number of iovec elements in a single readv()/writev(). */
 #define SC_IOV_MAX              _SC_IOV_MAX
 
 /** Maximum length of a line processed by POSIX text utilities. */
 #define SC_LINE_MAX             _SC_LINE_MAX
 
 /** Maximum length of a login name. */
 #define SC_LOGIN_NAME_MAX       _SC_LOGIN_NAME_MAX
 
 /** Maximum number of supplementary group IDs per process. */
 #define SC_NGROUPS_MAX          _SC_NGROUPS_MAX
 
 /** Initial buffer size suggested for getgrgid_r() / getgrnam_r(). */
 #define SC_GETGR_R_SIZE_MAX     _SC_GETGR_R_SIZE_MAX
 
 /** Initial buffer size suggested for getpwuid_r() / getpwnam_r(). */
 #define SC_GETPW_R_SIZE_MAX     _SC_GETPW_R_SIZE_MAX
 
 /** Maximum number of simultaneously open file descriptors per process. */
 #define SC_OPEN_MAX             _SC_OPEN_MAX
 
 /** System memory page size in bytes. Prefer getpagesize() or sysconf(_SC_PAGESIZE). */
 #define SC_PAGESIZE             _SC_PAGESIZE   /* also _SC_PAGE_SIZE */
 
 /* --- Thread limits (deferred to threads chapter) ----------------------- */
 
 /** [DEFERRED ch.12] Iterations of destructor calls on thread exit. */
 #define SC_THREAD_DESTRUCTOR_ITERATIONS  _SC_THREAD_DESTRUCTOR_ITERATIONS
 
 /** [DEFERRED ch.12] Maximum number of thread-specific data keys. */
 #define SC_THREAD_KEYS_MAX      _SC_THREAD_KEYS_MAX
 
 /** [DEFERRED ch.12] Minimum stack size for a thread (bytes). */
 #define SC_THREAD_STACK_MIN     _SC_THREAD_STACK_MIN
 
 /** [DEFERRED ch.12] Maximum number of simultaneous threads. */
 #define SC_THREAD_THREADS_MAX   _SC_THREAD_THREADS_MAX
 
 /* --- Signal / IPC limits ----------------------------------------------- */
 
 /** Maximum number of file descriptors duplicatable via dup*(). */
 #define SC_REDUP_MAX            _SC_REDUP_MAX   /* non-standard; check availability */
 
 /** Maximum number of real-time signals (SIGRTMIN..SIGRTMAX). */
 #define SC_RTSIG_MAX            _SC_RTSIG_MAX
 
 /** Maximum number of semaphores per semaphore set. */
 #define SC_SEM_NSEMS_MAX        _SC_SEM_NSEMS_MAX
 
 /** Maximum value a semaphore can hold. */
 #define SC_SEM_VALUE_MAX        _SC_SEM_VALUE_MAX
 
 /** Maximum number of queued real-time signals. */
 #define SC_SIGQUEUE_MAX         _SC_SIGQUEUE_MAX
 
 /** Maximum number of POSIX streams open simultaneously. */
 #define SC_STREAM_MAX           _SC_STREAM_MAX
 
 /** Maximum number of symbolic link levels traversed in path resolution. */
 #define SC_SYMLOOP_MAX          _SC_SYMLOOP_MAX
 
 /** Maximum number of POSIX timers per process. */
 #define SC_TIMER_MAX            _SC_TIMER_MAX
 
 /** Maximum length of a terminal device name. */
 #define SC_TTY_NAME_MAX         _SC_TTY_NAME_MAX
 
 /** Maximum length of a timezone name string. */
 #define SC_TZNAME_MAX           _SC_TZNAME_MAX
 
 /* =========================================================================
  * SECTION 2 — POSIX option / feature flags
  * sysconf() returns: -1 (unknown), 0 (not supported), >0 (supported)
  * ====================================================================== */
 
 /** Advisory file locking (fcntl POSIX locks). */
 #define SC_ADVISORY_INFO            _SC_ADVISORY_INFO
 
 /** [REQUIRED] Asynchronous I/O (aio_read, aio_write, etc.). */
 #define SC_ASYNCHRONOUS_IO          _SC_ASYNCHRONOUS_IO
 
 /** [REQUIRED] Thread barriers (pthread_barrier_*). */
 #define SC_BARRIERS                 _SC_BARRIERS
 
 /** [REQUIRED] Clock selection (pthread_condattr_setclock). */
 #define SC_CLOCK_SELECTION          _SC_CLOCK_SELECTION
 
 /** Per-process CPU-time clocks (CLOCK_PROCESS_CPUTIME_ID). */
 #define SC_CPUTIME                  _SC_CPUTIME
 
 /** File synchronization (fsync, fdatasync). */
 #define SC_FSYNC                    _SC_FSYNC
 
 /** IPv6 socket support. */
 #define SC_IPV6                     _SC_IPV6
 
 /** Job control (SIGCHLD, waitpid, process groups). */
 #define SC_JOB_CONTROL              _SC_JOB_CONTROL
 
 /** [REQUIRED] Memory-mapped files (mmap). */
 #define SC_MAPPED_FILES             _SC_MAPPED_FILES
 
 /** Locking entire process address space in RAM (mlockall). */
 #define SC_MEMLOCK                  _SC_MEMLOCK
 
 /** Locking arbitrary address ranges in RAM (mlock). */
 #define SC_MEMLOCK_RANGE            _SC_MEMLOCK_RANGE
 
 /** [REQUIRED] Memory protection (mprotect). */
 #define SC_MEMORY_PROTECTION        _SC_MEMORY_PROTECTION
 
 /** POSIX message queues (mq_open, mq_send, etc.). */
 #define SC_MESSAGE_PASSING          _SC_MESSAGE_PASSING
 
 /** Monotonic clock (CLOCK_MONOTONIC). */
 #define SC_MONOTONIC_CLOCK          _SC_MONOTONIC_CLOCK
 
 /** Prioritized I/O (lio_listio with LIO_NOP priority). */
 #define SC_PRIORITIZED_IO           _SC_PRIORITIZED_IO
 
 /** Priority scheduling (sched_setscheduler, SCHED_FIFO, SCHED_RR). */
 #define SC_PRIORITY_SCHEDULING      _SC_PRIORITY_SCHEDULING
 
 /** Raw socket support (SOCK_RAW). */
 #define SC_RAW_SOCKETS              _SC_RAW_SOCKETS
 
 /** [REQUIRED] Read-write locks (pthread_rwlock_*). */
 #define SC_READER_WRITER_LOCKS      _SC_READER_WRITER_LOCKS
 
 /** [REQUIRED] Real-time signals (SIGRTMIN..SIGRTMAX, sigqueue). */
 #define SC_REALTIME_SIGNALS         _SC_REALTIME_SIGNALS
 
 /** Regular expression support (regcomp, regexec). */
 #define SC_REGEXP                   _SC_REGEXP
 
 /** Saved set-user-ID and set-group-ID (seteuid/setegid semantics). */
 #define SC_SAVED_IDS                _SC_SAVED_IDS
 
 /** [REQUIRED] POSIX semaphores (sem_open, sem_wait, etc.). */
 #define SC_SEMAPHORES               _SC_SEMAPHORES
 
 /** POSIX shared memory objects (shm_open, shm_unlink). */
 #define SC_SHARED_MEMORY_OBJECTS    _SC_SHARED_MEMORY_OBJECTS
 
 /** Invoking a POSIX shell via system() / popen(). */
 #define SC_SHELL                    _SC_SHELL
 
 /** Process spawning (posix_spawn). */
 #define SC_SPAWN                    _SC_SPAWN
 
 /** [REQUIRED] Spin locks (pthread_spin_*). */
 #define SC_SPIN_LOCKS               _SC_SPIN_LOCKS
 
 /** Sporadic server scheduling (SCHED_SPORADIC). */
 #define SC_SPORADIC_SERVER          _SC_SPORADIC_SERVER
 
 /** Synchronized I/O (O_SYNC, O_DSYNC, fdatasync). */
 #define SC_SYNCHRONIZED_IO          _SC_SYNCHRONIZED_IO
 
 /* --- Thread options (deferred to threads chapter) ---------------------- */
 
 /** [DEFERRED ch.12] Thread stack address attribute. */
 #define SC_THREAD_ATTR_STACKADDR    _SC_THREAD_ATTR_STACKADDR
 
 /** [DEFERRED ch.12] Thread stack size attribute. */
 #define SC_THREAD_ATTR_STACKSIZE    _SC_THREAD_ATTR_STACKSIZE
 
 /** [DEFERRED ch.12] Per-thread CPU-time clocks. */
 #define SC_THREAD_CPUTIME           _SC_THREAD_CPUTIME
 
 /** [DEFERRED ch.12] Priority inheritance mutexes. */
 #define SC_THREAD_PRIO_INHERIT      _SC_THREAD_PRIO_INHERIT
 
 /** [DEFERRED ch.12] Priority scheduling for threads. */
 #define SC_THREAD_PRIORITY_SCHEDULING _SC_THREAD_PRIORITY_SCHEDULING
 
 /** [DEFERRED ch.12] Process-shared synchronization objects. */
 #define SC_THREAD_PROCESS_SHARED    _SC_THREAD_PROCESS_SHARED
 
 /** [DEFERRED ch.12] Robust priority-inheritance mutexes. */
 #define SC_THREAD_ROBUST_PRIO_INHERIT _SC_THREAD_ROBUST_PRIO_INHERIT
 
 /** [DEFERRED ch.12] Robust priority-protect mutexes. */
 #define SC_THREAD_ROBUST_PRIO_PROTECT _SC_THREAD_ROBUST_PRIO_PROTECT
 
 /** [DEFERRED ch.12] [REQUIRED] Thread-safe standard functions (_r variants). */
 #define SC_THREAD_SAFE_FUNCTIONS    _SC_THREAD_SAFE_FUNCTIONS
 
 /** [DEFERRED ch.12] Sporadic server scheduling for threads. */
 #define SC_THREAD_SPORADIC_SERVER   _SC_THREAD_SPORADIC_SERVER
 
 /** [DEFERRED ch.12] [REQUIRED] POSIX threads (pthread_create, etc.). */
 #define SC_THREADS                  _SC_THREADS
 
 /** [REQUIRED] Timeout variants of blocking functions (e.g. sem_timedwait). */
 #define SC_TIMEOUTS                 _SC_TIMEOUTS
 
 /** [REQUIRED] POSIX timers (timer_create, timer_settime). */
 #define SC_TIMERS                   _SC_TIMERS
 
 /** Typed memory objects (posix_typed_mem_open). */
 #define SC_TYPED_MEMORY_OBJECTS     _SC_TYPED_MEMORY_OBJECTS
 
 /** POSIX version (e.g. 200809L for POSIX.1-2008). */
 #define SC_VERSION                  _SC_VERSION
 
 /* --- Data model / word-size options (deferred to data-model chapter) --- */
 
 /** [DEFERRED ch.3] ILP32 with 32-bit off_t compilation environment. */
 #define SC_V7_ILP32_OFF32           _SC_V7_ILP32_OFF32
 
 /** [DEFERRED ch.3] ILP32 with 64-bit off_t compilation environment. */
 #define SC_V7_ILP32_OFFBIG          _SC_V7_ILP32_OFFBIG
 
 /** [DEFERRED ch.3] LP64 with 64-bit off_t compilation environment. */
 #define SC_V7_LP64_OFF64            _SC_V7_LP64_OFF64
 
 /** [DEFERRED ch.3] LP64 with large off_t compilation environment. */
 #define SC_V7_LPBIG_OFFBIG          _SC_V7_LPBIG_OFFBIG
 
 /* =========================================================================
  * SECTION 3 — X/Open (XSI) option flags
  * ====================================================================== */
 
 /** X/Open cryptography (DES, etc.). */
 #define SC_XOPEN_CRYPT              _SC_XOPEN_CRYPT
 
 /** X/Open enhanced internationalization (wide characters, locales). */
 #define SC_XOPEN_ENH_I18N           _SC_XOPEN_ENH_I18N
 
 /** X/Open real-time extensions. */
 #define SC_XOPEN_REALTIME           _SC_XOPEN_REALTIME
 
 /** X/Open real-time thread extensions. */
 #define SC_XOPEN_REALTIME_THREADS   _SC_XOPEN_REALTIME_THREADS
 
 /** X/Open shared memory (XSI IPC shm*). */
 #define SC_XOPEN_SHM                _SC_XOPEN_SHM
 
 /** [OBSOLETE] X/Open STREAMS (XSI STREAMS option — removed in SUSv4). */
 #define SC_XOPEN_STREAMS            _SC_XOPEN_STREAMS   /* obsolete */
 
 /** X/Open UNIX extensions (XSI). */
 #define SC_XOPEN_UNIX               _SC_XOPEN_UNIX
 
 /** X/Open UUCP utilities. */
 #define SC_XOPEN_UUCP               _SC_XOPEN_UUCP
 
 /** X/Open specification version (e.g. 700 for SUSv4). */
 #define SC_XOPEN_VERSION            _SC_XOPEN_VERSION
 
 /* =========================================================================
  * Convenience query helper
  * ====================================================================== */
 
 #include <errno.h>
 #include <stdio.h>
 
 /**
  * @brief Query a sysconf value and print a human-readable result.
  *
  * @param name   SC_* constant (e.g. SC_OPEN_MAX).
  * @param label  Human-readable label for the constant.
  */
 static inline void sysconf_print(int name, const char *label)
 {
     errno = 0;
     long val = sysconf(name);
     if (val == -1) {
         if (errno)
             fprintf(stderr, "  %-40s  error: %m\n", label);
         else
             printf("  %-40s  indeterminate (no limit)\n", label);
     } else {
         printf("  %-40s  %ld\n", label, val);
     }
 }
 
 #endif /* SYSCONF_CONSTANTS_H */
 