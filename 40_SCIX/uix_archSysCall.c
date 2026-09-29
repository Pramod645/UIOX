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
 *                     ├── 40_psa               algorithms
 *                     └── 50_scpcs             process calls
 *
 * ── what this layer does, and nothing else ──────────────────────────
 * It receives a NUMBER and up to six arguments from the arch trap
 * handler, decides WHICH subsystem owns that number, calls that
 * subsystem's dispatcher, and returns the result.  It holds no process
 * logic and no file logic — a prompt that it did would be a sign the
 * boundary had drifted.
 *
 * ── numbering: BSD'S, taken as given ────────────────────────────────
 * The numbers are NOT invented here.  They are the BSD syscall numbers
 * that 50_UIX's uix_sys.h already defines and that a userspace program
 * compiled against it will use — SYS_EXIT 1, SYS_FORK 2, SYS_GETPID 20,
 * SYS_CLOCK_GETTIME 87, SYS_KILL 122, and so on, roughly three hundred
 * of them.
 *
 * That matters more than it looks.  A program calling number 20 expects
 * getpid.  If this layer renumbered, that program would silently receive
 * whatever else landed on 20 — a wrong answer rather than a failure,
 * which is the hardest kind to find.  So the numbers stay as BSD
 * defines them, and this file routes only the subset the kernel
 * implements.  Everything else is ENOSYS, not EINVAL: the number IS a
 * real syscall, it is simply not implemented here yet.
 *
 * ── the two-range split ─────────────────────────────────────────────
 * Each number is owned by exactly one subsystem.  The ranges below are
 * declared as data, not as a chain of ifs, so a reader can see the whole
 * ownership map at once and a new range is one line rather than one
 * more branch.
 *
 * A number in NO range returns ENOSYS — see the note above on why that
 * is not EINVAL.
 *
 * @version 1.0.0  @date 2026-09-29
 */

 #include "uix_sys.h"                 /* the BSD numbers                */
 #include "uix_archSysCall.h"         /* the entry point this declares  */
 
 /* ── The two subsystem dispatchers ─────────────────────────────────
  * Declared here rather than included, because including both headers
  * would drag both subsystems' types into this file — and this layer
  * deliberately knows nothing about processes or files beyond "there is
  * a function I hand a number to". */
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
  * The numbers are the BSD ones; the ranges are this kernel's decision
  * about who implements them.  Where BSD spreads a group over a wide
  * span, several narrow ranges appear rather than one wide one that
  * would swallow numbers belonging to nobody.
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
     /* memory mapping, which is file-backed when it has an fd */
     case SYS_MMAP:        case SYS_MUNMAP:     case SYS_MPROTECT:
         return 1;
     default:
         return 0;
     }
 }
 
 /* ── 33_PCS/50_scpcs ──────────────────────────────────────────────── */
 static int scpcs_owns(uiox_uint64_t nr)
 {
     switch (nr) {
     /* process control */
     case SYS_EXIT:        case SYS_FORK:       case SYS_EXECVE:
     case SYS_WAIT4:       case SYS_BREAK:
     /* identity */
     case SYS_GETPID:      case SYS_GETUID:     case SYS_GETEUID:
     case SYS_GETGID:      case SYS_SETUID:     case SYS_SETGID:
     case SYS_GETPGRP:     case SYS_GETPGID:
     /* signals */
     case SYS_KILL:        case SYS_SIGACTION:  case SYS_SIGPROCMASK:
     case SYS_SIGPENDING:  case SYS_SIGRETURN:  case SYS_SIGSUSPEND:
         return 1;
     /* time and scheduling — the clock calls live in 50_scpcs because
      * they are process-facing; 01_schedular supplies the source. */
     case SYS_CLOCK_GETTIME: case SYS_NANOSLEEP:
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
 
     /* ── process control ────────────────────────────────────────── */
     if (scpcs_owns(nr))
         return uiox_kix_scpcs_dispatch(nr, a0, a1, a2, a3, a4, a5, regs);
 
     /* ── a real BSD number with no implementation here ─────────────
      * ENOSYS, not EINVAL.  SYS_KQUEUE is a genuine syscall; this kernel
      * does not have it.  Telling a caller its number is invalid would
      * send it looking for a bug in its own table. */
     return SCIX_ENOSYS;
 }
 