/**
 * @file  uix_posix.h
 * @brief UIOX POSIX compatibility — single umbrella include.
 *
 * Including this file gives a program access to all POSIX-compatible
 * function wrappers implemented in the uixlibs layer.
 *
 * Usage:
 *   #include "uix_posix.h"
 *   // Now use read(), write(), fork(), mmap(), socket(), etc.
 *
 * Place: 50_UIX/00_libs/00_uixlibs/uix_posix.h
 */

 #ifndef UIX_POSIX_H
 #define UIX_POSIX_H
 
 /* ── Core POSIX types and errno ──────────────────────────────── */
 #include "PoStd/uix_stddef.h"
 #include "PoStd/uix_stdint.h"
 #include "PoStd/uix_stdbool.h"
 #include "PoStd/uix_errno.h"
 #include "sys/uix_types.h"
 
 /* ── Arch-portable syscall dispatcher ───────────────────────── */
 #include "PoStd/uix_syscall.h"
 
 /* ── POSIX functional layers ─────────────────────────────────── */
 #include "PoStd/uix_posix_io.h"      /* read/write/open/close/lseek/dup */
 #include "PoStd/uix_posix_proc.h"    /* fork/exec/wait/exit/kill/signal */
 #include "PoStd/uix_posix_fs.h"      /* stat/mkdir/unlink/chmod/opendir */
 #include "PoStd/uix_posix_mm.h"      /* mmap/munmap/mprotect/brk        */
 
 /* ── Network ─────────────────────────────────────────────────── */
 #include "sys/uix_socket.h"
 #include "arpa/inet.h"    /* if present */
 #include "netinet/in.h"   /* if present */
 
 /* ── Standard C ─────────────────────────────────────────────── */
 #include "PoStd/uix_stdio.h"
 #include "PoStd/uix_stdlib.h"
 #include "PoStd/uix_string.h"
 #include "PoStd/uix_unistd.h"
 #include "PoStd/uix_fcntl.h"
 #include "PoStd/uix_signal.h"
 #include "PoStd/uix_termios.h"
 
 /* ── uix_sys.h (syscall numbers + existing subsystem includes) ─ */
 #include "../../40_SystemCallInterface/uix_sys.h"
 
 /* ── Source-compatibility: map POSIX names to uix_ names ─────── */
 /*   Already done inside each uix_posix_*.h via #define macros.   */
 
 #endif /* UIX_POSIX_H */
/*
50_UIX/00_libs/00_uixlibs/
├── PoStd/
│   ├── uix_syscall.h          # NEW — arch-portable syscall() + errno wiring
│   ├── uix_syscall.c          # NEW — syscall() implementation
│   ├── uix_posix_io.h         # NEW — read/write/open/close/lseek/dup wrappers
│   ├── uix_posix_io.c         # NEW — I/O syscall implementations
│   ├── uix_posix_proc.h       # NEW — fork/exec/wait/exit/getpid wrappers
│   ├── uix_posix_proc.c       # NEW — process syscall implementations
│   ├── uix_posix_fs.h         # NEW — stat/mkdir/rmdir/chdir/link/unlink
│   ├── uix_posix_fs.c         # NEW — filesystem syscall implementations
│   ├── uix_posix_mm.h         # NEW — mmap/munmap/mprotect/brk
│   ├── uix_posix_mm.c         # NEW — memory syscall implementations
│   └── uix_posix_time.h       # NEW — clock_gettime/nanosleep/gettimeofday
├── sys/
│   └── uix_posix_socket.c     # NEW — socket/bind/connect/send/recv
└── uix_posix.h                # NEW — single umbrella include for all POSIX wrappers

*/ 