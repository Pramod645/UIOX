#include "uix_unistd.h"
#include "uix_errno.h"
#include "uix_string.h"

/* ── File I/O stubs — hook into your 40_syscall_interface ───── */
uix_ssize_t uix_read(int fd, void *buf, uix_size_t count)
{
    extern long sys_read(int, void *, uix_size_t)
        __attribute__((weak));
    if (sys_read) return (uix_ssize_t)sys_read(fd, buf, count);
    uix_errno = UIX_EBADF; return -1;
}

uix_ssize_t uix_write(int fd, const void *buf, uix_size_t count)
{
    extern long sys_write(int, const void *, uix_size_t)
        __attribute__((weak));
    if (sys_write) return (uix_ssize_t)sys_write(fd, buf, count);
    uix_errno = UIX_EBADF; return -1;
}

int uix_close(int fd)
{
    extern int sys_close(int) __attribute__((weak));
    if (sys_close) return sys_close(fd);
    uix_errno = UIX_EBADF; return -1;
}

uix_off_t uix_lseek(int fd, uix_off_t offset, int whence)
{
    extern long sys_lseek(int, uix_off_t, int) __attribute__((weak));
    if (sys_lseek) return (uix_off_t)sys_lseek(fd, offset, whence);
    uix_errno = UIX_ESPIPE; return -1;
}

int uix_dup(int oldfd)
{
    extern int sys_dup(int) __attribute__((weak));
    return sys_dup ? sys_dup(oldfd) : (uix_errno = UIX_EBADF, -1);
}

int uix_dup2(int oldfd, int newfd)
{
    extern int sys_dup2(int, int) __attribute__((weak));
    return sys_dup2 ? sys_dup2(oldfd, newfd)
                    : (uix_errno = UIX_EBADF, -1);
}

int uix_pipe(int pipefd[2])
{
    extern int sys_pipe(int *) __attribute__((weak));
    return sys_pipe ? sys_pipe(pipefd)
                    : (uix_errno = UIX_ENFILE, -1);
}

/* ── Process ────────────────────────────────────────────────── */
uix_pid_t uix_fork(void)
{
    extern long sys_fork(void) __attribute__((weak));
    return sys_fork ? (uix_pid_t)sys_fork()
                    : (uix_errno = UIX_EAGAIN, -1);
}

uix_pid_t uix_getpid(void)
{
    extern long sys_getpid(void) __attribute__((weak));
    return sys_getpid ? (uix_pid_t)sys_getpid() : 1;
}

uix_pid_t uix_getppid(void)
{
    extern long sys_getppid(void) __attribute__((weak));
    return sys_getppid ? (uix_pid_t)sys_getppid() : 0;
}

void uix_exit(int status)
{
    extern void sys_exit(int) __attribute__((weak));
    if (sys_exit) sys_exit(status);
    while (1) {}
}

int uix_execv(const char *path, char *const argv[])
{
    extern int sys_execve(const char *, char *const *, char *const *)
        __attribute__((weak));
    if (sys_execve) return sys_execve(path, argv, NULL);
    uix_errno = UIX_ENOEXEC; return -1;
}

int uix_execve(const char *path, char *const argv[], char *const envp[])
{
    extern int sys_execve(const char *, char *const *, char *const *)
        __attribute__((weak));
    if (sys_execve) return sys_execve(path, argv, envp);
    uix_errno = UIX_ENOEXEC; return -1;
}

int uix_execvp(const char *file, char *const argv[])
{
    return uix_execv(file, argv);
}

/* ── User/Group ─────────────────────────────────────────────── */
uix_uid_t uix_getuid(void)  { return 0; }
uix_uid_t uix_geteuid(void) { return 0; }
uix_gid_t uix_getgid(void)  { return 0; }
uix_gid_t uix_getegid(void) { return 0; }
int uix_setuid(uix_uid_t uid) { (void)uid; return 0; }
int uix_setgid(uix_gid_t gid) { (void)gid; return 0; }

/* ── Directory ──────────────────────────────────────────────── */
static char cwd_buf[UIX_PATH_MAX] = "/";

char *uix_getcwd(char *buf, uix_size_t size)
{
    uix_size_t len = uix_strlen(cwd_buf);
    if (len + 1 > size) { uix_errno = UIX_ERANGE; return NULL; }
    uix_strcpy(buf, cwd_buf);
    return buf;
}

int uix_chdir(const char *path)
{
    extern int sys_chdir(const char *) __attribute__((weak));
    if (sys_chdir) return sys_chdir(path);
    uix_strcpy(cwd_buf, path);
    return 0;
}

int uix_chroot(const char *path)
{
    extern int sys_chroot(const char *) __attribute__((weak));
    return sys_chroot ? sys_chroot(path)
                      : (uix_errno = UIX_EPERM, -1);
}

/* ── File system ────────────────────────────────────────────── */
int uix_access(const char *path, int mode)
{
    (void)path; (void)mode; return 0;
}

int uix_unlink(const char *path)
{
    extern int sys_unlink(const char *) __attribute__((weak));
    return sys_unlink ? sys_unlink(path)
                      : (uix_errno = UIX_ENOENT, -1);
}

int uix_rmdir(const char *path)
{
    extern int sys_rmdir(const char *) __attribute__((weak));
    return sys_rmdir ? sys_rmdir(path)
                     : (uix_errno = UIX_ENOENT, -1);
}

int uix_link(const char *oldpath, const char *newpath)
{
    extern int sys_link(const char *, const char *)
        __attribute__((weak));
    return sys_link ? sys_link(oldpath, newpath)
                    : (uix_errno = UIX_EPERM, -1);
}

int uix_symlink(const char *target, const char *linkpath)
{
    (void)target; (void)linkpath;
    uix_errno = UIX_EPERM; return -1;
}

uix_ssize_t uix_readlink(const char *path, char *buf, uix_size_t bufsiz)
{
    (void)path; (void)buf; (void)bufsiz;
    uix_errno = UIX_EINVAL; return -1;
}

/* ── Sleep ──────────────────────────────────────────────────── */
unsigned int uix_sleep(unsigned int seconds)
{
    /* In real UIOX: use setitimer / nanosleep syscall */
    volatile unsigned long cnt = seconds * 10000000UL;
    while (cnt--) {}
    return 0;
}

int uix_usleep(unsigned int usec)
{
    volatile unsigned long cnt = usec * 10UL;
    while (cnt--) {}
    return 0;
}

long uix_sysconf(int name)
{
    switch (name) {
    case 0:  return 4096;   /* _SC_ARG_MAX   */
    case 1:  return 64;     /* _SC_CHILD_MAX  */
    case 2:  return 1000;   /* _SC_CLK_TCK    */
    case 5:  return 128;    /* _SC_OPEN_MAX   */
    case 6:  return 4096;   /* _SC_PAGESIZE   */
    default: return -1;
    }
}

int uix_gethostname(char *name, uix_size_t len)
{
    const char *h = "uiox";
    if (uix_strlen(h) + 1 > len)
        { uix_errno = UIX_ENAMETOOLONG; return -1; }
    uix_strcpy(name, h);
    return 0;
}

int uix_isatty(int fd) { return fd < 3 ? 1 : 0; }

/* ***This is End of file, there is no more line should be added after this line*** */
