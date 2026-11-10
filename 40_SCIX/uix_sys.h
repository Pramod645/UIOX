#ifndef __UIX_SYS__H
#define __UIX_SYS__H
/*
 * uix_sys.h — System Call Interface
 *
 * Layer map:
 *   50_UIX/00_libs/00_uixlibs**.c
 *       └── sys_*()          ← this file
 *               ├── fs_*()   ← 32_FileSystem/10_scfs/include/fs.h
 *               └── kernel_*()← 33_ProcessControlSubsystem/50_scps/include/
 *
 * SYS_CALL_ENBLE_DISABLE:
 *   0 = stub returns  (simulation / hosted build)
 *   1 = real calls into 32_FileSystem and 33_ProcessControlSubsystem
 */
#include "sys/uix_types.h"
//#define SYS_CALL_ENBLE_DISABLE  0   /* 0=stubs, 1=real kernel calls */
/* uix_sys.h:17 — guard, so -D on the command line takes effect. */
#ifndef SYS_CALL_ENBLE_DISABLE
#define SYS_CALL_ENBLE_DISABLE  0   /* 0=stubs, 1=real kernel calls */
#endif

#define SYS_CALL_CAS_ENBLE  0   /* 0=stubs, 1=real kernel calls */

//#define STUB 1// 0:STUB, 1:SYS funtinal call to Kernal
/* ── Kernel layer headers ──────────────────────────────────── */
/* ── Trap interface (userspace side of EL0/EL1) ─────────────── */
#include "uix_archSysCall.h"
#include "PoStd/uix_fcntl.h"
#include "sys/uix_stat.h"
#include "PoStd/uix_dirent.h"
#include "PoStd/uix_unistd.h"
#include "sys/uix_wait.h"
#include "sys/uix_mman.h"
#include "sys/uix_msg.h"
#include "sys/uix_shm.h"
#include "sys/uix_sem.h"
#include "sys/uix_socket.h"
#include "sys/uix_select.h"
#include "sys/uix_time.h"
#include "sys/uix_utsname.h"
#include "PoStd/uix_poll.h"
#include "PoStd/uix_mqueue.h"
#include "PoStd/uix_sched.h"
#include "PoStd/uix_utime.h"
#include "PoStd/uix_netdb.h"
#include "PoStd/uix_ifaddrs.h"
#include "sys/uix_times.h"

#define SYS_EXIT            1 //done
#define SYS_FORK            2 //done
#define SYS_READ            3 //done
#define SYS_WRITE           4 //done
#define SYS_OPEN            5   // done
#define SYS_CLOSE           6 //done
#define SYS_GETENTROPY      7
#define SYS_TFORK           8
#define SYS_LINK            9 //done
#define SYS_UNLINK          10 //done
#define SYS_WAIT4           11//done
#define SYS_CHDIR           12 //done
#define SYS_FCHDIR          13
#define SYS_MKNOD           14//done
#define SYS_CHMOD           15 //done
#define SYS_CHOWN           16//done
#define SYS_BREAK           17
#define SYS_GETDTABLECOUNT  18
#define SYS_GETRUSAGE       19
#define SYS_GETPID          20 //done
#define SYS_MOUNT           21
#define SYS_UNMOUNT         22
#define SYS_SETUID          23
#define SYS_GETUID          24
#define SYS_GETEUID         25
#define SYS_PTRACE          26
#define SYS_RECVMSG         27
#define SYS_SENDMSG         28
#define SYS_RECVFROM        29//done
#define SYS_ACCEPT          30 //done
#define SYS_GETPEERNAME     31 //done
#define SYS_GETSOCKNAME     32 //done
#define SYS_ACCESS          33
#define SYS_CHFLAGS         34
#define SYS_FCHFLAGS        35
#define SYS_SYNC            36
#define SYS_STAT            38 //done
//#define SYS_GETPID          39 //done duplicatw
#define SYS_LSTAT           40//done
#define SYS_DUP             41 //done
#define SYS_FSTATAT         42
#define SYS_GETEPID         43
#define SYS_KTRACE          45
#define SYS_SIGACTION       46
#define SYS_GETGID          47
#define SYS_SIGPROCMASK     48
#define SYS_MMAP            49
#define SYS_SETLOGIN        50
#define SYS_ACCT            51
#define SYS_SIGPENDING      52
#define SYS_FSTAT           53 //done
#define SYS_IOCTL           54 //done
#define SYS_REBOOT          55
#define SYS_REVOKE          56
#define SYS_SYMLINK         57
#define SYS_READLINK        58
//#define SYS_EXECVE          59 //duplicate
#define SYS_EXECV           250 //done
#define SYS_UMASK           60
#define SYS_CHROOT          61 //done
#define SYS_GETFSSTAT       62
#define SYS_STATFS          63
#define SYS_FSTATFS         64
#define SYS_FHSTATFS        65
#define SYS_VFORK           66
#define SYS_GETTIMEOFDAY    67 //done
#define SYS_SETTIMEOFDAY    68
#define SYS_SETITIMER       69
#define SYS_GETITIMER       70
#define SYS_SELECT          71 //done
#define SYS_KEVENT          72
#define SYS_MUNMAP          73 //done
#define SYS_MPROTECT        74 //done
#define SYS_MADVISE         75
#define SYS_UTIMES          76
#define SYS_FUTIMES         77
#define SYS_MQUERY          78
#define SYS_GETGROUPS       79
#define SYS_SETGROUPS       80
#define SYS_GETPGRP         81
#define SYS_SETPPID         82
#define SYS_FUTEX           83
#define SYS_UTIMENSAT       84
#define SYS_FUTIMENS        85
#define SYS_KBIND           86
#define SYS_CLOCK_GETTIME   87 //done
#define SYS_CLOCK_SETTIME   88
#define SYS_CLOCK_GETRES    89
#define SYS_DUP2            90
#define SYS_NANOSLEEP       91
#define SYS_FCNTL           92 // done
#define SYS_ACCEPT4         93
#define SYS_THRSLEEP        94
#define SYS_FSYNC           95
#define SYS_SETPRIORITY     96
#define SYS_SOCKET          97//done
#define SYS_CONNECT         98 //done
#define SYS_GETDENTS        99 //done
#define SYS_GETPRIORITY     100
#define SYS_PIPE2           101
#define SYS_DUP3            102
#define SYS_SIGRETURN       103
#define SYS_BIND            104 //done
#define SYS_SETSOCKOPT      105 //done
#define SYS_LISTEN          106 //done
#define SYS_SEND            44
#define SYS_RECV            45
#define SYS_CHFLAGSAT       107
#define SYS_PLEDGE          108
#define SYS_PPOLL           109
#define SYS_PSELECT         110
#define SYS_SIGSUSPEND      111
#define SYS_SENDSYSLOG      112
#define SYS_UNVEIL          114
#define SYS_REALPATH        115
#define SYS_RECVMMSG        116
#define SYS_SENDMMSG        117
#define SYS_GETSOCKOPT      118 //done
#define SYS_THRKILL         119
#define SYS_READV           120
#define SYS_WRITEV          121
#define SYS_KILL            122 //done
#define SYS_FCHOWN          123
#define SYS_FCHMOD          124
#define SYS_PLEDGE_OPEN     125
#define SYS_SETREUID        126
//#define SYS_SETREUID        127 duplicate
#define SYS_RENAME          128
#define SYS_FLOCK           131
#define SYS_MKFIFO          132
#define SYS_SENDTO          133 //done
#define SYS_SHUTDOWN        134 //done
#define SYS_SOCKETPAIR      135
#define SYS_MKDIR           136 //done
#define SYS_RMDIR           137 //done
#define SYS_ADJTIME         140
#define SYS_GETLOGIN_R      141
#define SYS_GETTHRNAME      142
#define SYS_SETTHRNAME      143
#define SYS_SETSID          147
#define SYS_QUOTACTL        148
#define SYS_YPCONNECT       150
#define SYS_NFSSVC          155
#define SYS_PINSYSCALLS     158
#define SYS_MIMMUTABLE      159
#define SYS_WAITID          160
#define SYS_GETFH           161
#define SYS_TMPFD           164
#define SYS_SYSARCH         165
#define SYS_LSEEK           166 //done
#define SYS_TRUNCATE        167
#define SYS_FTRUNCATE       168
#define SYS_PREAD           169
#define SYS_PWRITE          170
#define SYS_PREADV          171
#define SYS_PWRITEV         172
#define SYS_PROFIL          175
#define SYS_SETGID          181
#define SYS_SETEGID         182
#define SYS_SETEUID         183
#define SYS_PATHCONFAT      190
#define SYS_PATHCONF        191
#define SYS_FPATHCONF       192
#define SYS_SWAPCTL         193
#define SYS_GETRLIMIT       194
#define SYS_SETRLIMIT       195
#define SYS_SYSCTL          202
#define SYS_MLOCK           203
#define SYS_MUNLOCK         204
#define SYS_GETPGID         207
#define SYS_UTRACE          209
#define SYS_SEMGET          221 //done
#define SYS_MSGGET          225 //done
#define SYS_MSGSND          226 //done
#define SYS_MSGRCV          227 //done
#define SYS_SHMAT           228 //done
#define SYS_SHMDT           230 //done
#define SYS_MINHERIT        250
#define SYS_POLL            252 //done
#define SYS_ISSETUGIT       253
#define SYS_LCHOWN          254
#define SYS_GETSID          255
#define SYS_MSYNC           256
#define SYS_PIPE            263 //done
#define SYS_FHOPEN          264
#define SYS_KQUEUE          269
#define SYS_KQUEUE1         270
#define SYS_MLOCKALL        271
#define SYS_MUNLOCKALL      272
#define SYS_GETRESUID       281
#define SYS_SETRESUID       282
//#define SYS_GETRESUID       283 duplicate
#define SYS_SETRESGID       284
#define SYS_CLOSEFROM       287
#define SYS_SIGNALTSTACK    288
#define SYS_SHMGET          289
#define SYS_SEMOP           290 //done
#define SYS_FHSTAT          294
#define SYS_SEMCTL          295
#define SYS_SHMCTL          296 //dones
#define SYS_MSGCTL          297 //done
#define SYS_SCHED_YIELD     298 //done
#define SYS_GETTHRID        299
#define SYS_THRWAKEUP       301
#define SYS_THREXIT         302
#define SYS_THRSIGDIVERT    303
#define SYS_GETCWD          304
#define SYS_ADJFREQ         305
#define SYS_SETRTABLE       310
#define SYS_GETRTABLE       311
#define SYS_FACCESSAT       313
#define SYS_FCHMODAT        314
#define SYS_FCHOWNAT        315
#define SYS_LINKAT          317
#define SYS_MKDIRAT         318
#define SYS_MKFIFOAT        319
#define SYS_MKNODAT         320
#define SYS_OPENAT          321
#define SYS_READLINKAT      322
#define SYS_RENAMEAT        323
#define SYS_SYMLINKAT       324
#define SYS_UNLINKAT        325
#define SYS_SET_TCB         329
#define SYS_GET_TCB         330
#define SYS_GETIFADDRS      331
#define SYS_MQ_OPEN         332
#define SYS_MQ_CLOSE        333
#define SYS_MQ_UNLINK       334
#define SYS_MQ_SEND         335
#define SYS_MQ_RECEIVE      336
#define SYS_GETHOSTBYNAME   337
#define SYS_SCHED_SETSCHEDULER  338
#define SYS_SCHED_GETSCHEDULER  339
#define SYS_EXECVE          340
//#define SYS_EXECVE          341 //duplicate
//#define SYS_CHDIR           342 duplicate
#define SYS_UTIME           343
#define SYS_STST            344
//#define SYS_CLOCK_GETTIME   345 duplicate
#define SYS_TIMES           346
#define SYS_UNAME           347
#define SYS_MAXSYSCALL      348

/* ── Additional syscall numbers needed by uix_posix_*.c ──────── */
#ifndef SYS_VFORK
#define SYS_VFORK           66
#endif
#ifndef SYS_DUP3
#define SYS_DUP3            292
#endif
#ifndef SYS_PIPE2
#define SYS_PIPE2           293
#endif
#ifndef SYS_READV
#define SYS_READV           19
#endif
#ifndef SYS_WRITEV
#define SYS_WRITEV          20
#endif
#ifndef SYS_PREAD
#define SYS_PREAD           180
#endif
#ifndef SYS_PWRITE
#define SYS_PWRITE          181
#endif
#ifndef SYS_TRUNCATE
#define SYS_TRUNCATE        92
#endif
#ifndef SYS_FTRUNCATE
#define SYS_FTRUNCATE       93
#endif
#ifndef SYS_FDATASYNC
#define SYS_FDATASYNC       148
#endif
#ifndef SYS_SYNC
#define SYS_SYNC            36
#endif
#ifndef SYS_IOCTL
#define SYS_IOCTL           54
#endif
#ifndef SYS_FSTAT
#define SYS_FSTAT           108
#endif
#ifndef SYS_LSTAT
#define SYS_LSTAT           107
#endif
#ifndef SYS_FSTATAT
#define SYS_FSTATAT         300
#endif
#ifndef SYS_MKDIRAT
#define SYS_MKDIRAT         296
#endif
#ifndef SYS_RENAMEAT
#define SYS_RENAMEAT        302
#endif
#ifndef SYS_LINKAT
#define SYS_LINKAT          303
#endif
#ifndef SYS_UNLINKAT
#define SYS_UNLINKAT        301
#endif
#ifndef SYS_SYMLINKAT
#define SYS_SYMLINKAT       304
#endif
#ifndef SYS_READLINKAT
#define SYS_READLINKAT      305
#endif
#ifndef SYS_FCHMODAT
#define SYS_FCHMODAT        307
#endif
#ifndef SYS_FCHOWNAT
#define SYS_FCHOWNAT        298
#endif
#ifndef SYS_FACCESSAT
#define SYS_FACCESSAT       307
#endif
#ifndef SYS_MKNODAT
#define SYS_MKNODAT         297
#endif
#ifndef SYS_GETDENTS
#define SYS_GETDENTS        78
#endif
#ifndef SYS_GETPPID
#define SYS_GETPPID         64
#endif
#ifndef SYS_GETUID
#define SYS_GETUID          24
#endif
#ifndef SYS_GETEUID
#define SYS_GETEUID         49
#endif
#ifndef SYS_GETGID
#define SYS_GETGID          47
#endif
#ifndef SYS_GETEGID
#define SYS_GETEGID         50
#endif
#ifndef SYS_SETUID
#define SYS_SETUID          23
#endif
#ifndef SYS_SETGID
#define SYS_SETGID          46
#endif
#ifndef SYS_SETEUID
#define SYS_SETEUID         183
#endif
#ifndef SYS_SETEGID
#define SYS_SETEGID         184
#endif
#ifndef SYS_SETPGID
#define SYS_SETPGID         57
#endif
#ifndef SYS_GETPGID
#define SYS_GETPGID         132
#endif
#ifndef SYS_SETSID
#define SYS_SETSID          66
#endif
#ifndef SYS_GETSID
#define SYS_GETSID          147
#endif
#ifndef SYS_SIGACTION
#define SYS_SIGACTION       67
#endif
#ifndef SYS_SIGPROCMASK
#define SYS_SIGPROCMASK     126
#endif
#ifndef SYS_ALARM
#define SYS_ALARM           27
#endif
#ifndef SYS_PAUSE
#define SYS_PAUSE           29
#endif
#ifndef SYS_GETRLIMIT
#define SYS_GETRLIMIT       97
#endif
#ifndef SYS_SETRLIMIT
#define SYS_SETRLIMIT       75
#endif
#ifndef SYS_FCHDIR
#define SYS_FCHDIR          133
#endif
#ifndef SYS_CHROOT
#define SYS_CHROOT          61
#endif
#ifndef SYS_GETCWD
#define SYS_GETCWD          183
#endif
#ifndef SYS_UMASK
#define SYS_UMASK           60
#endif
#ifndef SYS_NICE
#define SYS_NICE            34
#endif
#ifndef SYS_MLOCK
#define SYS_MLOCK           150
#endif
#ifndef SYS_MUNLOCK
#define SYS_MUNLOCK         151
#endif
#ifndef SYS_MSYNC
#define SYS_MSYNC           144
#endif
#ifndef SYS_MREMAP
#define SYS_MREMAP          163
#endif
#ifndef SYS_BRK
#define SYS_BRK             45
#endif
#ifndef SYS_CLOCK_GETTIME
#define SYS_CLOCK_GETTIME   228
#endif
#ifndef SYS_CLOCK_SETTIME
#define SYS_CLOCK_SETTIME   227
#endif
#ifndef SYS_CLOCK_GETRES
#define SYS_CLOCK_GETRES    229
#endif
#ifndef SYS_NANOSLEEP
#define SYS_NANOSLEEP       162
#endif
#ifndef SYS_GETTIMEOFDAY
#define SYS_GETTIMEOFDAY    78
#endif
#ifndef SYS_SETTIMEOFDAY
#define SYS_SETTIMEOFDAY    79
#endif
#ifndef SYS_ACCEPT4
#define SYS_ACCEPT4         288
#endif
#ifndef SYS_SHUTDOWN
#define SYS_SHUTDOWN        293
#endif


/* ── All sys_* stubs are static inline so that every .c that
 * includes this header gets its own private copy.  Without
 * static the linker sees N definitions of the same symbol
 * (one per translation unit) and emits "duplicate symbol".
 * ---------------------------------------------------------- */
/* ════════════════════════════════════════════════════════════
   FILE-SYSTEM SYSCALLS  →  32_FileSystem/10_scfs/include/fs.h
   ════════════════════════════════════════════════════════════ */
   static inline int sys_open(const char *path, int flags, ...)
   {
       return my_syscall2(SYS_OPEN, (long)(path), (long)(flags));
   }
   /* POSIX creat.  NOT WIRED -- no SYS_CREAT number on either side, and
    * nothing calls it (sys_open covers the callers).  Stub by intent. */
   static inline int sys_creat(const char *path, uix_mode_t mode)
   {
       (void)path; (void)mode;
       return -1;
   }
   static inline int sys_close(int fd)
   {
       return my_syscall1(SYS_CLOSE, (long)(fd));
   }
   static inline uix_ssize_t sys_read(int fd, void *buf, uix_size_t count)
   {
       return (uix_ssize_t)my_syscall3(SYS_READ, (long)(fd), (long)(buf), (long)(count));
   }
   static inline uix_ssize_t sys_write(int fd, const void *buf, uix_size_t count)
   {
       return (uix_ssize_t)my_syscall3(SYS_WRITE, (long)(fd), (long)(buf), (long)(count));
   }
   static inline uix_off_t sys_lseek(int fd, uix_off_t off, int whence)
   {
       return (uix_off_t)my_syscall3(SYS_LSEEK, (long)(fd), (long)(off), (long)(whence));
   }
   static inline int sys_fcntl(int fd, int cmd, ...)
   {
       return my_syscall2(SYS_FCNTL, (long)(fd), (long)(cmd));
   }
   static inline int sys_dup(int fd)
   {
       return my_syscall1(SYS_DUP, (long)(fd));
   }
   static inline int sys_dup2(int oldfd, int newfd)
   {
       return my_syscall2(SYS_DUP2, (long)(oldfd), (long)(newfd));
   }
   static inline int sys_pipe(int pipefd[2])
   {
       return my_syscall1(SYS_PIPE, (long)(pipefd));
   }
   static inline int sys_stat(const char *path, uix_stat_t *buf)
   {
       return my_syscall2(SYS_STAT, (long)(path), (long)(buf));
   }
   static inline int sys_fstat(int fd, uix_stat_t *buf)
   {
       return my_syscall2(SYS_FSTAT, (long)(fd), (long)(buf));
   }
   static inline int sys_lstat(const char *path, uix_stat_t *buf)
   {
       return my_syscall2(SYS_LSTAT, (long)(path), (long)(buf));
   }
   static inline int sys_chmod(const char *path, uix_mode_t mode)
   {
       return my_syscall2(SYS_CHMOD, (long)(path), (long)(mode));
   }
   static inline int sys_chown(const char *path,
                                 uix_uid_t owner, uix_gid_t group)
   {
       return my_syscall3(SYS_CHOWN, (long)(path), (long)(owner), (long)(group));
   }
   static inline int sys_mkdir(const char *path, uix_mode_t mode)
   {
       return my_syscall2(SYS_MKDIR, (long)(path), (long)(mode));
   }
   static inline int sys_mknod(const char *path,
                                 uix_mode_t mode, uix_dev_t dev)
   {
       return (int)my_syscall4(SYS_MKNOD, (long)(path),
                             (long)(mode),
                             (long)(((dev)>>8) & 0xFF),
                             (long)((dev) & 0xFF));
   }
   static inline int sys_unlink(const char *path)
   {
       return my_syscall1(SYS_UNLINK, (long)(path));
   }
   static inline int sys_link(const char *old, const char *nw)
   {
       return my_syscall2(SYS_LINK, (long)(old), (long)(nw));
   }
   static inline int sys_rmdir(const char *path)
   {
       return my_syscall1(SYS_RMDIR, (long)(path));
   }
   static inline int sys_chdir(const char *path)
   {
       return my_syscall1(SYS_CHDIR, (long)(path));
   }
   static inline int sys_chroot(const char *path)
   {
       return my_syscall1(SYS_CHROOT, (long)(path));
   }
   static inline int sys_mount(const char *special,
                                 const char *dir, int flags)
   {
       return my_syscall3(SYS_MOUNT, (long)(special), (long)(dir), (long)(flags));
   }
   static inline int sys_umount(const char *special)
   {
       return my_syscall1(SYS_UNMOUNT, (long)(special));
   }
   static inline int sys_getdents(int fd, void *dirp, int count)
   {
       return my_syscall3(SYS_GETDENTS, (long)(fd), (long)(dirp), (long)(count));
   }
/* ════════════════════════════════════════════════════════════
   PROCESS SYSCALLS  →  33_ProcessControlSubsystem/50_scps/
   ════════════════════════════════════════════════════════════ */
   static inline uix_pid_t sys_fork(void)
   {
       return (uix_pid_t)my_syscall0(SYS_FORK);
   }
   static inline uix_pid_t sys_getpid(void)
   {
       return (uix_pid_t)my_syscall0(SYS_GETPID);
   }
   static inline uix_pid_t sys_getppid(void)
   {
       return (uix_pid_t)my_syscall0(SYS_GETPPID);
   }
   static inline void sys_exit(int status)
   {
       (void)my_syscall1(SYS_EXIT, (long)(status));
   }
   static inline int sys_execv(const char *p, char *const av[])
   {
       return my_syscall2(SYS_EXECV, (long)(p), (long)(av));
   }
   static inline int sys_execve(const char *p,
                                  char *const av[], char *const env[])
   {
       return my_syscall3(SYS_EXECVE, (long)(p), (long)(av), (long)(env));
   }
   static inline int sys_kill(uix_pid_t pid, int sig)
   {
       return my_syscall2(SYS_KILL, (long)(pid), (long)(sig));
   }
   static inline uix_pid_t sys_wait4(uix_pid_t pid, int *wstatus,
                                       int options, void *rusage)
   {
       return (uix_pid_t)my_syscall4(SYS_WAIT4, (long)(pid), (long)(wstatus), (long)(options), (long)(rusage));
   }
/* ════════════════════════════════════════════════════════════
   SIGNAL SYSCALLS  →  33_ProcessControlSubsystem/50_scps/signal.h
   ════════════════════════════════════════════════════════════ */
typedef void (*sig_handler_t)(int);

/* Handler type for sys_signal().  The kernel's entry point is

 * uiox_kix_scpcs_signal's sig_action shape; this is its userspace face. */
static inline int sys_signal(int signum, sig_handler_t handler)
{
    return my_syscall2(SYS_SIGACTION, (long)(signum), (long)(handler));
}
static inline int sys_sigaction(int signum,
                                  const uix_sigaction_t *act,
                                  uix_sigaction_t *oldact)
{
    return my_syscall3(SYS_SIGACTION, (long)(signum), (long)(act), (long)(oldact));
}

/* ════════════════════════════════════════════════════════════
   TIME SYSCALLS
   ════════════════════════════════════════════════════════════ */
static inline uix_time_t sys_time(uix_time_t *tloc)
{
    return (uix_time_t)my_syscall1(SYS_TIMES, (long)(tloc));
}
static inline int sys_gettimeofday(uix_timeval_t *tv, void *tz)
{
    return my_syscall2(SYS_GETTIMEOFDAY, (long)(tv), (long)(tz));
}
static inline int sys_clock_gettime(int clkid, uix_timespec_t *tp)
{
    return my_syscall2(SYS_CLOCK_GETTIME, (long)(clkid), (long)(tp));
}
static inline uix_clock_t sys_times(uix_tms_t *buf)
{
    return (uix_clock_t)my_syscall1(SYS_TIMES, (long)(buf));
}
static inline int sys_utime(const char *path,
                            const uix_utimbuf_t *times)
{
    return my_syscall2(SYS_UTIME, (long)(path), (long)(times));
}
/* ════════════════════════════════════════════════════════════
   UTSNAME
   ════════════════════════════════════════════════════════════ */
static inline int sys_uname(uix_utsname_t *buf)
{
    return my_syscall1(SYS_UNAME, (long)(buf));
}
/* ════════════════════════════════════════════════════════════
   MEMORY MANAGEMENT  →  33_ProcessControlSubsystem/02_memory-managment/
   ════════════════════════════════════════════════════════════ */
static inline void *sys_mmap(void *addr, uix_size_t length,
                    int prot, int flags, int fd, uix_off_t offset)
{
    return (void *)my_syscall6(SYS_MMAP, (long)(addr), (long)(length), (long)(prot), (long)(flags), (long)(fd), (long)(offset));
}
static inline int sys_munmap(void *addr, uix_size_t length)
{
    return my_syscall2(SYS_MUNMAP, (long)(addr), (long)(length));
}
static inline int sys_mprotect(void *addr, uix_size_t len, int prot)
{
    return my_syscall3(SYS_MPROTECT, (long)(addr), (long)(len), (long)(prot));
}
static inline uix_uintptr_t sys_brk(uix_uintptr_t newbrk)
{
    return (uix_uintptr_t)my_syscall1(SYS_BREAK, (long)(newbrk));
}
/* POSIX shm.  NOT WIRED -- there is no SYS_SHM_OPEN number on either side
 * of the boundary, and no kernel entry point.  Nothing calls this today:
 * the apps use the System V family (sys_shmat/shmdt/shmget/shmctl), which
 * IS wired.  Kept as a compiling stub so the header stays self-consistent;
 * when a caller appears, assign the number in this file's table AND add the
 * dispatcher entry in 30_KIX before converting the body to my_syscall3. */
static inline int sys_shm_open(const char *name,
                               int oflag, uix_mode_t mode)
{
    (void)name; (void)oflag; (void)mode;
    return -1;
}

/* POSIX shm.  NOT WIRED -- see sys_shm_open above. */
static inline int sys_shm_unlink(const char *name)
{
    (void)name;
    return -1;
}/*End of ememory mamanagment*/
/* ════════════════════════════════════════════════════════════
   SOCKET / NETWORK  →  33_ProcessControlSubsystem/00_ipc/
   ════════════════════════════════════════════════════════════ */
   //uiox_socket.h
static inline int sys_socket(int domain, int type, int protocol)
{
    return my_syscall3(SYS_SOCKET, (long)(domain), (long)(type), (long)(protocol));
}
static inline int sys_bind(int sockfd, const uix_sockaddr_t *addr,
                                uix_socklen_t addrlen)
{
    return my_syscall3(SYS_BIND, (long)(sockfd), (long)(addr), (long)(addrlen));
}
static inline int sys_listen(int sockfd, int backlog)
{
    return my_syscall2(SYS_LISTEN, (long)(sockfd), (long)(backlog));
}
static inline int sys_accept(int sockfd, uix_sockaddr_t *addr,
                                  uix_socklen_t *addrlen)
{
    return my_syscall3(SYS_ACCEPT, (long)(sockfd), (long)(addr), (long)(addrlen));
}
static inline int sys_connect(int sockfd,
            const uix_sockaddr_t *addr,uix_socklen_t addrlen)
{
    return my_syscall3(SYS_CONNECT, (long)(sockfd), (long)(addr), (long)(addrlen));
}
static inline int sys_shutdown(int sockfd, int how)
{
    return my_syscall2(SYS_SHUTDOWN, (long)(sockfd), (long)(how));
}
static inline uix_ssize_t sys_sendto(int sockfd, const void *buf,
                                          uix_size_t len, int flags,
                                          const uix_sockaddr_t *dest,
                                          uix_socklen_t addrlen)
{
    return (uix_ssize_t)my_syscall6(SYS_SENDTO, (long)(sockfd), (long)(buf), (long)(len), (long)(flags), (long)(dest), (long)(addrlen));
}
static inline uix_ssize_t sys_recvfrom(int sockfd, void *buf,
                                            uix_size_t len, int flags,
                                            uix_sockaddr_t *src,
                                            uix_socklen_t *addrlen)
{
    return (uix_ssize_t)my_syscall6(SYS_RECVFROM, (long)(sockfd), (long)(buf), (long)(len), (long)(flags), (long)(src), (long)(addrlen));
}
static inline int sys_getsockopt(int sockfd, int level, int optname,
                                      void *optval, uix_socklen_t *optlen)
{
    return my_syscall5(SYS_GETSOCKOPT, (long)(sockfd), (long)(level), (long)(optname), (long)(optval), (long)(optlen));
}
static inline int sys_setsockopt(int sockfd, int level, int optname,
                                    const void *optval, uix_socklen_t optlen)
{
    return my_syscall5(SYS_SETSOCKOPT, (long)(sockfd), (long)(level), (long)(optname), (long)(optval), (long)(optlen));
}
static inline int sys_getsockname(int sockfd, uix_sockaddr_t *addr,
                                    uix_socklen_t *addrlen)
{
    return my_syscall3(SYS_GETSOCKNAME, (long)(sockfd), (long)(addr), (long)(addrlen));
}
static inline int sys_getpeername(int sockfd, uix_sockaddr_t *addr,
                                       uix_socklen_t *addrlen)
{
    return my_syscall3(SYS_GETPEERNAME, (long)(sockfd), (long)(addr), (long)(addrlen));
}
/* ════════════════════════════════════════════════════════════
   I/O MULTIPLEXING
   ════════════════════════════════════════════════════════════ */
static inline int sys_select(int nfds, uix_fd_set *rfds, uix_fd_set *wfds,
                                uix_fd_set *efds, uix_timeval_t *timeout)
{
    return my_syscall5(SYS_SELECT, (long)(nfds), (long)(rfds), (long)(wfds), (long)(efds), (long)(timeout));
}
static inline int sys_poll(uix_pollfd_t *fds,
                            uix_nfds_t nfds, int timeout)
{
    return my_syscall3(SYS_POLL, (long)(fds), (long)(nfds), (long)(timeout));
}
static inline int sys_ioctl(int fd, unsigned long request, ...)
{
    return my_syscall2(SYS_IOCTL, (long)(fd), (long)(request));
}
/* ════════════════════════════════════════════════════════════
   SCHEDULER  →  33_ProcessControlSubsystem/01_schedular/
   ════════════════════════════════════════════════════════════ */
static inline int sys_sched_setscheduler(uix_pid_t pid, int policy,
                                            const uix_sched_param_t *p)
{
    return my_syscall3(SYS_SCHED_SETSCHEDULER, (long)(pid), (long)(policy), (long)(p));
}
static inline int sys_sched_getscheduler(uix_pid_t pid)
{
    return my_syscall1(SYS_SCHED_GETSCHEDULER, (long)(pid));
}
static inline int sys_sched_yield(void)
{
    return my_syscall0(SYS_SCHED_YIELD);
}
/* ════════════════════════════════════════════════════════════
   IPC: MSG / SHM / SEM  →  33_ProcessControlSubsystem/00_ipc/
   ════════════════════════════════════════════════════════════ */
static inline int sys_msgget(uix_key_t key, int msgflg)
{
    return my_syscall2(SYS_MSGGET, (long)(key), (long)(msgflg));
}
static inline int sys_msgsnd(int msqid, const void *msgp,
                                uix_size_t msgsz, int msgflg)
{
    return my_syscall4(SYS_MSGSND, (long)(msqid), (long)(msgp), (long)(msgsz), (long)(msgflg));
}
static inline uix_ssize_t sys_msgrcv(int msqid, void *msgp,
                    uix_size_t msgsz,long msgtyp, int msgflg)
{
    return (uix_ssize_t)my_syscall5(SYS_MSGRCV, (long)(msqid), (long)(msgp), (long)(msgsz), (long)(msgtyp), (long)(msgflg));
}
static inline int sys_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
{
    return my_syscall3(SYS_MSGCTL, (long)(msqid), (long)(cmd), (long)(buf));
}
static inline int sys_shmget(uix_key_t key, uix_size_t size, int shmflg)
{
    return my_syscall3(SYS_SHMGET, (long)(key), (long)(size), (long)(shmflg));
}
static inline void *sys_shmat(int shmid,
                                const void *shmaddr, int shmflg)
{
    return (void *)my_syscall3(SYS_SHMAT, (long)(shmid), (long)(shmaddr), (long)(shmflg));
}
static inline int sys_shmdt(const void *shmaddr)
{
    return my_syscall1(SYS_SHMDT, (long)(shmaddr));
}
static inline int sys_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
{
    return my_syscall3(SYS_SHMCTL, (long)(shmid), (long)(cmd), (long)(buf));
}
static inline int sys_semget(uix_key_t key, int nsems, int semflg)
{
    return my_syscall3(SYS_SEMGET, (long)(key), (long)(nsems), (long)(semflg));
}
static inline int sys_semop(int semid,
                            uix_sembuf_t *sops, uix_size_t nsops)
{
    return my_syscall3(SYS_SEMOP, (long)(semid), (long)(sops), (long)(nsops));
}
static inline int sys_semctl(int semid, int semnum, int cmd, ...)
{
    return my_syscall3(SYS_SEMCTL, (long)(semid), (long)(semnum), (long)(cmd));
}
/* ════════════════════════════════════════════════════════════
   NETWORK HELPERS
   ════════════════════════════════════════════════════════════ */
static inline int sys_getifaddrs(uix_ifaddrs_t **ifap)
{
    return (int)my_syscall1(SYS_GETIFADDRS, (long)(ifap));
}
static inline uix_hostent_t *sys_gethostbyname(const char *name)
{
    return (uix_hostent_t *)my_syscall1(SYS_GETHOSTBYNAME, (long)(name));
}
/* ════════════════════════════════════════════════════════════
   MQUEUE  →  33_ProcessControlSubsystem/00_ipc/
   ════════════════════════════════════════════════════════════ */
static inline uix_mqd_t sys_mq_open(const char *name, int oflag, ...)
{
    return (uix_mqd_t)my_syscall2(SYS_MQ_OPEN, (long)(name), (long)(oflag));
}
static inline int sys_mq_close(uix_mqd_t mqdes)
{
    return my_syscall1(SYS_MQ_CLOSE, (long)(mqdes));
}
static inline int sys_mq_unlink(const char *name)
{
    return my_syscall1(SYS_MQ_UNLINK, (long)(name));
}
static inline int sys_mq_send(uix_mqd_t mqdes, const char *msg_ptr,
                                uix_size_t msg_len, unsigned int msg_prio)
{
    return my_syscall4(SYS_MQ_SEND, (long)(mqdes), (long)(msg_ptr), (long)(msg_len), (long)(msg_prio));
}
static inline uix_ssize_t sys_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
                                uix_size_t msg_len,unsigned int *msg_prio)
{
    return (uix_ssize_t)my_syscall4(SYS_MQ_RECEIVE, (long)(mqdes), (long)(msg_ptr), (long)(msg_len), (long)(msg_prio));
}
/* ******** End system call here for core kernel  ***************** */


/* ******** Start system call here for drivers kernel ***************** */
/*
1. Ethernet
2. GPU
3. HDMI
4. Monitor
5. USB
6. Camera
7. Keyboard
8. Monitor
9. Mouse
10. TouchPassword
11. WiFi
12. Speaker
13. BMS
14. FAN
15. MIC
16. PMIC
17. Thermal

*/
//#if SYS_CALL_CAS_ENBLE

// Ethernet
//Mentioned above for SOCKET / NETWORK  →  33_ProcessControlSubsystem/00_ipc/

// GPU
//uiox_gpu_subsys_init
//uiox_gpu_subsys_deinit
//uiox_gpu_subsys_create_pso
//uiox_gpu_subsys_destroy_pso
//uiox_gpu_subsys_begin_frame
//uiox_gpu_subsys_begin_pass
//uiox_gpu_subsys_end_pass
//uiox_gpu_subsys_bind_pso
//uiox_gpu_subsys_end_frame
//uiox_gpu_subsys_frame_stats
//GPU end here

// HDMI
// Monitor
// USB
// Camera
// Keyboard
// Monitor
// Mouse
// TouchPassword
// WiFi
// Speaker
// BMS
// FAN
// MIC
// PMIC
// Thermal

//#endif/* SYS_CALL_CAS_ENBLE */




#endif /* __UIX_SYS__H */