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

--------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------
#3.files and directory
a.
#include <sys/stat.h>
int stat(const char *restrict pathname, s truct stat *restrict buf );
int fstat(int fd, s truct stat *buf );
int lstat(const char *restrict pathname, s truct stat *restrict buf );
int fstatat(int fd, c onst char *restrict pathname,
struct stat *restrict buf, i nt flag);
All four return: 0 if OK,−1 on error

b.
#include <unistd.h>
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
Both return: 0 if OK,−1 on error

c.
#include <sys/stat.h>
mode_t umask(mode_t cmask);
Returns: previous file mode creation mask

d.
#include <sys/stat.h>
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);
All three return: 0 if OK,−1 on error

e.
Sticky Bit

f.
#include <unistd.h>
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group,
int flag);
int lchown(const char *pathname, uid_t owner, gid_t group);
All four return: 0 if OK,−1 on error

g.
#include <unistd.h>
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
Both return: 0 if OK,−1 on error

h.
File Systems

i.
#include <unistd.h>
int link(const char *existingpath, const char *newpath);
int linkat(int efd, const char *existingpath, int nfd, const char *newpath,
int flag);
Both return: 0 if OK,−1 on error

#include <unistd.h>
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
Both return: 0 if OK,−1 on error

#include <stdio.h>
int remove(const char *pathname);
Returns: 0 if OK,−1 on error

j.
#include <stdio.h>
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char *oldname, int newfd,
const char *newname);
Both return: 0 if OK,−1 on error

k.
#include <unistd.h>
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
Both return: 0 if OK,−1 on error

#include <unistd.h>
ssize_t readlink(const char* restrict pathname, char *restrict buf,
size_t bufsize);
ssize_t readlinkat(int fd, const char* restrict pathname,
char *restrict buf, size_t bufsize);
Both return: number of bytes read if OK,−1 on error

l.
#include <sys/stat.h>
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2],
int flag);
Both return: 0 if OK,−1 on error
==================================================================================================
m.4
#include <sys/stat.h>
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
Both return: 0 if OK,−1 on error

#include <unistd.h>
int rmdir(const char *pathname);
Returns: 0 if OK,−1 on error

n.
#include <dirent.h>
DIR *opendir(const char *pathname);
DIR *fdopendir(int fd);
Both return: pointer if OK, NULLon error
struct dirent *readdir(DIR *dp);
Returns: pointer if OK, NULLat end of directory or error
void rewinddir(DIR *dp);
int closedir(DIR *dp);
Returns: 0 if OK,−1 on error
long telldir(DIR *dp);
Returns: current location in directory associated with dp
void seekdir(DIR *dp, long loc);

o.
#include <unistd.h>
int chdir(const char *pathname);
int fchdir(int fd);
Both return: 0 if OK,−1 on error

#include <unistd.h>
char *getcwd(char *buf, size_t size);
Returns: buf if OK, NULLon error

==================================================================================================
==================================================================================================
#5.

==================================================================================================
==================================================================================================
#6.

==================================================================================================
==================================================================================================
#7.

==================================================================================================
==================================================================================================
#8.

==================================================================================================
==================================================================================================
#9.

==================================================================================================
==================================================================================================
#10.

==================================================================================================
==================================================================================================