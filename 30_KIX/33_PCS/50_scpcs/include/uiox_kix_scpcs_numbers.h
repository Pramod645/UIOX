/*
 * 30_KIX/33_PCS/50_scpcs/include/uiox_kix_scpcs_numbers.h
 *
 * SCPCS — the BSD syscall numbers THIS layer's dispatch table routes.
 *
 * ── why these are copied here rather than included ──────────────────
 * The dispatcher used to reach up to 40_SCIX/uix_sys.h for these.  That
 * is an UPWARD dependency: 40_SCIX is the syscall boundary ABOVE this
 * layer, and a process-control layer including the boundary's header
 * inverts the direction of calls.  The numbers are constants — they
 * belong below, beside the table that indexes them.  Same fix as
 * 10scfs's uioxkixscfsnumbers.h, for the same reason.
 *
 * ── which block of uix_sys.h these come from ────────────────────────
 * 40_SCIX/uix_sys.h defines many of these names TWICE with DIFFERENT
 * values — once in its canonical numeric run, once in a block of
 * #ifndef fallbacks near the end.  Examples:
 *
 *     SYS_READV    120 (canonical)  vs   19 (fallback)
 *     SYS_LSTAT     40 (canonical)  vs  107 (fallback)
 *     SYS_TRUNCATE 167 (canonical)  vs   92 (fallback)
 *     SYS_CLOCK_GETTIME 87          vs  228
 *
 * The CANONICAL values are the ones below.  The fallback block's values
 * are Linux-shaped and match nothing on the userspace side; mixing them
 * in would dispatch a syscall to the wrong handler while compiling
 * clean, so every define here is #ifndef-guarded against exactly that.
 *
 * If 40_SCIX is ever fixed so its fallback block agrees with its
 * canonical block, this header can retire to a single include.
 *
 * @version 1.0.0  @date 2026-10-11
 */
#ifndef UIOX_KIX_SCPCS_NUMBERS_H
#define UIOX_KIX_SCPCS_NUMBERS_H

/* ── process lifecycle ─────────────────────────────────────────────── */
#ifndef SYS_EXIT
#define SYS_EXIT            1
#endif
#ifndef SYS_FORK
#define SYS_FORK            2
#endif
#ifndef SYS_WAIT4
#define SYS_WAIT4           11
#endif
#ifndef SYS_BREAK
#define SYS_BREAK           17
#endif
#ifndef SYS_EXECV
#define SYS_EXECV           250
#endif

/* ── identity ──────────────────────────────────────────────────────── */
#ifndef SYS_GETPID
#define SYS_GETPID          20
#endif
#ifndef SYS_SETUID
#define SYS_SETUID          23
#endif
#ifndef SYS_GETUID
#define SYS_GETUID          24
#endif
#ifndef SYS_GETEUID
#define SYS_GETEUID         25
#endif
#ifndef SYS_GETGID
#define SYS_GETGID          47
#endif
#ifndef SYS_GETEGID
#define SYS_GETEGID         50
#endif
#ifndef SYS_SETGID
#define SYS_SETGID          181
#endif
#ifndef SYS_GETPGRP
#define SYS_GETPGRP         81
#endif
#ifndef SYS_GETPGID
#define SYS_GETPGID         207
#endif
#ifndef SYS_GETPPID
#define SYS_GETPPID         64   /* fallback block only; collides with   */
#endif                           /* SYS_FSTATFS 64 in the canonical run — */
                                 /* not a conflict here, this layer does  */
                                 /* not route FSTATFS                    */

/* ── signals ───────────────────────────────────────────────────────── */
#ifndef SYS_KILL
#define SYS_KILL            122
#endif
#ifndef SYS_SIGACTION
#define SYS_SIGACTION       46
#endif
#ifndef SYS_SIGPROCMASK
#define SYS_SIGPROCMASK     48
#endif
#ifndef SYS_SIGPENDING
#define SYS_SIGPENDING      52
#endif
#ifndef SYS_SIGSUSPEND
#define SYS_SIGSUSPEND      111
#endif

/* ── time ──────────────────────────────────────────────────────────── */
/* ── time ──────────────────────────────────────────────────────────── */
#ifndef SYS_ALARM
#define SYS_ALARM           27   /* fallback block only — matches       */
#endif                           /* 50_UIX/…/uix_posix_proc.c:130       */
#ifndef SYS_NANOSLEEP
#define SYS_NANOSLEEP       91
#endif
#ifndef SYS_GETTIMEOFDAY
#define SYS_GETTIMEOFDAY    67
#endif
#ifndef SYS_SETTIMEOFDAY
#define SYS_SETTIMEOFDAY    68
#endif
#ifndef SYS_SETITIMER
#define SYS_SETITIMER       69
#endif
#ifndef SYS_GETITIMER
#define SYS_GETITIMER       70
#endif
#ifndef SYS_CLOCK_GETTIME
#define SYS_CLOCK_GETTIME   87
#endif
#ifndef SYS_CLOCK_SETTIME
#define SYS_CLOCK_SETTIME   88
#endif
#ifndef SYS_CLOCK_GETRES
#define SYS_CLOCK_GETRES    89
#endif
#ifndef SYS_ADJTIME
#define SYS_ADJTIME         140
#endif
#ifndef SYS_TIMES
#define SYS_TIMES           346
#endif

/* ── scheduling ────────────────────────────────────────────────────── */
#ifndef SYS_SETPRIORITY
#define SYS_SETPRIORITY     96
#endif

/* ── System V IPC (reachable through this table, owned by 00_IPC) ───── */
#ifndef SYS_SEMGET
#define SYS_SEMGET          221
#endif
#ifndef SYS_SEMOP
#define SYS_SEMOP           290
#endif
#ifndef SYS_SEMCTL
#define SYS_SEMCTL          295
#endif
#ifndef SYS_MSGGET
#define SYS_MSGGET          225
#endif
#ifndef SYS_MSGSND
#define SYS_MSGSND          226
#endif
#ifndef SYS_MSGRCV
#define SYS_MSGRCV          227
#endif
#ifndef SYS_MSGCTL
#define SYS_MSGCTL          297
#endif
#ifndef SYS_SHMGET
#define SYS_SHMGET          289
#endif
#ifndef SYS_SHMAT
#define SYS_SHMAT           228
#endif
#ifndef SYS_SHMDT
#define SYS_SHMDT           230
#endif
#ifndef SYS_SHMCTL
#define SYS_SHMCTL          296
#endif

/* ── debug ─────────────────────────────────────────────────────────── */
#ifndef SYS_PTRACE
#define SYS_PTRACE          26
#endif

#endif /* UIOX_KIX_SCPCS_NUMBERS_H */
