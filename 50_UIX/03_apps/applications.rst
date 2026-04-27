#1. 
Console:
stdin,stdout,stderr



---------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------
#2. Segments:File descriptor, 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

a.STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO

b.
#include <unistd.h> 
 ssize_t pread(int fd, void *buf, size_t num, off_t offset); 
 ssize_t pwrite(int fd, void *buf, size_t num, off_t offset); 
Returns: number of bytes read/written, -1 on error

#include <unistd.h> 
 int dup(int oldfd); 
 int dup2(int oldfd, int newfd); 
Returns: newfd, -1 on error

 #include <fcntl.h> 
 int fcntl(int fd, int cmd, ...); 
Returns: depends on cmd, -1 on error

#include <sys/ioctl.h> 
 int ioctl(int fd, unsigned long request, ...); 
Returns: depends on request, -1 on error

c.
#include <fcntl.h> 
 int creat(const char *pathname, mode_t mode); 
Returns: file descriptor if OK, -1 on error


#include <fcntl.h> 
 int creat(const char *pathname, mode_t mode); 
Returns: file descriptor if OK, -1 on error

#include <fcntl.h> 
 int open(const char *pathname, int oflag, ... /* mode_t mode */); 
 int openat(int dirfd, const char *pathname, int oflag, ... /* mode_t mode); 
Returns: file descriptor if OK, -1 on error

#include <unistd.h> 
 int close(int fd); 
Returns: 0 if OK, -1 on error

#include <unistd.h> 
 int close(int fd); 
Returns: 0 if OK, -1 on error

d.
#include <unistd.h> 
 ssize_t read(int fd, void *buf, size_t num); 
Returns: number of bytes read; 0 on EOF, -1 on error

 #include <unistd.h> 
 ssize_t write(int fd, void *buf, size_t num); 
Returns: number of bytes written if OK; -1 on error

 #include <sys/types.h> 
 #include <fcntl.h> 
 off_t lseek(int fd, off_t offset, int whence); 
Returns: new offset if OK; -1 on error
#3.
--------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------
#3.


#4.
--------------------------------------------------------------------------------------------------
==================================================================================================
#4.
#5.
#6.
#7.
#8.
#9.
#10.