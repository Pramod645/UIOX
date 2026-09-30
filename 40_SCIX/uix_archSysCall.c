/*
 * 40_SystemCallInterface/uix_archSysCall.c
 *
 * System Call Interface — the THIN LAYER.
 *
 *   user space   50_UIX              sys_*()   stub or forward
 *        |
 *   thin layer   40_SystemCallInterface        <-- THIS FILE
 *        |
 *   kernel       30_KIX
 *                ├── 32_FileSystem/10_scfs     fs_*()
 *                └── 33_PCS
 *                     ├── 01_schedular         timekeeping, timer wheel
 *                     ├── 40_psa               algorithms
 *                     └── 50_scpcs             process calls
 *
 * ── what this layer does, and nothing else ──────────────────────────
 * It receives a NUMBER and up to six arguments from the arch trap
 * handler, decides WHICH subsystem owns that number, calls that
 * subsystem's dispatcher, and returns the result.  It holds no process
 * logic and no file logic — a place where it did would be a sign the
 * boundary had drifted.
 *
 * ── numbering: BSD'S, taken as given ────────────────────────────────
 * The numbers are NOT invented here.  They are the BSD syscall numbers
 * that 50_UIX's uix_sys.h already defines and that a userspace program
 * compiled against it will use — SYS_EXIT 1, SYS_FORK 2, SYS_GETPID 20,
 * SYS_CLOCK_GETTIME 87, SYS_KILL 122, and so on.
 *
 * That matters more than it looks.  A program calling number 20 expects
 * getpid.  If this layer renumbered, that program would silently receive
 * whatever else landed on 20 — a wrong answer rather than a failure,
 * which is the hardest kind to find.
 *
 * ── two subsystems, one number space ────────────────────────────────
 * Each number is owned by exactly one subsystem, and the ownership
 * tables below say which.  A number in NO table returns ENOSYS — it is
 * a real BSD syscall this kernel does not implement, which is a
 * different fact from "not a syscall at all".
 *
 * ── where the third group went ──────────────────────────────────────
 * The timing calls (SYS_GETTIMEOFDAY 67, SYS_CLOCK_GETTIME 87,
 * SYS_NANOSLEEP 91, SYS_ADJTIME 140, and the itimer pair) are declared
 * and implemented in 33_PCS/01_schedular — timekeeping.c owns xtime and
 * the timer wheel is timer.c's.  They are PROCESS-facing calls, so this
 * file routes them to the process dispatcher rather than to a third
 * table.  A separate timing dispatcher would be a fourth place deciding
 * what a number means.
 *
 * @version 2.0.0  @date 2026-09-29
 */

 #include "uix_sys.h"                 /* the BSD numbers               */
 #include "uix_archSysCall.h"         /* the entry point this declares */
 
 /* ── The two subsystem dispatchers ─────────────────────────────────
  * Declared here rather than included, because including both headers
  * would drag both subsystems' types into this file — and this layer
  * deliberately knows nothing about processes or files beyond "there is
  * a function I hand a number to".
  *
  * The regs parameter is void * rather than a typed pointer for the same
  * reason: SCiX must not depend on 40_psa's context layout to pass it
  * through. */
 extern int64_t uiox_kix_scpcs_dispatch(uiox_uint64_t nr,
                                        uiox_uintptr_t a0, uiox_uintptr_t a1,
                                        uiox_uintptr_t a2, uiox_uintptr_t a3,
                                        uiox_uintptr_t a4, uiox_uintptr_t a5,
                                        void *regs);
 extern int64_t uiox_kix_scfs_dispatch(uiox_uint64_t nr,
                                       uiox_uintptr_t a0, uiox_uintptr_t a1,
                                       uiox_uintptr_t a2, uiox_uintptr_t a3,
                                       uiox_uintptr_t a4, uiox_uintptr_t a5,
                                       void *regs);
 
 /* ── Return convention ──────────────────────────────────────────────
  * Negative is an error, and it propagates unchanged from the subsystem
  * below.  This layer does not reinterpret one subsystem's error as
  * another's: a failure means the same thing wherever it came from. */
 #define SCIX_ENOSYS   ((int64_t)-38)
 #define SCIX_EINVAL   ((int64_t)-22)
 
 /* ── Which subsystem owns which number ──────────────────────────────
  * The numbers are BSD's; the ranges are this kernel's decision about who
  * implements them.  Where BSD spreads a group over a wide span, several
  * narrow cases appear rather than one wide range that would swallow
  * numbers belonging to nobody.
  * ──────────────────────────────────────────────────────────────────── */
 
 /* ── 32_FileSystem/10_scfs ────────────────────────────────────────── */
 static int scfs_owns(uiox_uint64_t nr)
 {
     switch (nr) {
     /* file open, read, write, close */
     case SYS_READ:        case SYS_WRITE:      case SYS_OPEN:
     case SYS_CLOSE:       case SYS_LSEEK:      case SYS_DUP:
     case SYS_FCNTL:       case SYS_IOCTL:
     /* metadata */
     case SYS_STAT:        case SYS_FSTAT:      case SYS_LSTAT:
     case SYS_ACCESS:      case SYS_CHMOD:      case SYS_CHOWN:
     case SYS_FCHMOD:      case SYS_FCHOWN:     case SYS_UMASK:
     case SYS_TRUNCATE:    case SYS_FTRUNCATE:  case SYS_FSYNC:
     /* directories and links */
     case SYS_MKDIR:       case SYS_RMDIR:      case SYS_CHDIR:
     case SYS_LINK:        case SYS_UNLINK:     case SYS_SYMLINK:
     case SYS_READLINK:    case SYS_RENAME:     case SYS_MKNOD:
     case SYS_CHROOT:      case SYS_GETDENTS:
     /* memory mapping, file-backed when it carries an fd */
     case SYS_MMAP:        case SYS_MUNMAP:     case SYS_MPROTECT:
         return 1;
     default:
         return 0;
     }
 }
 
 /* ── 33_PCS/50_scpcs — process control ───────────────────────────── */
 static int scpcs_owns(uiox_uint64_t nr)
 {
     switch (nr) {
     /* process control */
     case SYS_EXIT:        case SYS_FORK:       case SYS_EXECV:
     case SYS_WAIT4:       case SYS_BREAK:
     /* identity */
     case SYS_GETPID:      case SYS_GETUID:     case SYS_GETEUID:
     case SYS_GETGID:      case SYS_SETUID:     case SYS_SETGID:
     case SYS_GETPGRP:     case SYS_GETPGID:
     /* signals */
     case SYS_KILL:        case SYS_SIGACTION:  case SYS_SIGPROCMASK:
     case SYS_SIGPENDING:  case SYS_SIGSUSPEND:
     /* scheduling: priority adjustment is a process attribute */
     case SYS_SETPRIORITY:
         return 1;
     default:
         return 0;
     }
 }
 
 /* ── 33_PCS/01_schedular — the timing calls ──────────────────────────
  * A THIRD ownership group, and it does not get a third table: these
  * numbers route to the process dispatcher alongside the calls above,
  * because 01_schedular is a part of the process control subsystem and
  * its entry points are reached through the same boundary.
  *
  * The functions themselves live in 01_schedular/src/syscall_time.c and
  * read xtime / the timer wheel — see syscall_time.h.  What this table
  * decides is only WHICH NUMBER reaches them.
  *
  * Note SYS_GETTIMEOFDAY serves both gettimeofday() and time(): BSD has
  * no separate time() number, so a time() call arrives on 67 and the
  * wrapper returns the seconds alone.
  * ──────────────────────────────────────────────────────────────────── */
 static int scix_time_owns(uiox_uint64_t nr)
 {
     switch (nr) {
     case SYS_GETTIMEOFDAY:      /*  67  sys_gettimeofday / sys_time   */
     case SYS_SETTIMEOFDAY:      /*  68  sys_clock_settime (rt)        */
     case SYS_SETITIMER:         /*  69  sys_setitimer                 */
     case SYS_GETITIMER:         /*  70  sys_getitimer                 */
     case SYS_CLOCK_GETTIME:     /*  87  sys_clock_gettime             */
     case SYS_CLOCK_SETTIME:     /*  88  sys_clock_settime             */
     case SYS_CLOCK_GETRES:      /*  89  sys_clock_getres              */
     case SYS_NANOSLEEP:         /*  91  sys_clock_nanosleep           */
     case SYS_ADJTIME:           /* 140  sys_adjtimex                  */
         return 1;
     default:
         return 0;
     }
 }
 
 /* ── 33_PCS/00IPC — System V message queues, semaphores, shared memory,
  * and process tracing ───────────────────────────────────────────────
  * A FOURTH ownership group with no fourth TABLE: these numbers route to
  * the process dispatcher alongside the calls above, because 00IPC is a
  * sibling of 50_scpcs inside 33_PCS rather than a layer of its own.
  *
  * The numbers are BSD's and are scattered, as IPC's family always is:
  * ptrace sits at 26, the System V calls between 221 and 297.
  *
  * Note that a call the kernel does not implement is NOT listed here.
  * SYS_MSGSND with no queue, SYS_SEMOP on a bad set — those are the
  * dispatcher's EINVAL, not an ownership question.
  * ──────────────────────────────────────────────────────────────────── */
 static int scix_ipc_owns(uiox_uint64_t nr)
 {
     switch (nr) {
     case SYS_PTRACE:      /*  26  process tracing            */
     case SYS_SEMGET:      /* 221  semaphore set, create/find */
     case SYS_MSGGET:      /* 225  message queue, create/find */
     case SYS_MSGSND:      /* 226  send a message             */
     case SYS_MSGRCV:      /* 227  receive a message          */
     case SYS_SHMAT:       /* 228  attach shared memory       */
     case SYS_SHMDT:       /* 230  detach shared memory       */
     case SYS_SHMGET:      /* 289  shared region, create/find */
     case SYS_SEMOP:       /* 290  semaphore operations       */
     case SYS_SEMCTL:      /* 295  semaphore control          */
     case SYS_SHMCTL:      /* 296  shared memory control      */
     case SYS_MSGCTL:      /* 297  message queue control      */
         return 1;
     default:
         return 0;
     }
 }
 
 /* ────────────────────────────────────────────────────────────────────
  * uix_arch_syscall — the SCiX entry point.
  *
  * Called by 10_Arch's trap handler with the syscall number and the six
  * argument registers, and with the interrupted register context so the
  * return value can be placed where the user program will find it.
  *
  * The ARM64 convention, which the arch layer implements:
  *   number in x8 · arguments in x0..x5 · return in x0 · svc #0
  *
  * Returns the raw int64_t as well, so a caller with no register context
  * — a test harness, or a hosted build — can still use it.
  * ──────────────────────────────────────────────────────────────────── */
 int64_t uix_arch_syscall(uiox_uint64_t nr,
                          uiox_uintptr_t a0, uiox_uintptr_t a1,
                          uiox_uintptr_t a2, uiox_uintptr_t a3,
                          uiox_uintptr_t a4, uiox_uintptr_t a5,
                          void *regs)
 {
     /* ── file system ────────────────────────────────────────────── */
     if (scfs_owns(nr))
         return uiox_kix_scfs_dispatch(nr, a0, a1, a2, a3, a4, a5, regs);
 
     /* ── process control and timing ───────────────────────────────
      * Both groups route to the same dispatcher: 01_schedular is part of
      * the process subsystem, and its entry points sit behind the same
      * boundary as the calls above. */
     if (scpcs_owns(nr) || scix_time_owns(nr) || scix_ipc_owns(nr))
         return uiox_kix_scpcs_dispatch(nr, a0, a1, a2, a3, a4, a5, regs);
 
     /* ── a real BSD number with no implementation here ─────────────
      * ENOSYS, not EINVAL.  SYS_KQUEUE is a genuine syscall; this kernel
      * does not have it.  Telling a caller its number is invalid would
      * send it looking for a bug in its own table. */
     return SCIX_ENOSYS;
 }
 
 /* ────────────────────────────────────────────────────────────────────
  * Introspection — for a diagnostic that must not guess.
  *
  * Both return NULL/-1 for an unowned number, so a caller can tell "no
  * such call" from "call with no name" without a second lookup.
  * ──────────────────────────────────────────────────────────────────── */
 const char *uix_arch_syscall_class(uiox_uint64_t nr)
 {
     if (scfs_owns(nr))      return "fs";
     if (scpcs_owns(nr))     return "proc";
     if (scix_time_owns(nr)) return "time";
     if (scix_ipc_owns(nr))  return "ipc";
     return (const char *)0;
 }
 
 int uix_arch_syscall_owned(uiox_uint64_t nr)
 {
     return scfs_owns(nr) || scpcs_owns(nr) || scix_time_owns(nr) ||
            scix_ipc_owns(nr);
 }
 