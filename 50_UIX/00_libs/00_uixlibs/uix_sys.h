#include "sys/uix_types.h"

#if 0 // create and  close sys call definations which is coming from uix_fcntl.h
int sys_create(const char *name, unsigned int flags)
{
	//return __sysret(sys_memfd_create(name, flags));
}

int sys_close(const char *name, unsigned int flags)
{
	//return __sysret(sys_memfd_create(name, flags));
}
#endif 

#if 0 // wait sys call defination which is coming from uix_wait.h

int sys_wait(const char *name, unsigned int flags)
{
	//return __sysret(sys_memfd_create(name, flags));
}

#endif

#if 0 // kill and signal,  sys call defination which is coming from uix_signal.h

int sys_kill(pid_t pid, int signal)
{
	//return my_syscall2(__NR_kill, pid, signal);
}

int sys_signal(pid_t pid, int signal)
{
	//return my_syscall2(__NR_kill, pid, signal);
}

#endif

#if 0 // chown, chmod, stat, mknod, mkdir, chmod and stat,  sys call defination which is coming from uix_state.h

int sys_chown(const char *path, uid_t owner, gid_t group)
{

	//return my_syscall3(__NR_chown, path, owner, group);

}

int sys_chmod(const char *path, uix_mode_t mode)
{

	//return my_syscall2(__NR_chmod, path, mode);

}

int sys_stat(const char *path, uid_t owner, gid_t group)
{

	//return my_syscall3(__NR_chown, path, owner, group);

}

int sys_mknod(const char *path, uid_t owner, gid_t group)
{

	//return my_syscall3(__NR_chown, path, owner, group);

}

int sys_mkdir(const char *path, uid_t owner, gid_t group)
{

	//return my_syscall3(__NR_chown, path, owner, group);

}

int sys_chmod(const char *path, uid_t owner, gid_t group)
{

	//return my_syscall3(__NR_chown, path, owner, group);

}

#endif


#if 0 // open,  and close,  sys call defination which is coming from uix_stdio.h

int sys_open(int fd)
{
	//return my_syscall1(__NR_close, fd);
}

int sys_close(int fd)
{
	//return my_syscall1(__NR_close, fd);
}

#endif


#if 0 // dup, pipe ,close, chdir, chroot, link, unlink , read, write, lseek, fork, exec,exit, setpgrp and setuid,  sys call defination which is coming from uix.unisdt.h

//dup
int sys_dup(int fd)
{
	//return my_syscall1(__NR_dup, fd);
}
//pipe

int sys_pipe(int pipefd[2], int flags)
{
	//return my_syscall2(__NR_pipe2, pipefd, flags);
}

//chdir
int sys_chdir(const char *path)
{
	//return my_syscall1(__NR_chdir, path);
}

//chroot
int sys_chroot(const char *path)
{
	return my_syscall1(__NR_chroot, path);
}
//link
int sys_link(const char *old, const char *new)
{
	//return my_syscall2(__NR_link, old, new);

}
//unlink
int sys_unlink(const char *path)
{

	//return my_syscall1(__NR_unlink, path);

}
//read
ssize_t sys_read(int fd, void *buf, size_t count)
{
	//return my_syscall3(__NR_read, fd, buf, count);
}
//write
ssize_t sys_write(int fd, const void *buf, size_t count)
{
	//return my_syscall3(__NR_write, fd, buf, count);
}
//lseek
off_t sys_lseek(int fd, off_t offset, int whence)
{

	//return my_syscall3(__NR_lseek, fd, offset, whence);

}
//fork
pid_t sys_fork(void)
{


	//return my_syscall0(__NR_fork);
}
//exec
int sys_execve(const char *filename, char *const argv[], char *const envp[])
{
	//return my_syscall3(__NR_execve, filename, argv, envp);
}
//exit
void sys_exit(int status)
{
	//my_syscall1(__NR_exit, status & 255);
	
}

//setpgrp
pid_t setpgrp(void)
{
	//return setpgid(0, 0);
}
//setuid
#endif


#if 0 // mount and  unmount, sys call defination which is coming from ???.h

int sys_mount(const char *path, int flags)
{
	//return my_syscall2(__NR_umount2, path, flags);
}

int sys_umount(const char *path, int flags)
{
	//return my_syscall2(__NR_umount2, path, flags);
}

#endif


#if 0 // brk, sys call defination which is coming from ???.h

void *sys_brk(void *addr)
{
	//return (void *)my_syscall1(__NR_brk, addr);
}

#endif


#if 1 //sys_msg
#include "sys/uix_msg.h"
int sys_msgsnd(int msqid, const void *msgp,uix_size_t msgsz, int msgflg)
{
	//return (void *)my_syscall1(__NR_brk, addr);
	return 0;

}

int sys_msgget(uix_key_t key, int msgflg)
{
	//return (void *)my_syscall1(__NR_brk, addr);
	return 0;

}
uix_ssize_t sys_msgrcv(int msqid, void *msgp, uix_size_t msgsz,
	long msgtyp, int msgflg)
{
	//return (void *)my_syscall1(__NR_brk, addr);
	return 0;

}
int sys_msgctl(int msqid, int cmd, uix_msqid_ds_t *buf)
{
	//return (void *)my_syscall1(__NR_brk, addr);
	return 0;

}

#endif

#if 1 //shm
#include "sys/uix_shm.h"
int sys_shmget(uix_key_t key, uix_size_t size, int shmflg)
{
	return 0;
}

int *sys_shmat(int shmid, const void *shmaddr, int shmflg)
{
	return 0;
}

int sys_shmdt(const void *shmaddr)
{
	return 0;
}

int sys_shmctl(int shmid, int cmd, uix_shmid_ds_t *buf)
{
	return 0;
}

#endif

#if 1 //stat
#include "sys/uix_stat.h"
int sys_stat(const char *path, uix_stat_t *buf)
{
	return 0;
}

int sys_fstat(int fd, uix_stat_t *buf)
{
	return 0;
}

int sys_lstat(const char *path, uix_stat_t *buf)
{
	return 0;
}

int sys_chmod(const char *path, uix_mode_t mode)
{
	return 0;
}

int sys_chown(const char *path, uix_uid_t owner, uix_gid_t group)
{
	return 0;
}

int sys_mkdir(const char *path, uix_mode_t mode)
{
	return 0;
}

int sys_mknod(const char *path, uix_mode_t mode, uix_dev_t dev)
{
	return 0;
}

#endif

#if 1 //mman
#include "sys/uix_mman.h"

void *sys_mmap(void *addr, uix_size_t length, int prot,
	int flags, int fd, uix_off_t offset)
{
	return 0;
}

int sys_munmap(void *addr, uix_size_t length)
{
	return 0;
}

int sys_mprotect(void *addr, uix_size_t len, int prot)
{
	return 0;
}


int sys_shm_open(const char *name, int oflag, uix_mode_t mode)
{
	return 0;
}

int sys_shm_unlink(const char *name)
{
	return 0;
}

#endif

#if 1 //
#include "sys/uix_sem.h"
int sys_semget(uix_key_t key, int nsems, int semflg)
{
	return 0;
}

int sys_semop(int semid, uix_sembuf_t *sops, uix_size_t nsops)
{
	return 0;
}

int sys_semctl(int semid, int semnum, int cmd, ...)
{
	return 0;
}

#endif

#if 1 //mqueue
#include "PoStd/uix_mqueue.h"


uix_mqd_t sys_mq_open(const char *name, int oflag, ...)
{
	return 0;
}

int sys_mq_close(uix_mqd_t mqdes)
{
	return 0;
}

int sys_mq_unlink(const char *name)
{
	return 0;
}

int sys_mq_send(uix_mqd_t mqdes, const char *msg_ptr,
	uix_size_t msg_len, unsigned int msg_prio)
{
	return 0;
}

uix_ssize_t sys_mq_receive(uix_mqd_t mqdes, char *msg_ptr,
	uix_size_t msg_len, unsigned int *msg_prio)
{
	return 0;
}

#endif

#if 1 //uinstd
#include "PoStd/uix_unistd.h"

uix_ssize_t sys_read(int fd, void *buf, uix_size_t count)
{
	return 0;
}

uix_ssize_t sys_write(int fd, const void *buf, uix_size_t count)
{
	return 0;
}

int sys_close(int fd)
{
	return 0;
}

uix_off_t sys_lseek(int fd, uix_off_t offset, int whence)
{
	return 0;
}

int sys_dup(int oldfd)
{
	return 0;
}

int sys_dup2(int oldfd, int newfd)
{
	return 0;
}

int sys_pipe(int pipefd[2])
{
	return 0;
}


uix_pid_t sys_fork(void)
{
	return 0;
}

uix_pid_t sys_getpid(void)
{
	return 0;
}

uix_pid_t sys_getppid(void)
{
	return 0;
}

void sys_exit(int status)
{
	//return 0;
}


int sys_execv(const char *path, char *const argv[])
{
	return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
	return 0;
}

int sys_chdir(const char *path)
{
	return 0;
}

int sys_chroot(const char *path)
{
	return 0;
}

int sys_unlink(const char *path)
{
	return 0;
}


int sys_rmdir(const char *path)
{
	return 0;
}

int sys_link(const char *oldpath, const char *newpath)
{
	return 0;
}

#endif


#if 1//fcntle
#include "PoStd/uix_fcntl.h"

int sys_open(const char *path, int flags, ...)
{
	return 0;
}

int sys_fcntl(int fd, int cmd, ...)
{

	return 0;
}

#endif

#if 1 //wait
#include "sys/uix_wait.h"

uix_pid_t sys_wait4(uix_pid_t pid, int *wstatus,
                    int options, void *rusage)
{
	return 0;
}

#endif


#if 1 //select
#include "sys/uix_select.h"
int sys_select(int nfds, uix_fd_set *readfds, uix_fd_set *writefds,
	uix_fd_set *exceptfds, uix_timeval_t *timeout)
{

	return 0;
}

#endif

#if 1 //sched
#include "PoStd/uix_sched.h"

int sys_sched_setscheduler(uix_pid_t pid, int policy,
	const uix_sched_param_t *param)
{
	return 0;
}

int sys_sched_getscheduler(uix_pid_t pid)
{
	return 0;
}
int sys_sched_yield(void)
{
    return 0;

}

#endif


#if 1// utsname
#include "sys/uix_utsname.h"

int sys_uname(uix_utsname_t *buf)
{
	return 0;
}

#endif


#if 1// utime
#include "PoStd/uix_utime.h"

int sys_utime(const char *path, const uix_utimbuf_t *times)
{
	return 0;
}

#endif

#if 1// times
#include "sys/uix_times.h"

uix_clock_t sys_times(uix_tms_t *buf)
{
	return 0;
}

#endif


#if 1// time
#include "sys/uix_time.h"

uix_time_t sys_time(uix_time_t *tloc)
{
	return 0;
}

int sys_gettimeofday(uix_timeval_t *tv, void *tz)
{
	return 0;
}

int sys_clock_gettime(int clkid, uix_timespec_t *tp)
{
	return 0;
}

#endif


#if 1 //poll
#include "PoStd/uix_poll.h"

int sys_poll(uix_pollfd_t *fds, uix_nfds_t nfds, int timeout)
{

	return 0;
}

#endif


#if 1 //signal
#include "PoStd/uix_signal.h"

int sys_kill(uix_pid_t pid, int sig)
{
	return 0;
}

#endif

#if 1 //ioctl
#include "sys/uix_ioctl.h"

int sys_ioctl(int fd, unsigned long request, ...)
{
	return 0;
}

#endif


#if 1//dirent
#include "PoStd/uix_dirent.h"


int sys_getdents(int fd, uix_DIR *dirp, int count)
{
	//return my_syscall3(__NR_getdents64, fd, dirp, count);
	return 0;
}

#endif

#if 1//netdb
#include "PoStd/uix_netdb.h"


uix_hostent_t *sys_gethostbyname(const char *name)
{
	return 0;
}

#endif

#if 1//ifaddrs
#include "PoStd/uix_ifaddrs.h"


int sys_getifaddrs(uix_ifaddrs_t **ifap)
{
	return 0;
}

#endif


#if 1//socket
#include "sys/uix_socket.h"

int sys_socket(int domain, int type, int protocol)
{
	return 0;
}

int sys_bind(int sockfd, const uix_sockaddr_t *addr,
             uix_socklen_t addrlen)
{
	return 0;
}

int sys_listen(int sockfd, int backlog)
{
	return 0;
}

int sys_accept(int sockfd, uix_sockaddr_t *addr,
               uix_socklen_t *addrlen)
{
	return 0;
}

int sys_connect(int sockfd, const uix_sockaddr_t *addr,
                uix_socklen_t addrlen)
{
	return 0;
}

int sys_shutdown(int sockfd, int how)
{
	return 0;
}

uix_ssize_t sys_sendto(int sockfd, const void *buf, uix_size_t len,
	int flags, const uix_sockaddr_t *dest_addr,
	uix_socklen_t addrlen)
{
	return 0;
}



uix_ssize_t sys_recvfrom(int sockfd, void *buf, uix_size_t len,
                         int flags, uix_sockaddr_t *src_addr,
                         uix_socklen_t *addrlen)
{
	return 0;
}

int sys_getsockopt(int sockfd, int level, int optname,
                   void *optval, uix_socklen_t *optlen)
{
	return 0;
}

int sys_setsockopt(int sockfd, int level, int optname,
                   const void *optval, uix_socklen_t optlen)
{
	return 0;
}

int sys_getsockname(int sockfd, uix_sockaddr_t *addr,
                    uix_socklen_t *addrlen)
{
	return 0;
}

int sys_getpeername(int sockfd, uix_sockaddr_t *addr,
                    uix_socklen_t *addrlen)
{
	return 0;
}

#endif


/* ***This is End of file, there is no more line should be added after this line*** */
