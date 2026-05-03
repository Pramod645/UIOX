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



#endif /* End of __UNISTD__H */
/* ***This is End of file, there is no more line should be added after this line*** */