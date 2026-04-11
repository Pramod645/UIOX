#include "sys/stat.h"

#include <unistd.h>
#include <sys/syscall.h>

int stat(const char pathname, struct stat buf)
{
    return syscall(SYSstat, pathname, buf);
}

int fstat(int fd, struct stat buf)
{
    return syscall(SYSfstat, fd, buf);
}

int lstat(const char pathname, struct stat buf)
{
    return syscall(SYSlstat, pathname, buf);
}

int chmod(const char pathname, modet mode)
{
    return syscall(SYSchmod, pathname, mode);
}

int fchmod(int fd, modet mode)
{
    return syscall(SYSfchmod, fd, mode);
}

modet umask(modet mask)
{
    return (modet)syscall(SYSumask, mask);
}

int mkdir(const char pathname, modet mode)
{
    return syscall(SYSmkdir, pathname, mode);
}

int mkfifo(const char pathname, modet mode)
{
    /*
      On Linux, mkfifo is usually implemented via mknod with SIFIFO,
      or via library logic depending on the libc.
     */
    return syscall(SYSmknod, pathname, SIFIFO | mode, 0);
}
/*
Important caveats

That .c file is only a teaching example. Real Linux/glibc code is more complicated because of:

• stat vs newfstatat
• 32-bit vs 64-bit ABIs
• large file support
• time64 transitions
• architecture-specific syscall numbers
• symbol versioning and compatibility layers

On modern Linux, many libc implementations route these through newer syscalls like:

• newfstatat
• fchmodat
• mkdirat

rather than directly using old SYSstat.

More accurate Linux reality

Real sys/stat.h often also includes:

• struct timespec timestamps or split nanosecond fields
• stat64
• fstatat
• lchmod on some systems, not Linux
• device number helper macros:
  - major()
  - minor()
  - makedev()

*/