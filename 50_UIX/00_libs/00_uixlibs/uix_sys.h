#ifndef __UIX_SYS__H
#define __UIX_SYS__H

#include "sys/uix_types.h"

#define STUB 1// 0:STUB, 1:SYS funtinal call to Kernal


#if 0
#define SYS_EXIT            1 //done
#define SYS_FORK            2 //done
#define SYS_READ            3 //done
#define SYS_WRITE           4 //done
#define SYS_OPEN            5   // done
#define SYS_CLOSE           6 //done
#define SYS_GETENTROPY      7
#define SYS___TFORK         8
#define SYS_LINK            9 //done
#define SYS_unlink          10 //done
#define sys_wait4           11//done
#define sys_chdir           12 //done
#define SYS_fchdir          13
#define SYS_mknod           14//done
#define SYS_chmod           15 //done
#define SYS_chown           16//done
#define SYS_break           17
#define SYS_getdtablecount  18
#define SYS_getrusage       19
#define SYS_getpid          20 //done
#define SYS_mount           21
#define SYS_unmount         22
#define SYS_setuid          23
#define SYS_getuid          24
#define SYS_geteuid         25
#define SYS_ptrace          26
#define SYS_recvmsg         27
#define SYS_sendmsg         28
#define SYS_recvfrom        29//done
#define SYS_accept          30 //done
#define SYS_getpeername     31 //done
#define SYS_getsockname     32 //done
#define SYS_access          33
#define SYS_chflags         34
#define SYS_fchflags        35
#define SYS_sync            36
#define SYS_stat            38 //done
#define SYS_getppid         39 //done
#define SYS_lstat           40//done
#define SYS_dup             41 //done
#define SYS_fstatat         42
#define SYS_getegid         43
#define SYS_ktrace          45
#define SYS_sigaction       46
#define SYS_getgid          47
#define SYS_sigprocmask     48
#define SYS_mmap            49
#define SYS_setlogin        50
#define SYS_acct            51
#define SYS_sigpending      52
#define SYS_fstat           53 //done
#define SYS_ioctl           54 //done
#define SYS_reboot          55
#define SYS_revoke          56
#define SYS_symlink         57
#define SYS_readlink        58
#define sys_execve          59
#define SYS_execv           250 //done
#define SYS_umask           60
#define SYS_chroot          61 //done
#define SYS_getfsstat       62
#define SYS_statfs          63
#define SYS_fstatfs         64
#define SYS_fhstatfs        65
#define SYS_vfork           66
#define SYS_gettimeofday    67 //done
#define SYS_settimeofday    68
#define SYS_setitimer       69
#define SYS_getitimer       70
#define SYS_select          71 //done
#define SYS_kevent          72
#define SYS_munmap          73 //done
#define SYS_mprotect        74 //done
#define SYS_madvise         75
#define SYS_utimes          76
#define SYS_futimes         77
#define SYS_mquery          78
#define SYS_getgroups       79
#define SYS_setgroups       80
#define SYS_getpgrp         81
#define SYS_setpgid         82
#define SYS_futex           83
#define SYS_utimensat       84
#define SYS_futimens        85
#define SYS_kbind           86
#define sys_clock_gettime   87 //done
#define SYS_clock_settime   88
#define SYS_clock_getres    89
#define SYS_dup2            90
#define SYS_nanosleep       91
#define sys_fcntl           92 // done
#define SYS_accept4         93
#define SYS___thrsleep      94
#define SYS_fsync           95
#define SYS_setpriority     96
#define SYS_socket          97//done
#define SYS_connect         98 //done
#define SYS_getdents        99 //done
#define SYS_getpriority     100
#define SYS_pipe2           101
#define SYS_dup3            102
#define SYS_sigreturn       103
#define SYS_bind            104 //done
#define SYS_setsockopt      105 //done
#define SYS_listen          106 //done
#define SYS_chflagsat       107
#define SYS_pledge          108
#define SYS_ppoll           109
#define SYS_pselect         110
#define SYS_sigsuspend      111
#define SYS_sendsyslog      112
#define SYS_unveil          114
#define SYS___realpath      115
#define SYS_recvmmsg        116
#define SYS_sendmmsg        117
#define SYS_getsockopt      118 //done
#define SYS_thrkill         119
#define SYS_readv           120
#define SYS_writev          121
#define SYS_kill            122 //done
#define SYS_fchown          123
#define SYS_fchmod          124
#define SYS___pledge_open   125
#define SYS_setreuid        126
#define SYS_setregid        127
#define SYS_rename          128
#define SYS_flock           131
#define SYS_mkfifo          132
#define SYS_sendto          133 //done
#define SYS_shutdown        134 //done
#define SYS_socketpair      135
#define SYS_mkdir           136 //done
#define SYS_rmdir           137 //done
#define SYS_adjtime         140
#define SYS_getlogin_r      141
#define SYS_getthrname      142
#define SYS_setthrname      143
#define SYS_setsid          147
#define SYS_quotactl        148
#define SYS_ypconnect       150
#define SYS_nfssvc          155
#define SYS_pinsyscalls     158
#define SYS_mimmutable      159
#define SYS_waitid          160
#define SYS_getfh           161
#define SYS___tmpfd         164
#define SYS_sysarch         165
#define SYS_lseek           166 //done
#define SYS_truncate        167
#define SYS_ftruncate       168
#define SYS_pread           169
#define SYS_pwrite          170
#define SYS_preadv          171
#define SYS_pwritev         172
#define SYS_profil          175
#define SYS_setgid          181
#define SYS_setegid         182
#define SYS_seteuid         183
#define SYS_pathconfat      190
#define SYS_pathconf        191
#define SYS_fpathconf       192
#define SYS_swapctl         193
#define SYS_getrlimit       194
#define SYS_setrlimit       195
#define SYS_sysctl          202
#define SYS_mlock           203
#define SYS_munlock         204
#define SYS_getpgid         207
#define SYS_utrace          209
#define SYS_semget          221 //done
#define SYS_msgget          225 //done
#define SYS_msgsnd          226 //done
#define SYS_msgrcv          227 //done
#define SYS_shmat           228 //done
#define SYS_shmdt           230 //done
#define SYS_minherit        250
#define sys_poll            252 //done
#define SYS_issetugid       253
#define SYS_lchown          254
#define SYS_getsid          255
#define SYS_msync           256
#define SYS_pipe            263 //done
#define SYS_fhopen          264
#define SYS_kqueue          269
#define SYS_kqueue1         270
#define SYS_mlockall        271
#define SYS_munlockall      272
#define SYS_getresuid       281
#define SYS_setresuid       282
#define SYS_getresgid       283
#define SYS_setresgid       284
#define SYS_closefrom       287
#define SYS_sigaltstack     288
#define SYS_shmget          289
#define SYS_semop           290 //done
#define SYS_fhstat          294
#define SYS___semctl        295
#define SYS_shmctl          296 //dones
#define SYS_msgctl          297 //done
#define SYS_sched_yield     298 //done
#define SYS_getthrid        299
#define SYS___thrwakeup     301
#define SYS___threxit       302
#define SYS___thrsigdivert  303
#define SYS___getcwd        304
#define SYS_adjfreq         305
#define SYS_setrtable       310
#define SYS_getrtable       311
#define SYS_faccessat       313
#define SYS_fchmodat        314
#define SYS_fchownat        315
#define SYS_linkat          317
#define SYS_mkdirat         318
#define SYS_mkfifoat        319
#define SYS_mknodat         320
#define SYS_openat          321
#define SYS_readlinkat      322
#define SYS_renameat        323
#define SYS_symlinkat       324
#define SYS_unlinkat        325
#define SYS___set_tcb       329
#define SYS___get_tcb       330
#define SYS_MAXSYSCALL      331
#endif

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
