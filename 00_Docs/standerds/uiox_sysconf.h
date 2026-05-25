/**
 * @file    uiox_sysconf.h
 * @brief   POSIX sysconf(3) symbolic constants — system-wide limits & options
 *
 * Maps POSIX symbolic names to their _SC_* query tokens used with
 * sysconf() to retrieve system-wide runtime configuration values and
 * optional-feature availability flags at runtime.
 *
 * Feature flags return:
 *   > 0   — feature is supported
 *     0   — feature is not supported
 *    -1   — support is indeterminate (check at runtime)
 *
 * Conforms to: POSIX.1-2017 (IEEE Std 1003.1-2017 / SUSv4)
 * Target platforms: ARM64, ARM32, x86_64 (0x8064)
 *
 * Usage:
 *   long nproc = sysconf(_SC_NPROCESSORS_ONLN);
 *
 * Thread-related constants (_SC_THREAD_*) are deferred to chapter 12
 * (threading) per project convention and marked accordingly.
 * AIO constants (_SC_AIO_*) are deferred to chapter 14.
 *
 * @author  UIOX Project
 * @date    2026-05-25
 */

 #ifndef UIOX_SYSCONF_H
 #define UIOX_SYSCONF_H
 
 #include <unistd.h>   /* sysconf(), _SC_* tokens */
 
 /* ================================================================== */
 /*  SECTION 1 — Runtime Limit Constants                               */
 /* ================================================================== */
 
 /**
  * @defgroup sysconf_limits  System-wide runtime limit constants
  * @{
  */
 
 /* -- Async I/O (deferred to chapter 14) ---------------------------- */
 
 /** Max number of outstanding async I/O operations (AIO).
  *  Deferred: see chapter 14.  |  Query: _SC_AIO_LISTIO_MAX */
 #define UIOX_SC_AIOLISTIOMAX            _SC_AIO_LISTIO_MAX
 
 /** Max number of simultaneous async I/O operations per process.
  *  Deferred: see chapter 14.  |  Query: _SC_AIO_MAX */
 #define UIOX_SC_AIOMAX                  _SC_AIO_MAX
 
 /** Max amount by which an AIO priority can be decreased.
  *  Deferred: see chapter 14.  |  Query: _SC_AIO_PRIO_DELTA_MAX */
 #define UIOX_SC_AIOPRIODELTAMAX         _SC_AIO_PRIO_DELTA_MAX
 
 /* -- Process & argument limits ------------------------------------- */
 
 /** Maximum total length of exec() argument list + environment, bytes.
  *  POSIX name: ARG_MAX        |  Query: _SC_ARG_MAX */
 #define UIOX_SC_ARGMAX                  _SC_ARG_MAX
 
 /** Maximum number of functions registerable via atexit().
  *  POSIX name: ATEXIT_MAX     |  Query: _SC_ATEXIT_MAX */
 #define UIOX_SC_ATEXITMAX               _SC_ATEXIT_MAX
 
 /** Maximum number of characters in a character class name (locale).
  *  POSIX name: CHARCLASS_NAME_MAX  |  Query: _SC_CHARCLASS_NAME_MAX */
 #define UIOX_SC_CHARCLASSNAMEMAX        _SC_CHARCLASS_NAME_MAX
 
 /** Maximum number of simultaneous child processes per real user ID.
  *  POSIX name: CHILD_MAX      |  Query: _SC_CHILD_MAX */
 #define UIOX_SC_CHILDMAX                _SC_CHILD_MAX
 
 /** Number of clock ticks (jiffies) per second.
  *  POSIX name: CLK_TCK        |  Query: _SC_CLK_TCK */
 #define UIOX_SC_CLOCKTICKSPERSECOND     _SC_CLK_TCK
 
 /** Maximum number of weights assignable to a locale collating element.
  *  POSIX name: COLL_WEIGHTS_MAX  |  Query: _SC_COLL_WEIGHTS_MAX */
 #define UIOX_SC_COLLWEIGHTSMAX          _SC_COLL_WEIGHTS_MAX
 
 /** Maximum number of timer expiration overruns for a single event.
  *  POSIX name: DELAYTIMER_MAX  |  Query: _SC_DELAYTIMER_MAX */
 #define UIOX_SC_DELAYTIMERMAX           _SC_DELAYTIMER_MAX
 
 /** Maximum length of a host name (excluding null), per gethostname().
  *  POSIX name: HOST_NAME_MAX   |  Query: _SC_HOST_NAME_MAX */
 #define UIOX_SC_HOSTNAMEMAX             _SC_HOST_NAME_MAX
 
 /** Maximum number of iovec elements in a single scatter/gather call.
  *  POSIX name: IOV_MAX         |  Query: _SC_IOV_MAX */
 #define UIOX_SC_IOVMAX                  _SC_IOV_MAX
 
 /** Maximum length of a line produced by POSIX utilities (e.g. awk).
  *  POSIX name: LINE_MAX        |  Query: _SC_LINE_MAX */
 #define UIOX_SC_LINEMAX                 _SC_LINE_MAX
 
 /** Maximum length of a login name (excluding null terminator).
  *  POSIX name: LOGIN_NAME_MAX  |  Query: _SC_LOGIN_NAME_MAX */
 #define UIOX_SC_LOGINNAMEMAX            _SC_LOGIN_NAME_MAX
 
 /** Maximum number of supplementary group IDs per process.
  *  POSIX name: NGROUPS_MAX     |  Query: _SC_NGROUPS_MAX */
 #define UIOX_SC_NGROUPSMAX              _SC_NGROUPS_MAX
 
 /** Initial buffer size suggested for getgrgid_r() / getgrnam_r().
  *  Query: _SC_GETGR_R_SIZE_MAX */
 #define UIOX_SC_GETGRRSIZEMAX           _SC_GETGR_R_SIZE_MAX
 
 /** Initial buffer size suggested for getpwuid_r() / getpwnam_r().
  *  Query: _SC_GETPW_R_SIZE_MAX */
 #define UIOX_SC_GETPWRSIZEMAX           _SC_GETPW_R_SIZE_MAX
 
 /** Maximum number of files a process may have open simultaneously.
  *  POSIX name: OPEN_MAX        |  Query: _SC_OPEN_MAX */
 #define UIOX_SC_OPENMAX                 _SC_OPEN_MAX
 
 /** Size of a memory page in bytes (architecture-specific).
  *  POSIX name: PAGESIZE        |  Query: _SC_PAGESIZE  (alias: _SC_PAGE_SIZE) */
 #define UIOX_SC_PAGESIZE                _SC_PAGESIZE
 
 /* -- Thread limits (deferred to chapter 12) ------------------------ */
 
 /** Max number of destructor functions per thread-specific data key.
  *  Deferred: see chapter 12.  |  Query: _SC_THREAD_DESTRUCTOR_ITERATIONS */
 #define UIOX_SC_PTHREADDESTRUCTORITERATIONS  _SC_THREAD_DESTRUCTOR_ITERATIONS
 
 /** Max number of thread-specific data keys per process.
  *  Deferred: see chapter 12.  |  Query: _SC_THREAD_KEYS_MAX */
 #define UIOX_SC_PTHREADKEYSMAX          _SC_THREAD_KEYS_MAX
 
 /** Minimum stack size for a thread, in bytes.
  *  Deferred: see chapter 12.  |  Query: _SC_THREAD_STACK_MIN */
 #define UIOX_SC_PTHREADSTACKMIN         _SC_THREAD_STACK_MIN
 
 /** Maximum number of threads per process.
  *  Deferred: see chapter 12.  |  Query: _SC_THREAD_THREADS_MAX */
 #define UIOX_SC_PTHREADTHREADSMAX       _SC_THREAD_THREADS_MAX
 
 /* -- Signal & IPC limits ------------------------------------------- */
 
 /** Maximum number of file descriptors that dup2() can create.
  *  POSIX name: RE_DUP_MAX     |  Query: _SC_RE_DUP_MAX */
 #define UIOX_SC_REDUPMAX                _SC_RE_DUP_MAX
 
 /** Maximum number of real-time signals reserved for application use.
  *  POSIX name: RTSIG_MAX      |  Query: _SC_RTSIG_MAX */
 #define UIOX_SC_RTSIGMAX                _SC_RTSIG_MAX
 
 /** Maximum number of semaphores a process may have open.
  *  POSIX name: SEM_NSEMS_MAX  |  Query: _SC_SEM_NSEMS_MAX */
 #define UIOX_SC_SEMNSEMSMAX             _SC_SEM_NSEMS_MAX
 
 /** Maximum value a semaphore can hold.
  *  POSIX name: SEM_VALUE_MAX  |  Query: _SC_SEM_VALUE_MAX */
 #define UIOX_SC_SEMVALUEMAX             _SC_SEM_VALUE_MAX
 
 /** Maximum number of signals that can be queued for a process.
  *  POSIX name: SIGQUEUE_MAX   |  Query: _SC_SIGQUEUE_MAX */
 #define UIOX_SC_SIGQUEUEMAX             _SC_SIGQUEUE_MAX
 
 /** Maximum number of streams a process may have open simultaneously.
  *  POSIX name: STREAM_MAX     |  Query: _SC_STREAM_MAX */
 #define UIOX_SC_STREAMMAX               _SC_STREAM_MAX
 
 /** Maximum number of symbolic links that may be traversed in one lookup.
  *  POSIX name: SYMLOOP_MAX    |  Query: _SC_SYMLOOP_MAX */
 #define UIOX_SC_SYMLOOPMAX              _SC_SYMLOOP_MAX
 
 /** Maximum number of POSIX timers a process may create.
  *  POSIX name: TIMER_MAX      |  Query: _SC_TIMER_MAX */
 #define UIOX_SC_TIMERMAX                _SC_TIMER_MAX
 
 /** Maximum length of a terminal device name (including null).
  *  POSIX name: TTY_NAME_MAX   |  Query: _SC_TTY_NAME_MAX */
 #define UIOX_SC_TTYNAMEMAX              _SC_TTY_NAME_MAX
 
 /** Maximum number of bytes in a timezone name string.
  *  POSIX name: TZNAME_MAX     |  Query: _SC_TZNAME_MAX */
 #define UIOX_SC_TZNAMEMAX               _SC_TZNAME_MAX
 
 /** @} */  /* end sysconf_limits */
 
 /* ================================================================== */
 /*  SECTION 2 — POSIX Optional Feature Flags                          */
 /* ================================================================== */
 
 /**
  * @defgroup sysconf_features  POSIX optional feature availability flags
  *
  * Query these with sysconf() at runtime. Return value > 0 means the
  * feature is supported. Items marked [required] must be supported on
  * any conforming POSIX.1-2017 implementation.
  * @{
  */
 
 /** Advisory file locking (fcntl/lockf) is supported.
  *  Query: _SC_ADVISORY_INFO */
 #define UIOX_SC_POSIXADVISORYINFO          _SC_ADVISORY_INFO
 
 /** Asynchronous I/O (aio_*) is supported. [required]
  *  Query: _SC_ASYNCHRONOUS_IO */
 #define UIOX_SC_POSIXASYNCHRONOUSIO        _SC_ASYNCHRONOUS_IO
 
 /** Thread barriers (pthread_barrier_*) are supported. [required]
  *  Query: _SC_BARRIERS */
 #define UIOX_SC_POSIXBARRIERS              _SC_BARRIERS
 
 /** Clock selection (clock_nanosleep with clock choice) supported. [required]
  *  Query: _SC_CLOCK_SELECTION */
 #define UIOX_SC_POSIXCLOCKSELECTION        _SC_CLOCK_SELECTION
 
 /** Per-process CPU-time clocks (CLOCK_PROCESS_CPUTIME_ID) supported.
  *  Query: _SC_CPUTIME */
 #define UIOX_SC_POSIXCPUTIME               _SC_CPUTIME
 
 /** Synchronized I/O (fsync/fdatasync) is supported.
  *  Query: _SC_FSYNC */
 #define UIOX_SC_POSIXFSYNC                 _SC_FSYNC
 
 /** IPv6 networking interfaces are available.
  *  Query: _SC_IPV6 */
 #define UIOX_SC_POSIXIPV6                  _SC_IPV6
 
 /** Job control (tcsetpgrp, SIGTSTP, etc.) is supported.
  *  Query: _SC_JOB_CONTROL */
 #define UIOX_SC_POSIXJOBCONTROL            _SC_JOB_CONTROL
 
 /** Memory-mapped files (mmap) are supported. [required]
  *  Query: _SC_MAPPED_FILES */
 #define UIOX_SC_POSIXMAPPEDFILES           _SC_MAPPED_FILES
 
 /** Whole-process memory locking (mlockall) is supported.
  *  Query: _SC_MEMLOCK */
 #define UIOX_SC_POSIXMEMLOCK               _SC_MEMLOCK
 
 /** Range memory locking (mlock) is supported.
  *  Query: _SC_MEMLOCK_RANGE */
 #define UIOX_SC_POSIXMEMLOCKRANGE          _SC_MEMLOCK_RANGE
 
 /** Memory protection (mprotect) is supported. [required]
  *  Query: _SC_MEMORY_PROTECTION */
 #define UIOX_SC_POSIXMEMORYPROTECTION      _SC_MEMORY_PROTECTION
 
 /** POSIX message queues (mq_*) are supported.
  *  Query: _SC_MESSAGE_PASSING */
 #define UIOX_SC_POSIXMESSAGEPASSING        _SC_MESSAGE_PASSING
 
 /** Monotonic clock (CLOCK_MONOTONIC) is supported.
  *  Query: _SC_MONOTONIC_CLOCK */
 #define UIOX_SC_POSIXMONOTONICCLOCK        _SC_MONOTONIC_CLOCK
 
 /** Prioritized I/O (aio priority) is supported.
  *  Query: _SC_PRIORITIZED_IO */
 #define UIOX_SC_POSIXPRIORITIZEDIO         _SC_PRIORITIZED_IO
 
 /** Priority scheduling (sched_setscheduler, etc.) is supported.
  *  Query: _SC_PRIORITY_SCHEDULING */
 #define UIOX_SC_POSIXPRIORITYSCHEDULING    _SC_PRIORITY_SCHEDULING
 
 /** Raw socket support is available.
  *  Query: _SC_RAW_SOCKETS */
 #define UIOX_SC_POSIXRAWSOCKETS            _SC_RAW_SOCKETS
 
 /** Reader-writer locks (pthread_rwlock_*) are supported. [required]
  *  Query: _SC_READER_WRITER_LOCKS */
 #define UIOX_SC_POSIXREADERWRITERLOCKS     _SC_READER_WRITER_LOCKS
 
 /** Real-time signals (SIGRTMIN..SIGRTMAX) are supported. [required]
  *  Query: _SC_REALTIME_SIGNALS */
 #define UIOX_SC_POSIXREALTIMESIGNALS       _SC_REALTIME_SIGNALS
 
 /** POSIX regular expression support (regcomp/regexec) is available.
  *  Query: _SC_REGEXP */
 #define UIOX_SC_POSIXREGEXP                _SC_REGEXP
 
 /** Saved set-user-ID and saved set-group-ID are supported.
  *  Query: _SC_SAVED_IDS */
 #define UIOX_SC_POSIXSAVEDIDS              _SC_SAVED_IDS
 
 /** POSIX named/unnamed semaphores (sem_*) are supported. [required]
  *  Query: _SC_SEMAPHORES */
 #define UIOX_SC_POSIXSEMAPHORES            _SC_SEMAPHORES
 
 /** POSIX shared memory objects (shm_open/shm_unlink) are supported.
  *  Query: _SC_SHARED_MEMORY_OBJECTS */
 #define UIOX_SC_POSIXSHAREDMEMORYOBJECTS   _SC_SHARED_MEMORY_OBJECTS
 
 /** A POSIX-compliant shell is available.
  *  Query: _SC_SHELL */
 #define UIOX_SC_POSIXSHELL                 _SC_SHELL
 
 /** posix_spawn() family is supported.
  *  Query: _SC_SPAWN */
 #define UIOX_SC_POSIXSPAWN                 _SC_SPAWN
 
 /** Spin locks (pthread_spin_*) are supported. [required]
  *  Query: _SC_SPIN_LOCKS */
 #define UIOX_SC_POSIXSPINLOCKS             _SC_SPIN_LOCKS
 
 /** Sporadic server scheduling policy is supported.
  *  Query: _SC_SPORADIC_SERVER */
 #define UIOX_SC_POSIXSPORADICSERVER        _SC_SPORADIC_SERVER
 
 /** Synchronized I/O (O_SYNC/O_DSYNC) is supported.
  *  Query: _SC_SYNCHRONIZED_IO */
 #define UIOX_SC_POSIXSYNCHRONIZEDIO        _SC_SYNCHRONIZED_IO
 
 /* -- Thread options (deferred to chapter 12) ----------------------- */
 
 /** Thread stack address attribute is supported.  (chapter 12)
  *  Query: _SC_THREAD_ATTR_STACKADDR */
 #define UIOX_SC_POSIXTHREADATTRSTACKADDR      _SC_THREAD_ATTR_STACKADDR
 
 /** Thread stack size attribute is supported.  (chapter 12)
  *  Query: _SC_THREAD_ATTR_STACKSIZE */
 #define UIOX_SC_POSIXTHREADATTRSTACKSIZE      _SC_THREAD_ATTR_STACKSIZE
 
 /** Per-thread CPU-time clocks are supported.  (chapter 12)
  *  Query: _SC_THREAD_CPUTIME */
 #define UIOX_SC_POSIXTHREADATTRCPUTIME        _SC_THREAD_CPUTIME
 
 /** Thread priority inheritance is supported.  (chapter 12)
  *  Query: _SC_THREAD_PRIO_INHERIT */
 #define UIOX_SC_POSIXTHREADPRIOINHERIT        _SC_THREAD_PRIO_INHERIT
 
 /** Thread priority scheduling is supported.  (chapter 12)
  *  Query: _SC_THREAD_PRIORITY_SCHEDULING */
 #define UIOX_SC_POSIXTHREADPRIORITYSCHEDULING _SC_THREAD_PRIORITY_SCHEDULING
 
 /** Thread process-shared synchronization is supported.  (chapter 12)
  *  Query: _SC_THREAD_PROCESS_SHARED */
 #define UIOX_SC_POSIXTHREADPROCESSSHARED      _SC_THREAD_PROCESS_SHARED
 
 /** Robust mutexes with priority inheritance are supported.  (chapter 12)
  *  Query: _SC_THREAD_ROBUST_PRIO_INHERIT */
 #define UIOX_SC_POSIXTHREADROBUSTPRIOINHERIT  _SC_THREAD_ROBUST_PRIO_INHERIT
 
 /** Robust mutexes with priority protection are supported.  (chapter 12)
  *  Query: _SC_THREAD_ROBUST_PRIO_PROTECT */
 #define UIOX_SC_POSIXTHREADROBUSTPRIOPROTECT  _SC_THREAD_ROBUST_PRIO_PROTECT
 
 /** Thread-safe functions (_r variants) are supported. [required] (ch.12)
  *  Query: _SC_THREAD_SAFE_FUNCTIONS */
 #define UIOX_SC_POSIXTHREADSAFEFUNCTIONS      _SC_THREAD_SAFE_FUNCTIONS
 
 /** Thread sporadic server is supported.  (chapter 12)
  *  Query: _SC_ _POSIX_THREAD_SPORADIC_SERVER	_SC_THREAD_SPORADIC_SERVER	;defer to chapter 12
_POSIX_THREADS	_SC_THREADS	; required
_POSIX_TIMEOUTS	_SC_TIMEOUTS	; required
_POSIX_TIMERS	_SC_TIMERS	; required
_POSIX_TYPED_MEMORY_OBJECTS	_SC_TYPED_MEMORY_OBJECTS
_POSIX_VERSION	_SC_VERSION
# _POSIX_V7_ILP32_OFF32	_SC_V7_ILP32_OFF32	;defer to chapter 3
# _POSIX_V7_ILP32_OFFBIG	_SC_V7_ILP32_OFFBIG	;defer to chapter 3
# _POSIX_V7_LP64_OFF64	_SC_V7_LP64_OFF64	;defer to chapter 3
# _POSIX_V7_LPBIG_OFFBIG	_SC_V7_LPBIG_OFFBIG	;defer to chapter 3
_XOPEN_CRYPT		_SC_XOPEN_CRYPT
_XOPEN_ENH_I18N		_SC_XOPEN_ENH_I18N
_XOPEN_REALTIME		_SC_XOPEN_REALTIME
_XOPEN_REALTIME_THREADS	_SC_XOPEN_REALTIME_THREADS
_XOPEN_SHM		_SC_XOPEN_SHM
# obsolete _XOPEN_STREAMS		_SC_XOPEN_STREAMS
_XOPEN_UNIX		_SC_XOPEN_UNIX
_XOPEN_UUCP		_SC_XOPEN_UUCP
_XOPEN_VERSION		_SC_XOPEN_VERSION */

#endif