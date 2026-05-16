#ifndef __UIX_SYS__H
#define __UIX_SYS__H

#include "sys/uix_types.h"

#define STUB 1// 0:STUB, 1:SYS funtinal call to Kernal


#define SYS_EXIT            1 //done
#define SYS_FORK            2 //done
#define SYS_READ            3 //done
#define SYS_WRITE           4 //done
#define SYS_OPEN            5   // done
#define SYS_CLOSE           6 //done
#define SYS_GETENTROPY      7
#define SYS_TFORK         8
#define SYS_LINK            9 //done
#define SYS_UNLINK         10 //done
#define SYS_WAIT4           11//done
#define sys_CHDIR           12 //done
#define SYS_FCHDIR          13
#define SYS_MKNOD           14//done
#define SYS_CHMOD           15 //done
#define SYS_CHOWN           16//done
#define SYS_BREAK           17
#define SYS_GETDTABLECOUNT  18
#define SYS_GETRUSAGE       19
#define SYS_GETPID         20 //done
#define SYS_MOUNT           21
#define SYS_UNMOUNT         22
#define SYS_SETUID          23
#define SYS_GETUID          24
#define SYS_GETEUID         25
#define SYS_PTRACE          26
#define SYS_RECVMSG         27
#define SYS_SENDMSG         28
#define SYS_RECVFROM       29//done
#define SYS_ACCEPT          30 //done
#define SYS_GETPEERNAME     31 //done
#define SYS_GETSOCKNAME     32 //done
#define SYS_ACCESS         33
#define SYS_CHFLAGS        34
#define SYS_FCHFLAGS        35
#define SYS_SYNC           36
#define SYS_STAT           38 //done
#define SYS_GETPID         39 //done
#define SYS_LSTAT          40//done
#define SYS_DUP             41 //done
#define SYS_FSTATAT         42
#define SYS_GETEPID         43
#define SYS_KTRACE        45
#define SYS_SIGACTION       46
#define SYS_GETGID         47
#define SYS_SIGPROCMASK     48
#define SYS_MMAP           49
#define SYS_SETLOGIN       50
#define SYS_ACCT            51
#define SYS_SIGPENDING      52
#define SYS_FSTAT         53 //done
#define SYS_IOCTL          54 //done
#define SYS_REBOOT          55
#define SYS_REVOKE        56
#define SYS_SYMLINK         57
#define SYS_READLINK        58
#define sys_EXECVE          59
#define SYS_EXECV          250 //done
#define SYS_UMASK           60
#define SYS_CHROOT          61 //done
#define SYS_GETFSSTAT      62
#define SYS_STATFS          63
#define SYS_FSTATFS         64
#define SYS_FHSTATFS        65
#define SYS_VFORK        66
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
#define SYS_GETGROUPS      79
#define SYS_SETGROUPS      80
#define SYS_GETPGRP         81
#define SYS_SETPPID         82
#define SYS_FUTEX           83
#define SYS_UTIMENSAT       84
#define SYS_FUTIMENS        85
#define SYS_KBIND           86
#define sys_CLOCK_GETTIME   87 //done
#define SYS_CLOCK_SETTIME  88
#define SYS_CLOCK_GETRES   89
#define SYS_DUP2            90
#define SYS_NANOSLEEP       91
#define sys_FCNTL           92 // done
#define SYS_ACCEPT4        93
#define SYS_THRSLEEP      94
#define SYS_FSYNC          95
#define SYS_SETPRIORITY     96
#define SYS_SOCKET         97//done
#define SYS_CONNECT         98 //done
#define SYS_GETDENTS        99 //done
#define SYS_GETPRIORITY     100
#define SYS_PIPE2           101
#define SYS_DUP3            102
#define SYS_SIGRETURN      103
#define SYS_BIND            104 //done
#define SYS_SETSOCKOPT     105 //done
#define SYS_LISTEN          106 //done
#define SYS_CHFLAGSAT       107
#define SYS_PLEDGE         108
#define SYS_PPOLL          109
#define SYS_PSELECT         110
#define SYS_SIGSUSPEND      111
#define SYS_SENDSYSLOG     112
#define SYS_UNVEIL         114
#define SYS_REALPATH    115
#define SYS_RECVMMSG       116
#define SYS_SENDMMSG       117
#define SYS_GETSOCKOPT     118 //done
#define SYS_THRKILL        119
#define SYS_READV           120
#define SYS_WRITEV          121
#define SYS_KILL            122 //done
#define SYS_FCHOWN        123
#define SYS_FCHMOD          124
#define SYS_PLEDGE_OPEN   125
#define SYS_SETREUID       126
#define SYS_SETREUID       127
#define SYS_RENAME         128
#define SYS_FLOCK           131
#define SYS_MKFIFO          132
#define SYS_SENDTO          133 //done
#define SYS_SHUTDOWN        134 //done
#define SYS_SOCKETPAIR      135
#define SYS_MKDIR          136 //done
#define SYS_RMDIR           137 //done
#define SYS_ADJTIME         140
#define SYS_GETLOGIN_R      141
#define SYS_GETTHRNAME      142
#define SYS_SETTHRNAME     143
#define SYS_SETSID          147
#define SYS_QUOTACTL        148
#define SYS_YPCONNECT       150
#define SYS_NFSSVC          155
#define SYS_PINSYSCALLS    158
#define SYS_MIMMUTABLE     159
#define SYS_WAITID         160
#define SYS_GETFH           161
#define SYS_TMPFD         164
#define SYS_SYSARCH         165
#define SYS_LSEEK          166 //done
#define SYS_TRUNCATE        167
#define SYS_FTRUNCATE        168
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
#define SYS_GETPGID          207
#define SYS_UTRACE          209
#define SYS_SEMGET          221 //done
#define SYS_MSGGET          225 //done
#define SYS_MSGSND          226 //done
#define SYS_MSGRCV          227 //done
#define SYS_SHMAT           228 //done
#define SYS_SHMDT           230 //done
#define SYS_MINHERIT        250
#define SYS_POLL            252 //done
#define SYS_ISSETUGIT      253
#define SYS_LCHOWN          254
#define SYS_GETSID          255
#define SYS_MSYNC           256
#define SYS_PIPE            263 //done
#define SYS_FHOPEN          264
#define SYS_KQUEUE          269
#define SYS_KQUEUE1        270
#define SYS_MLOCKALL       271
#define SYS_MUNLOCKALL     272
#define SYS_GETRESUID      281
#define SYS_SETRESUID       282
#define SYS_GETRESUID       283
#define SYS_SETRESGID       284
#define SYS_CLOSEFROM       287
#define SYS_SIGNALTSTACK     288
#define SYS_SHMGET          289
#define SYS_SEMOP           290 //done
#define SYS_FHSTAT          294
#define SYS_SEMCTL        295
#define SYS_SHMCTL         296 //dones
#define SYS_MSGCTL          297 //done
#define SYS_SCHED_YIELD     298 //done
#define SYS_GETTHRID        299
#define SYS_THRWAKEUP     301
#define SYS_THREXIT       302
#define SYS_THRSIGDIVERT  303
#define SYS_GETCWD        304
#define SYS_ADJFREQ         305
#define SYS_SETRTABLE       310
#define SYS_GETRTABLE       311
#define SYS_FACCESSAT       313
#define SYS_FCHMODAT        314
#define SYS_FCHOWNAT        315
#define SYS_LINKAT         317
#define SYS_MKDIRAT         318
#define SYS_mkfifoat        319
#define SYS_MKNODAT         320
#define SYS_OPENAT          321
#define SYS_READLINKAT      322
#define SYS_RENAMEAT        323
#define SYS_SYMLINKAT       324
#define SYS_UNLINKAT        325
#define SYS_SET_TCB       329
#define SYS_GET_TCB       330
#define SYS_MAXSYSCALL      331


/* ── sys-call stub selector ──────────────────────────────────
 * 0 = all stubs return 0 (simulation / hosted build)
 * 1 = real syscall wrappers (bare-metal / kernel build)
 * ---------------------------------------------------------- */
#define SYS_CALL_ENBLE_DISABLE 0

/* ── All sys_* stubs are static inline so that every .c that
 * includes this header gets its own private copy.  Without
 * static the linker sees N definitions of the same symbol
 * (one per translation unit) and emits "duplicate symbol".
 * ---------------------------------------------------------- */

#if 1 /* file-system calls */
#include "PoStd/uix_fcntl.h"
#include "sys/uix_stat.h"
#include "PoStd/uix_dirent.h"

static inline int sys_open(const char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    return -1;
}

static inline int sys_creat(const char *path, uix_mode_t mode) // not macro added
{
    (void)path;
    (void)mode;
    return -1;
}

static inline int sys_close(int fd)
{
    (void)fd;
    return 0;
}

static inline uix_ssize_t sys_read(int fd, void *buf, uix_size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

static inline uix_ssize_t sys_write(int fd, const void *buf, uix_size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
}

static inline uix_off_t sys_lseek(int fd, uix_off_t off, int whence)
{
    (void)fd;
    (void)off;
    (void)whence;
    return -1;
}

static inline int sys_fcntl(int fd, int cmd, ...)
{
    (void)fd;
    (void)cmd;
    return -1;
}

static inline int sys_dup(int fd)
{
    (void)fd;
    return -1;
}

static inline int sys_dup2(int old, int nw)
{
    (void)old;
    (void)nw;
    return -1;
}

static inline int sys_pipe(int pipefd[2])
{
    (void)pipefd;
    return -1;
}

static inline int sys_stat(const char *path, uix_stat_t *buf)
{
    (void)path;
    (void)buf;
    return -1;
}

static inline int sys_fstat(int fd, uix_stat_t *buf)
{
    (void)fd;
    (void)buf;
    return -1;
}

static inline int sys_lstat(const char *path, uix_stat_t *buf)
{
    (void)path;
    (void)buf;
    return -1;
}

static inline int sys_chmod(const char *path, uix_mode_t mode)
{
    (void)path;
    (void)mode;
    return -1;
}

static inline int sys_chown(const char *path,
                            uix_uid_t owner, uix_gid_t group)
{
    (void)path;
    (void)owner;
    (void)group;
    return -1;
}

static inline int sys_mkdir(const char *path, uix_mode_t mode)
{
    (void)path;
    (void)mode;
    return -1;
}

static inline int sys_mknod(const char *path,
                            uix_mode_t mode, uix_dev_t dev)
{
    (void)path;
    (void)mode;
    (void)dev;
    return -1;
}

static inline int sys_unlink(const char *path)
{
    (void)path;
    return -1;
}

static inline int sys_rmdir(const char *path)
{
    (void)path;
    return -1;
}

static inline int sys_link(const char *old, const char *nw)
{
    (void)old;
    (void)nw;
    return -1;
}

static inline int sys_chdir(const char *path)
{
    (void)path;
    return -1;
}

static inline int sys_chroot(const char *path)
{
    (void)path;
    return -1;
}

static inline int sys_getdents(int fd, uix_DIR *dirp, int count)
{
    (void)fd;
    (void)dirp;
    (void)count;
    return -1;
}
#endif /* file-system */

#if 1 /* process */
#include "PoStd/uix_unistd.h"

static inline uix_pid_t sys_fork(void) { return -1; }
static inline uix_pid_t sys_getpid(void) { return 1; }
static inline uix_pid_t sys_getppid(void) { return 0; }

static inline void sys_exit(int status)
{
    (void)status;
    while (1)
    {
    }
}

static inline int sys_execv(const char *p, char *const av[])
{
    (void)p;
    (void)av;
    return -1;
}

static inline int sys_execve(const char *p,
                             char *const av[], char *const env[])
{
    (void)p;
    (void)av;
    (void)env;
    return -1;
}

static inline int sys_kill(uix_pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}
#endif /* process */

#if 1 /* wait */
#include "sys/uix_wait.h"

static inline uix_pid_t sys_wait4(uix_pid_t pid, int *wstatus,
                                  int options, void *rusage)
{
    (void)pid;
    (void)wstatus;
    (void)options;
    (void)rusage;
    return -1;
}
#endif /* wait */

#if 1 /* time */
#include "sys/uix_time.h"

static inline uix_time_t sys_time(uix_time_t *tloc) // not added
{
    (void)tloc;
    return 0;
}

static inline int sys_gettimeofday(uix_timeval_t *tv, void *tz)
{
    (void)tv;
    (void)tz;
    return 0;
}

static inline int sys_clock_gettime(int clkid, uix_timespec_t *tp)
{
    (void)clkid;
    (void)tp;
    return 0;
}
#endif /* time */

#if 1 /* times */
#include "sys/uix_times.h"

static inline uix_clock_t sys_times(uix_tms_t *buf) //not added
{
    (void)buf;
    return 0;
}
#endif

#if 1 /* utime */
#include "PoStd/uix_utime.h"

static inline int sys_utime(const char *path,
                            const uix_utimbuf_t *times) //not added
{
    (void)path;
    (void)times;
    return 0;
}
#endif

#if 1 /* utsname */
#include "sys/uix_utsname.h"

static inline int sys_uname(uix_utsname_t *buf) //not added
{
    (void)buf;
    return 0;
}
#endif

#if 1 /* mman */
#include "sys/uix_mman.h"

static inline void *sys_mmap(void *addr, uix_size_t length,
                             int prot, int flags,
                             int fd, uix_off_t offset)
{
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    return (void *)-1;
}

static inline int sys_munmap(void *addr, uix_size_t length)
{
    (void)addr;
    (void)length;
    return 0;
}

static inline int sys_mprotect(void *addr, uix_size_t len, int prot)
{
    (void)addr;
    (void)len;
    (void)prot;
    return 0;
}

static inline int sys_shm_open(const char *name,
                               int oflag, uix_mode_t mode)//not added
{
    (void)name;
    (void)oflag;
    (void)mode;
    return -1;
}

static inline int sys_shm_unlink(const char *name) //not added
{
    (void)name;
    return -1;
}
#endif /* mman */

#if 1 /* socket */
#include "sys/uix_socket.h"

static inline int sys_socket(int domain, int type, int protocol)
{
    (void)domain;
    (void)type;
    (void)protocol;
    return -1;
}

static inline int sys_bind(int sockfd,
                           const uix_sockaddr_t *addr,
                           uix_socklen_t addrlen)
{
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
}

static inline int sys_listen(int sockfd, int backlog)
{
    (void)sockfd;
    (void)backlog;
    return -1;
}

static inline int sys_accept(int sockfd,
                             uix_sockaddr_t *addr,
                             uix_socklen_t *addrlen)
{
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
}

static inline int sys_connect(int sockfd,
                              const uix_sockaddr_t *addr,
                              uix_socklen_t addrlen)
{
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
}

static inline int sys_shutdown(int sockfd, int how)
{
    (void)sockfd;
    (void)how;
    return -1;
}

static inline uix_ssize_t sys_sendto(int sockfd,
                                     const void *buf, uix_size_t len,
                                     int flags,
                                     const uix_sockaddr_t *dest,
                                     uix_socklen_t addrlen)
{
    (void)sockfd;
    (void)buf;
    (void)len;
    (void)flags;
    (void)dest;
    (void)addrlen;
    return -1;
}

static inline uix_ssize_t sys_recvfrom(int sockfd,
                                       void *buf, uix_size_t len,
                                       int flags,
                                       uix_sockaddr_t *src,
                                       uix_socklen_t *addrlen)
{
    (void)sockfd;
    (void)buf;
    (void)len;
    (void)flags;
    (void)src;
    (void)addrlen;
    return -1;
}

static inline int sys_getsockopt(int sockfd, int level, int optname,
                                 void *optval, uix_socklen_t *optlen)
{
    (void)sockfd;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;
    return -1;
}

static inline int sys_setsockopt(int sockfd, int level, int optname,
                                 const void *optval,
                                 uix_socklen_t optlen)
{
    (void)sockfd;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;
    return -1;
}

static inline int sys_getsockname(int sockfd,
                                  uix_sockaddr_t *addr,
                                  uix_socklen_t *addrlen)
{
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
}

static inline int sys_getpeername(int sockfd,
                                  uix_sockaddr_t *addr,
                                  uix_socklen_t *addrlen)
{
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
}
#endif /* socket */

#if 1 /* select */
#include "sys/uix_select.h"

static inline int sys_select(int nfds,
                             uix_fd_set *rfds, uix_fd_set *wfds,
                             uix_fd_set *efds, uix_timeval_t *timeout)
{
    (void)nfds;
    (void)rfds;
    (void)wfds;
    (void)efds;
    (void)timeout;
    return 0;
}
#endif

#if 1 /* poll */
#include "PoStd/uix_poll.h"

static inline int sys_poll(uix_pollfd_t *fds,
                           uix_nfds_t nfds, int timeout)
{
    (void)fds;
    (void)nfds;
    (void)timeout;
    return 0;
}
#endif

#if 1 /* sched */
#include "PoStd/uix_sched.h"

static inline int sys_sched_setscheduler(uix_pid_t pid, int policy,
                                         const uix_sched_param_t *p) //not added
{
    (void)pid;
    (void)policy;
    (void)p;
    return 0;
}

static inline int sys_sched_getscheduler(uix_pid_t pid) //not added
{
    (void)pid;
    return 0;
}

static inline int sys_sched_yield(void) { return 0; }
#endif

#if 1 /* ioctl */
#include "sys/uix_ioctl.h"

static inline int sys_ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    (void)request;
    return -1;
}
#endif

#if 1 /* msg */
#include "sys/uix_msg.h"

static inline int sys_msgsnd(int msqid, const void *msgp,
                             uix_size_t msgsz, int msgflg)
{
    (void)msqid;
    (void)msgp;
    (void)msgsz;
    (void)msgflg;
    return 0;
}

static inline int sys_msgget(uix_key_t key, int msgflg)
{
    (void)key;
    (void)msgflg;
    return 0;
}

static inline uix_ssize_t sys_msgrcv(int msqid, void *msgp,
                                     uix_size_t msgsz,
                                     long msgtyp, int msgflg)
{
    (void)msqid;
    (void)msgp;
    (void)msgsz;
    (void)msgtyp;
    (void)msgflg;
    return 0;
}

static inline int sys_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
{
    (void)msqid;
    (void)cmd;
    (void)buf;
    return 0;
}
#endif /* msg */

#if 1 /* shm */
#include "sys/uix_shm.h"

static inline int sys_shmget(uix_key_t key,
                             uix_size_t size, int shmflg)
{
    (void)key;
    (void)size;
    (void)shmflg;
    return 0;
}

static inline void *sys_shmat(int shmid,
                              const void *shmaddr, int shmflg)
{
    (void)shmid;
    (void)shmaddr;
    (void)shmflg;
    return (void *)-1;
}

static inline int sys_shmdt(const void *shmaddr)
{
    (void)shmaddr;
    return 0;
}

static inline int sys_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
{
    (void)shmid;
    (void)cmd;
    (void)buf;
    return 0;
}
#endif /* shm */

#if 1 /* sem */
#include "sys/uix_sem.h"

static inline int sys_semget(uix_key_t key, int nsems, int semflg)
{
    (void)key;
    (void)nsems;
    (void)semflg;
    return 0;
}

static inline int sys_semop(int semid,
                            uix_sembuf_t *sops, uix_size_t nsops)
{
    (void)semid;
    (void)sops;
    (void)nsops;
    return 0;
}

static inline int sys_semctl(int semid, int semnum, int cmd, ...) //not added
{
    (void)semid;
    (void)semnum;
    (void)cmd;
    return 0;
}
#endif /* sem */

#if 1 /* getdents / dir */
#include "Postd/uix_ifaddrs.h"
#include "sys/uix_socket.h"
#include "Postd/uix_netdb.h"

static inline int sys_getifaddrs(uix_ifaddrs_t **ifap) //not added
{
    (void)ifap;
    return -1;
}

static inline uix_hostent_t *sys_gethostbyname(const char *name) //not added
{
    (void)name;
    return 0;
}
#endif

#if 1 /* mqueue */
#include "PoStd/uix_mqueue.h"

static inline uix_mqd_t sys_mq_open(const char *name,
                                    int oflag, ...) //not added
{
    (void)name;
    (void)oflag;
    return (uix_mqd_t)-1;
}

static inline int sys_mq_close(uix_mqd_t mqdes) //not added
{
    (void)mqdes;
    return 0;
}

static inline int sys_mq_unlink(const char *name) //not added
{
    (void)name;
    return 0;
}

static inline int sys_mq_send(uix_mqd_t mqdes,
                              const char *msg_ptr,
                              uix_size_t msg_len,
                              unsigned int msg_prio) //not added
{
    (void)mqdes;
    (void)msg_ptr;
    (void)msg_len;
    (void)msg_prio;
    return -1;
}

static inline uix_ssize_t sys_mq_receive(uix_mqd_t mqdes,
                                         char *msg_ptr,
                                         uix_size_t msg_len,
                                         unsigned int *msg_prio) //not added
{
    (void)mqdes;
    (void)msg_ptr;
    (void)msg_len;
    (void)msg_prio;
    return -1;
}
#endif /* mqueue */

#endif /* __UIX_SYS__H */
