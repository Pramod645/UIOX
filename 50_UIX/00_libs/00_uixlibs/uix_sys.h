#ifndef __UIX_SYS__H
#define __UIX_SYS__H

#include "sys/uix_types.h"

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

#if 1  /* file-system calls */
#include "PoStd/uix_fcntl.h"
#include "sys/uix_stat.h"
#include "PoStd/uix_dirent.h"

static inline int sys_open(const char *path, int flags, ...)
    { (void)path; (void)flags; return -1; }

static inline int sys_creat(const char *path, uix_mode_t mode)
    { (void)path; (void)mode; return -1; }

static inline int sys_close(int fd)
    { (void)fd; return 0; }

static inline uix_ssize_t sys_read(int fd, void *buf, uix_size_t count)
    { (void)fd; (void)buf; (void)count; return -1; }

static inline uix_ssize_t sys_write(int fd, const void *buf, uix_size_t count)
    { (void)fd; (void)buf; (void)count; return -1; }

static inline uix_off_t sys_lseek(int fd, uix_off_t off, int whence)
    { (void)fd; (void)off; (void)whence; return -1; }

static inline int sys_fcntl(int fd, int cmd, ...)
    { (void)fd; (void)cmd; return -1; }

static inline int sys_dup(int fd)
    { (void)fd; return -1; }

static inline int sys_dup2(int old, int nw)
    { (void)old; (void)nw; return -1; }

static inline int sys_pipe(int pipefd[2])
    { (void)pipefd; return -1; }

static inline int sys_stat(const char *path, uix_stat_t *buf)
    { (void)path; (void)buf; return -1; }

static inline int sys_fstat(int fd, uix_stat_t *buf)
    { (void)fd; (void)buf; return -1; }

static inline int sys_lstat(const char *path, uix_stat_t *buf)
    { (void)path; (void)buf; return -1; }

static inline int sys_chmod(const char *path, uix_mode_t mode)
    { (void)path; (void)mode; return -1; }

static inline int sys_chown(const char *path,
                             uix_uid_t owner, uix_gid_t group)
    { (void)path; (void)owner; (void)group; return -1; }

static inline int sys_mkdir(const char *path, uix_mode_t mode)
    { (void)path; (void)mode; return -1; }

static inline int sys_mknod(const char *path,
                             uix_mode_t mode, uix_dev_t dev)
    { (void)path; (void)mode; (void)dev; return -1; }

static inline int sys_unlink(const char *path)
    { (void)path; return -1; }

static inline int sys_rmdir(const char *path)
    { (void)path; return -1; }

static inline int sys_link(const char *old, const char *nw)
    { (void)old; (void)nw; return -1; }

static inline int sys_chdir(const char *path)
    { (void)path; return -1; }

static inline int sys_chroot(const char *path)
    { (void)path; return -1; }

static inline int sys_getdents(int fd, uix_DIR *dirp, int count)
    { (void)fd; (void)dirp; (void)count; return -1; }
#endif  /* file-system */

#if 1  /* process */
#include "PoStd/uix_unistd.h"

static inline uix_pid_t sys_fork(void)   { return -1; }
static inline uix_pid_t sys_getpid(void) { return 1; }
static inline uix_pid_t sys_getppid(void){ return 0; }

static inline void sys_exit(int status)
    { (void)status; while(1){} }

static inline int sys_execv(const char *p, char *const av[])
    { (void)p; (void)av; return -1; }

static inline int sys_execve(const char *p,
                              char *const av[], char *const env[])
    { (void)p; (void)av; (void)env; return -1; }

static inline int sys_kill(uix_pid_t pid, int sig)
    { (void)pid; (void)sig; return -1; }
#endif  /* process */

#if 1  /* wait */
#include "sys/uix_wait.h"

static inline uix_pid_t sys_wait4(uix_pid_t pid, int *wstatus,
                                    int options, void *rusage)
    { (void)pid; (void)wstatus; (void)options; (void)rusage; return -1; }
#endif  /* wait */

#if 1  /* time */
#include "sys/uix_time.h"

static inline uix_time_t sys_time(uix_time_t *tloc)
    { (void)tloc; return 0; }

static inline int sys_gettimeofday(uix_timeval_t *tv, void *tz)
    { (void)tv; (void)tz; return 0; }

static inline int sys_clock_gettime(int clkid, uix_timespec_t *tp)
    { (void)clkid; (void)tp; return 0; }
#endif  /* time */

#if 1  /* times */
#include "sys/uix_times.h"

static inline uix_clock_t sys_times(uix_tms_t *buf)
    { (void)buf; return 0; }
#endif

#if 1  /* utime */
#include "PoStd/uix_utime.h"

static inline int sys_utime(const char *path,
                             const uix_utimbuf_t *times)
    { (void)path; (void)times; return 0; }
#endif

#if 1  /* utsname */
#include "sys/uix_utsname.h"

static inline int sys_uname(uix_utsname_t *buf)
    { (void)buf; return 0; }
#endif

#if 1  /* mman */
#include "sys/uix_mman.h"

static inline void *sys_mmap(void *addr, uix_size_t length,
                               int prot, int flags,
                               int fd, uix_off_t offset)
    { (void)addr;(void)length;(void)prot;
      (void)flags;(void)fd;(void)offset; return (void*)-1; }

static inline int sys_munmap(void *addr, uix_size_t length)
    { (void)addr; (void)length; return 0; }

static inline int sys_mprotect(void *addr, uix_size_t len, int prot)
    { (void)addr; (void)len; (void)prot; return 0; }

static inline int sys_shm_open(const char *name,
                                int oflag, uix_mode_t mode)
    { (void)name; (void)oflag; (void)mode; return -1; }

static inline int sys_shm_unlink(const char *name)
    { (void)name; return -1; }
#endif  /* mman */

#if 1  /* socket */
#include "sys/uix_socket.h"

static inline int sys_socket(int domain, int type, int protocol)
    { (void)domain;(void)type;(void)protocol; return -1; }

static inline int sys_bind(int sockfd,
                            const uix_sockaddr_t *addr,
                            uix_socklen_t addrlen)
    { (void)sockfd;(void)addr;(void)addrlen; return -1; }

static inline int sys_listen(int sockfd, int backlog)
    { (void)sockfd;(void)backlog; return -1; }

static inline int sys_accept(int sockfd,
                              uix_sockaddr_t *addr,
                              uix_socklen_t *addrlen)
    { (void)sockfd;(void)addr;(void)addrlen; return -1; }

static inline int sys_connect(int sockfd,
                               const uix_sockaddr_t *addr,
                               uix_socklen_t addrlen)
    { (void)sockfd;(void)addr;(void)addrlen; return -1; }

static inline int sys_shutdown(int sockfd, int how)
    { (void)sockfd;(void)how; return -1; }

static inline uix_ssize_t sys_sendto(int sockfd,
                                      const void *buf, uix_size_t len,
                                      int flags,
                                      const uix_sockaddr_t *dest,
                                      uix_socklen_t addrlen)
    { (void)sockfd;(void)buf;(void)len;(void)flags;
      (void)dest;(void)addrlen; return -1; }

static inline uix_ssize_t sys_recvfrom(int sockfd,
                                        void *buf, uix_size_t len,
                                        int flags,
                                        uix_sockaddr_t *src,
                                        uix_socklen_t *addrlen)
    { (void)sockfd;(void)buf;(void)len;(void)flags;
      (void)src;(void)addrlen; return -1; }

static inline int sys_getsockopt(int sockfd, int level, int optname,
                                  void *optval, uix_socklen_t *optlen)
    { (void)sockfd;(void)level;(void)optname;
      (void)optval;(void)optlen; return -1; }

static inline int sys_setsockopt(int sockfd, int level, int optname,
                                  const void *optval,
                                  uix_socklen_t optlen)
    { (void)sockfd;(void)level;(void)optname;
      (void)optval;(void)optlen; return -1; }

static inline int sys_getsockname(int sockfd,
                                   uix_sockaddr_t *addr,
                                   uix_socklen_t *addrlen)
    { (void)sockfd;(void)addr;(void)addrlen; return -1; }

static inline int sys_getpeername(int sockfd,
                                   uix_sockaddr_t *addr,
                                   uix_socklen_t *addrlen)
    { (void)sockfd;(void)addr;(void)addrlen; return -1; }
#endif  /* socket */

#if 1  /* select */
#include "sys/uix_select.h"

static inline int sys_select(int nfds,
                              uix_fd_set *rfds, uix_fd_set *wfds,
                              uix_fd_set *efds, uix_timeval_t *timeout)
    { (void)nfds;(void)rfds;(void)wfds;
      (void)efds;(void)timeout; return 0; }
#endif

#if 1  /* poll */
#include "PoStd/uix_poll.h"

static inline int sys_poll(uix_pollfd_t *fds,
                            uix_nfds_t nfds, int timeout)
    { (void)fds;(void)nfds;(void)timeout; return 0; }
#endif

#if 1  /* sched */
#include "PoStd/uix_sched.h"

static inline int sys_sched_setscheduler(uix_pid_t pid, int policy,
                                          const uix_sched_param_t *p)
    { (void)pid;(void)policy;(void)p; return 0; }

static inline int sys_sched_getscheduler(uix_pid_t pid)
    { (void)pid; return 0; }

static inline int sys_sched_yield(void) { return 0; }
#endif

#if 1  /* ioctl */
#include "sys/uix_ioctl.h"

static inline int sys_ioctl(int fd, unsigned long request, ...)
    { (void)fd;(void)request; return -1; }
#endif

#if 1  /* msg */
#include "sys/uix_msg.h"

static inline int sys_msgsnd(int msqid, const void *msgp,
                              uix_size_t msgsz, int msgflg)
    { (void)msqid;(void)msgp;(void)msgsz;(void)msgflg; return 0; }

static inline int sys_msgget(uix_key_t key, int msgflg)
    { (void)key;(void)msgflg; return 0; }

static inline uix_ssize_t sys_msgrcv(int msqid, void *msgp,
                                      uix_size_t msgsz,
                                      long msgtyp, int msgflg)
    { (void)msqid;(void)msgp;(void)msgsz;
      (void)msgtyp;(void)msgflg; return 0; }

static inline int sys_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
    { (void)msqid;(void)cmd;(void)buf; return 0; }
#endif  /* msg */

#if 1  /* shm */
#include "sys/uix_shm.h"

static inline int sys_shmget(uix_key_t key,
                              uix_size_t size, int shmflg)
    { (void)key;(void)size;(void)shmflg; return 0; }

static inline void *sys_shmat(int shmid,
                               const void *shmaddr, int shmflg)
    { (void)shmid;(void)shmaddr;(void)shmflg; return (void*)-1; }

static inline int sys_shmdt(const void *shmaddr)
    { (void)shmaddr; return 0; }

static inline int sys_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
    { (void)shmid;(void)cmd;(void)buf; return 0; }
#endif  /* shm */

#if 1  /* sem */
#include "sys/uix_sem.h"

static inline int sys_semget(uix_key_t key, int nsems, int semflg)
    { (void)key;(void)nsems;(void)semflg; return 0; }

static inline int sys_semop(int semid,
                             uix_sembuf_t *sops, uix_size_t nsops)
    { (void)semid;(void)sops;(void)nsops; return 0; }

static inline int sys_semctl(int semid, int semnum, int cmd, ...)
    { (void)semid;(void)semnum;(void)cmd; return 0; }
#endif  /* sem */

#if 1  /* getdents / dir */
#include "Postd/uix_ifaddrs.h"
#include "sys/uix_socket.h"
#include "Postd/uix_netdb.h"

static inline int sys_getifaddrs(uix_ifaddrs_t **ifap)
    { (void)ifap; return -1; }

static inline uix_hostent_t *sys_gethostbyname(const char *name)
    { (void)name; return 0; }
#endif

#if 1  /* mqueue */
#include "PoStd/uix_mqueue.h"

static inline uix_mqd_t sys_mq_open(const char *name,
                                      int oflag, ...)
    { (void)name;(void)oflag; return (uix_mqd_t)-1; }

static inline int sys_mq_close(uix_mqd_t mqdes)
    { (void)mqdes; return 0; }

static inline int sys_mq_unlink(const char *name)
    { (void)name; return 0; }

static inline int sys_mq_send(uix_mqd_t mqdes,
                               const char *msg_ptr,
                               uix_size_t msg_len,
                               unsigned int msg_prio)
    { (void)mqdes;(void)msg_ptr;(void)msg_len;
      (void)msg_prio; return -1; }

static inline uix_ssize_t sys_mq_receive(uix_mqd_t mqdes,
                                          char *msg_ptr,
                                          uix_size_t msg_len,
                                          unsigned int *msg_prio)
    { (void)mqdes;(void)msg_ptr;(void)msg_len;
      (void)msg_prio; return -1; }
#endif  /* mqueue */

#endif /* __UIX_SYS__H */
