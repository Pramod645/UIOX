#ifndef __UNISTD__H
#define __UNISTD__H
/*
unistd.h is one of the most fundamental POSIX headers.  
It defines constants, types, and function prototypes for system calls that provide access to the operating system’s 
API — i.e., file operations, process control, environmental queries, and more.

*/
/* This is for only POXIS */

#include "features.h"

#include <stddef.h>  // sizet
#include <sys/types.h>  // pidt, uidt, etc.

#if  (define __POSIX)


//File and I/O

int     access(const char pathname, int mode);
int     close(int fd);
ssizet read(int fd, void buf, sizet count);
ssizet write(int fd, const void buf, sizet count);
offt   lseek(int fd, offt offset, int whence);
int     unlink(const char pathname);
int     rmdir(const char pathname);
int     isatty(int fd);
`

//Process Control
pidt fork(void);
int   execv(const char path, char const argv[]);
int   execvp(const char file, char const argv[]);
pidt getpid(void);
pidt getppid(void);
int   pipe(int fd[2]);
unsigned int sleep(unsigned int seconds);


//Environment and User Information

char  getcwd(char buf, sizet size);
char  getenv(const char name);
int    setenv(const char name, const char value, int overwrite);
int    unsetenv(const char name);
uidt  getuid(void);
gidt  getgid(void);
char  ttyname(int fd);

//System Configuration

long sysconf(int name);
sizet confstr(int name, char buf, sizet len);
int pathconf(const char path, int name);
int fpathconf(int fd, int name);

/////////////////////
#define SEEKSET 0
#define SEEKCUR 1
#define SEEKEND 2

// File operations /
int open(const char pathname, int flags, ...);
int close(int fd);
ssizet read(int fd, void buf, sizet count);
ssizet write(int fd, const void buf, sizet count);
offt lseek(int fd, offt offset, int whence);

// Process control /
pidt fork(void);
int execv(const char path, char const argv[]);
pidt getpid(void);
pidt getppid(void);

// Sleep /
unsigned int sleep(unsigned int seconds);

// Working directory /
char getcwd(char buf, sizet size);

#endif /* End  of POXIS */

#ifndef UIX_UNISTD_H
#define UIX_UNISTD_H

#include "uix_types.h"

#define UIX_STDIN_FILENO  0
#define UIX_STDOUT_FILENO 1
#define UIX_STDERR_FILENO 2

#define UIX_F_OK 0
#define UIX_X_OK 1
#define UIX_W_OK 2
#define UIX_R_OK 4

#define UIX_SEEK_SET 0
#define UIX_SEEK_CUR 1
#define UIX_SEEK_END 2

uix_ssize_t  uix_read     (int fd, void *buf, uix_size_t count);
uix_ssize_t  uix_write    (int fd, const void *buf, uix_size_t count);
int          uix_close    (int fd);
uix_off_t    uix_lseek    (int fd, uix_off_t offset, int whence);
int          uix_dup      (int oldfd);
int          uix_dup2     (int oldfd, int newfd);
int          uix_pipe     (int pipefd[2]);

uix_pid_t    uix_fork     (void);
uix_pid_t    uix_getpid   (void);
uix_pid_t    uix_getppid  (void);
void         uix_exit     (int status) __attribute__((noreturn));
int          uix_execv    (const char *path, char *const argv[]);
int          uix_execve   (const char *path, char *const argv[],
                            char *const envp[]);
int          uix_execvp   (const char *file, char *const argv[]);

uix_uid_t    uix_getuid   (void);
uix_uid_t    uix_geteuid  (void);
uix_gid_t    uix_getgid   (void);
uix_gid_t    uix_getegid  (void);
int          uix_setuid   (uix_uid_t uid);
int          uix_setgid   (uix_gid_t gid);

char        *uix_getcwd   (char *buf, uix_size_t size);
int          uix_chdir    (const char *path);
int          uix_chroot   (const char *path);

int          uix_access   (const char *path, int mode);
int          uix_unlink   (const char *path);
int          uix_rmdir    (const char *path);
int          uix_link     (const char *oldpath, const char *newpath);
int          uix_symlink  (const char *target, const char *linkpath);
uix_ssize_t  uix_readlink (const char *path, char *buf, uix_size_t bufsiz);

unsigned int uix_sleep    (unsigned int seconds);
int          uix_usleep   (unsigned int usec);
unsigned int uix_alarm    (unsigned int seconds);

long         uix_sysconf  (int name);
int          uix_gethostname(char *name, uix_size_t len);
int          uix_isatty   (int fd);

uix_pid_t    uix_getpgrp  (void);
int          uix_setpgrp  (void);
uix_pid_t    uix_getpgid  (uix_pid_t pid);
int          uix_setpgid  (uix_pid_t pid, uix_pid_t pgid);
uix_pid_t    uix_setsid   (void);

#endif /* UIX_UNISTD_H */


#endif /* End of __UNISTD__H */
/* ***This is End of file, there is no more line should be added after this line*** */