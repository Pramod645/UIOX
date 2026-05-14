#ifndef __UIX_UNISTD__H
#define __UIX_UNISTD__H
/*
unistd.h is one of the most fundamental POSIX headers.  
It defines constants, types, and function prototypes for system calls that provide access to the operating system’s 
API — i.e., file operations, process control, environmental queries, and more.

*/
/* This is for only POXIS */

//#include "features.h"


#include "../sys/uix_types.h"

#define UIX_STDIN_FILENO  0       // File descriptor for standard input
#define UIX_STDOUT_FILENO 1       // File descriptor for standard output
#define UIX_STDERR_FILENO 2        // File descriptor for standard error

#define UIX_F_OK 0     // Test for file existence in access()
#define UIX_X_OK 1    // Test read permission
#define UIX_W_OK 2     // Test write permission
#define UIX_R_OK 4     // Test execute permission

#define UIX_SEEK_SET 0
#define UIX_SEEK_CUR 1
#define UIX_SEEK_END 2

uix_ssize_t  uix_read     (int fd, void *buf, uix_size_t count);  // Reads up to n bytes from fd
uix_ssize_t  uix_write    (int fd, const void *buf, uix_size_t count);  // Writes n bytes to fd
int          uix_close    (int fd);                                      // Closes file descriptor
uix_off_t    uix_lseek    (int fd, uix_off_t offset, int whence);  // Repositions fd offset
int          uix_dup      (int oldfd);                            // Duplicates fd to lowest available
int          uix_dup2     (int oldfd, int newfd);                 // Duplicates old to specific new fd
int          uix_pipe     (int pipefd[2]);           // Creates unidirectional pipe — fds[0]=read, fds[1]=write

uix_pid_t    uix_fork     (void);       // Creates child process — returns pid to parent, 0 to child
uix_pid_t    uix_getpid   (void);   // // Returns process ID
uix_pid_t    uix_getppid  (void);  // Returns parent process ID
void         uix_exit     (int status) __attribute__((noreturn));
int          uix_execv    (const char *path, char *const argv[]);    // Replaces process image — never returns on success
int          uix_execve   (const char *path, char *const argv[],
                            char *const envp[]);
int          uix_execvp   (const char *file, char *const argv[]);  // exec with PATH search

uix_uid_t    uix_getuid   (void);    // Returns real user ID
uix_uid_t    uix_geteuid  (void);    // Returns effective user ID
uix_gid_t    uix_getgid   (void);
uix_gid_t    uix_getegid  (void);
int          uix_setuid   (uix_uid_t uid);  // Sets user ID — POSIX
int          uix_setgid   (uix_gid_t gid);

char        *uix_getcwd   (char *buf, uix_size_t size);  // Gets current working directory path
int          uix_chdir    (const char *path);  // Changes working directory
int          uix_chroot   (const char *path);   // Changes root directory — requires root

int          uix_access   (const char *path, int mode); // Checks file accessibility
int          uix_unlink   (const char *path); // Removes file — decrements link count
int          uix_rmdir    (const char *path);  // Removes empty directory
int          uix_link     (const char *oldpath, const char *newpath);   // Creates hard link
int          uix_symlink  (const char *target, const char *linkpath);   // Creates symbolic link
uix_ssize_t  uix_readlink (const char *path, char *buf, uix_size_t bufsiz);

unsigned int uix_sleep    (unsigned int seconds);  // Sleeps for seconds
int          uix_usleep   (unsigned int usec);   // Sleeps for microseconds — POSIX
unsigned int uix_alarm    (unsigned int seconds); // Schedules SIGALRM after sec seconds

long         uix_sysconf  (int name);             // Runtime system configuration values
int          uix_gethostname(char *name, uix_size_t len);   /// Gets system hostname
int          uix_isatty   (int fd);   // Returns 1 if fd refers to terminal

uix_pid_t    uix_getpgrp  (void);   // Returns process group ID
int          uix_setpgrp  (void);
uix_pid_t    uix_getpgid  (uix_pid_t pid);
int          uix_setpgid  (uix_pid_t pid, uix_pid_t pgid);
uix_pid_t    uix_setsid   (void);            // Creates new session, detaches from terminal



#endif /* End of __UIX_UNISTD__H */
/* ***This is End of file, there is no more line should be added after this line*** */
