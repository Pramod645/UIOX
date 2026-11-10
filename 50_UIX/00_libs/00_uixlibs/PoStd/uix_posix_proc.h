/**
 * @file  uix_posix_proc.h
 * @brief UIOX POSIX — process management syscall wrappers.
 *        fork, execve, wait4, exit, getpid, getppid, kill, sigaction,
 *        setpgid, setsid, getrlimit, setrlimit, chdir, chroot.
 */

 #ifndef UIX_POSIX_PROC_H
 #define UIX_POSIX_PROC_H
 
 #include "uix_signal.h"
 #include "uix_errno.h"
 #include "uix_string.h"

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Process creation / execution
  * ====================================================================== */
 
 uix_pid_t   uix_fork        (void);
 uix_pid_t   uix_vfork       (void);
 int     uix_execve      (const char *path,
                           char *const argv[], char *const envp[]);
 int     uix_execvp      (const char *file, char *const argv[]);
 int     uix_execvpe     (const char *file,
                           char *const argv[], char *const envp[]);
 
 /* =========================================================================
  * Process wait
  * ====================================================================== */
 
 uix_pid_t   uix_wait        (int *wstatus);
 uix_pid_t   uix_waitpid     (uix_pid_t pid, int *wstatus, int options);
 
 /* =========================================================================
  * Process identity
  * ====================================================================== */
 
 uix_pid_t   uix_getpid      (void);
 uix_pid_t   uix_getppid     (void);
 uix_uid_t   uix_getuid      (void);
 uix_uid_t   uix_geteuid     (void);
 uix_gid_t   uix_getgid      (void);
 uix_gid_t   uix_getegid     (void);
 int     uix_setuid      (uix_uid_t uid);
 int     uix_setgid      (uix_gid_t gid);
 int     uix_seteuid     (uix_uid_t euid);
 int     uix_setegid     (uix_gid_t egid);
 
 /* =========================================================================
  * Process groups / sessions
  * ====================================================================== */
 
 int     uix_setpgid     (uix_pid_t pid, uix_pid_t pgid);
 uix_pid_t   uix_getpgid     (uix_pid_t pid);
 uix_pid_t   uix_setsid      (void);
 uix_pid_t   uix_getsid      (uix_pid_t pid);
 
 /* =========================================================================
  * Signals
  * ====================================================================== */
 
 int     uix_kill        (uix_pid_t pid, int sig);
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
 
 
 /* =========================================================================
  * Misc process operations
  * ====================================================================== */
 
 int     uix_chdir       (const char *path);
 int     uix_fchdir      (int fd);
 int     uix_chroot      (const char *path);
 char   *uix_getcwd      (char *buf, uix_size_t size);
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
 