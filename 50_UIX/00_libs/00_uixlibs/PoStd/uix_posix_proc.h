/**
 * @file  uix_posix_proc.h
 * @brief UIOX POSIX — process management syscall wrappers.
 *        fork, execve, wait4, exit, getpid, getppid, kill, sigaction,
 *        setpgid, setsid, getrlimit, setrlimit, chdir, chroot.
 */

 #ifndef UIX_POSIX_PROC_H
 #define UIX_POSIX_PROC_H
 
 #include "uix_syscall.h"
 #include "uix_signal.h"
 #include "../sys/uix_types.h"
 #include "../sys/uix_wait.h"
 #include "../sys/uix_resource.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Process creation / execution
  * ====================================================================== */
 
 pid_t   uix_fork        (void);
 pid_t   uix_vfork       (void);
 int     uix_execve      (const char *path,
                           char *const argv[], char *const envp[]);
 int     uix_execvp      (const char *file, char *const argv[]);
 int     uix_execvpe     (const char *file,
                           char *const argv[], char *const envp[]);
 
 /* =========================================================================
  * Process wait
  * ====================================================================== */
 
 pid_t   uix_wait        (int *wstatus);
 pid_t   uix_waitpid     (pid_t pid, int *wstatus, int options);
 pid_t   uix_wait4       (pid_t pid, int *wstatus,
                           int options, struct uix_rusage *rusage);
 
 /* =========================================================================
  * Process identity
  * ====================================================================== */
 
 pid_t   uix_getpid      (void);
 pid_t   uix_getppid     (void);
 uid_t   uix_getuid      (void);
 uid_t   uix_geteuid     (void);
 gid_t   uix_getgid      (void);
 gid_t   uix_getegid     (void);
 int     uix_setuid      (uid_t uid);
 int     uix_setgid      (gid_t gid);
 int     uix_seteuid     (uid_t euid);
 int     uix_setegid     (gid_t egid);
 
 /* =========================================================================
  * Process groups / sessions
  * ====================================================================== */
 
 int     uix_setpgid     (pid_t pid, pid_t pgid);
 pid_t   uix_getpgid     (pid_t pid);
 pid_t   uix_setsid      (void);
 pid_t   uix_getsid      (pid_t pid);
 
 /* =========================================================================
  * Signals
  * ====================================================================== */
 
 int     uix_kill        (pid_t pid, int sig);
 int     uix_raise       (int sig);
 int     uix_sigaction   (int signum,
                           const struct uix_sigaction *act,
                           struct uix_sigaction *oldact);
 int     uix_sigprocmask (int how,
                           const uix_sigset_t *set,
                           uix_sigset_t *oldset);
 unsigned int uix_alarm  (unsigned int seconds);
 int     uix_pause       (void);
 
 /* =========================================================================
  * Exit
  * ====================================================================== */
 
 void    uix_exit        (int status) __attribute__((noreturn));
 void    uix__exit       (int status) __attribute__((noreturn));
 
 /* =========================================================================
  * Resource limits
  * ====================================================================== */
 
 int     uix_getrlimit   (int resource, struct uix_rlimit *rlim);
 int     uix_setrlimit   (int resource, const struct uix_rlimit *rlim);
 
 /* =========================================================================
  * Misc process operations
  * ====================================================================== */
 
 int     uix_chdir       (const char *path);
 int     uix_fchdir      (int fd);
 int     uix_chroot      (const char *path);
 char   *uix_getcwd      (char *buf, size_t size);
 int     uix_umask       (int mask);
 int     uix_nice        (int inc);
 
 /* POSIX aliases */
 #define fork        uix_fork
 #define vfork       uix_vfork
 #define execve      uix_execve
 #define execvp      uix_execvp
 #define wait        uix_wait
 #define waitpid     uix_waitpid
 #define getpid      uix_getpid
 #define getppid     uix_getppid
 #define getuid      uix_getuid
 #define geteuid     uix_geteuid
 #define getgid      uix_getgid
 #define getegid     uix_getegid
 #define setuid      uix_setuid
 #define setgid      uix_setgid
 #define kill        uix_kill
 #define sigaction   uix_sigaction
 #define sigprocmask uix_sigprocmask
 #define exit        uix_exit
 #define _exit       uix__exit
 #define chdir       uix_chdir
 #define getcwd      uix_getcwd
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIX_POSIX_PROC_H */
 