/**
 * @file  uix_posix_proc.c
 * @brief UIOX POSIX — process syscall implementations.
 */

 #include "uix_posix_proc.h"

 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 /* ── Fork / exec ─────────────────────────────────────────────── */
 
 pid_t uix_fork(void)
 {
     return (pid_t)_ret(uix_syscall0(SYS_FORK));
 }
 
 pid_t uix_vfork(void)
 {
     return (pid_t)_ret(uix_syscall0(SYS_VFORK));
 }
 
 int uix_execve(const char *path, char *const argv[], char *const envp[])
 {
     return (int)_ret(uix_syscall3(SYS_EXECVE,
                                    (long)path, (long)argv, (long)envp));
 }
 
 int uix_execvp(const char *file, char *const argv[])
 {
     /* execvp inherits the current environment */
     extern char **uix_environ;  /* provided by uix_stdlib.c */
     return uix_execve(file, argv, uix_environ);
 }
 
 int uix_execvpe(const char *file, char *const argv[], char *const envp[])
 {
     return uix_execve(file, argv, envp);
 }
 
 /* ── Wait ────────────────────────────────────────────────────── */
 
 pid_t uix_wait(int *wstatus)
 {
     return uix_wait4(-1, wstatus, 0, NULL);
 }
 
 pid_t uix_waitpid(pid_t pid, int *wstatus, int options)
 {
     return uix_wait4(pid, wstatus, options, NULL);
 }
 
 pid_t uix_wait4(pid_t pid, int *wstatus,
                  int options, struct uix_rusage *rusage)
 {
     return (pid_t)_ret(uix_syscall4(SYS_WAIT4,
                         (long)pid, (long)wstatus,
                         (long)options, (long)rusage));
 }
 
 /* ── Identity ────────────────────────────────────────────────── */
 
 pid_t uix_getpid (void) { return (pid_t)uix_syscall0(SYS_GETPID); }
 pid_t uix_getppid(void) { return (pid_t)uix_syscall0(SYS_GETPPID); }
 uid_t uix_getuid (void) { return (uid_t)uix_syscall0(SYS_GETUID); }
 uid_t uix_geteuid(void) { return (uid_t)uix_syscall0(SYS_GETEUID); }
 gid_t uix_getgid (void) { return (gid_t)uix_syscall0(SYS_GETGID); }
 gid_t uix_getegid(void) { return (gid_t)uix_syscall0(SYS_GETEGID); }
 
 int uix_setuid (uid_t uid)  { return (int)_ret(uix_syscall1(SYS_SETUID, (long)uid)); }
 int uix_setgid (gid_t gid)  { return (int)_ret(uix_syscall1(SYS_SETGID, (long)gid)); }
 int uix_seteuid(uid_t euid) { return (int)_ret(uix_syscall1(SYS_SETEUID,(long)euid)); }
 int uix_setegid(gid_t egid) { return (int)_ret(uix_syscall1(SYS_SETEGID,(long)egid)); }
 
 /* ── Process groups ──────────────────────────────────────────── */
 
 int  uix_setpgid(pid_t pid, pid_t pgid)
 {
     return (int)_ret(uix_syscall2(SYS_SETPGID,(long)pid,(long)pgid));
 }
 
 pid_t uix_getpgid(pid_t pid)
 {
     return (pid_t)_ret(uix_syscall1(SYS_GETPGID,(long)pid));
 }
 
 pid_t uix_setsid(void)
 {
     return (pid_t)_ret(uix_syscall0(SYS_SETSID));
 }
 
 pid_t uix_getsid(pid_t pid)
 {
     return (pid_t)_ret(uix_syscall1(SYS_GETSID,(long)pid));
 }
 
 /* ── Signals ─────────────────────────────────────────────────── */
 
 int uix_kill(pid_t pid, int sig)
 {
     return (int)_ret(uix_syscall2(SYS_KILL,(long)pid,(long)sig));
 }
 
 int uix_raise(int sig)
 {
     return uix_kill(uix_getpid(), sig);
 }
 
 int uix_sigaction(int signum,
                    const struct uix_sigaction *act,
                    struct uix_sigaction *oldact)
 {
     return (int)_ret(uix_syscall3(SYS_SIGACTION,
                                    (long)signum,(long)act,(long)oldact));
 }
 
 int uix_sigprocmask(int how,
                      const uix_sigset_t *set,
                      uix_sigset_t *oldset)
 {
     return (int)_ret(uix_syscall3(SYS_SIGPROCMASK,
                                    (long)how,(long)set,(long)oldset));
 }
 
 unsigned int uix_alarm(unsigned int seconds)
 {
     return (unsigned int)uix_syscall1(SYS_ALARM,(long)seconds);
 }
 
 int uix_pause(void)
 {
     return (int)_ret(uix_syscall0(SYS_PAUSE));
 }
 
 /* ── Exit ────────────────────────────────────────────────────── */
 
 void uix_exit(int status)
 {
     uix_syscall1(SYS_EXIT, (long)status);
     __builtin_unreachable();
 }
 
 void uix__exit(int status)
 {
     uix_syscall1(SYS_EXIT, (long)status);
     __builtin_unreachable();
 }
 
 /* ── Resource limits ─────────────────────────────────────────── */
 
 int uix_getrlimit(int resource, struct uix_rlimit *rlim)
 {
     return (int)_ret(uix_syscall2(SYS_GETRLIMIT,
                                    (long)resource,(long)rlim));
 }
 
 int uix_setrlimit(int resource, const struct uix_rlimit *rlim)
 {
     return (int)_ret(uix_syscall2(SYS_SETRLIMIT,
                                    (long)resource,(long)rlim));
 }
 
 /* ── Working directory ───────────────────────────────────────── */
 
 int uix_chdir(const char *path)
 {
     return (int)_ret(uix_syscall1(SYS_CHDIR,(long)path));
 }
 
 int uix_fchdir(int fd)
 {
     return (int)_ret(uix_syscall1(SYS_FCHDIR,(long)fd));
 }
 
 int uix_chroot(const char *path)
 {
     return (int)_ret(uix_syscall1(SYS_CHROOT,(long)path));
 }
 
 char *uix_getcwd(char *buf, size_t size)
 {
     long r = uix_syscall2(SYS_GETCWD,(long)buf,(long)size);
     if (r < 0L) { uix_errno = (int)(-r); return NULL; }
     return buf;
 }
 
 int uix_umask(int mask)
 {
     return (int)uix_syscall1(SYS_UMASK,(long)mask);
 }
 
 int uix_nice(int inc)
 {
     return (int)_ret(uix_syscall1(SYS_NICE,(long)inc));
 }
 