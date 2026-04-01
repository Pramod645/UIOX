//can you provide the header file and source code for this stat.h file?
Here’s a Linux-style sys/stat.h in simplified form, plus the important note that it normally has no .c source file of its own.

sys/stat.h

``c
#ifndef SYSSTATH
#define SYSSTATH 1

#ifdef _cplusplus
extern "C" {
#endif

#include <sys/types.h>

/ File type bits /
#define SIFMT   0170000
#define SIFSOCK 0140000
#define SIFLNK  0120000
#define SIFREG  0100000
#define SIFBLK  0060000
#define SIFDIR  0040000
#define SIFCHR  0020000
#define SIFIFO  0010000

/ Permission bits /
#define SISUID  0004000
#define SISGID  0002000
#define SISVTX  0001000

#define SIRUSR  0000400
#define SIWUSR  0000200
#define SIXUSR  0000100

#define SIRGRP  0000040
#define SIWGRP  0000020
#define SIXGRP  0000010

#define SIROTH  0000004
#define SIWOTH  0000002
#define SIXOTH  0000001

#define SIRWXU (SIRUSR | SIWUSR | SIXUSR)
#define SIRWXG (SIRGRP | SIWGRP | SIXGRP)
#define SIRWXO (SIROTH | SIWOTH | SIXOTH)

/ File type test macros /
#define SISREG(m)  (((m) & SIFMT) == SIFREG)
#define SISDIR(m)  (((m) & SIFMT) == SIFDIR)
#define SISCHR(m)  (((m) & SIFMT) == SIFCHR)
#define SISBLK(m)  (((m) & SIFMT) == SIFBLK)
#define SISFIFO(m) (((m) & SIFMT) == SIFIFO)
#define SISLNK(m)  (((m) & SIFMT) == SIFLNK)
#define SISSOCK(m) (((m) & SIFMT) == SIFSOCK)

/ Simplified Linux-style stat structure /
struct stat {
    devt     stdev;     / ID of device containing file /
    inot     stino;     / inode number /
    modet    stmode;    / file type and mode /
    nlinkt   stnlink;   / number of hard links /
    uidt     stuid;     / user ID of owner /
    gidt     stgid;     / group ID of owner /
    devt     strdev;    / device ID (if special file) /
    offt     stsize;    / total size, in bytes /
    blksizet stblksize; / blocksize for filesystem I/O /
    blkcntt  stblocks;  / number of 512B blocks allocated /
    timet    statime;   / time of last access /
    timet    stmtime;   / time of last modification /
    timet    stctime;   / time of last status change /
};

/ Function declarations /
int stat(const char pathname, struct stat buf);
int fstat(int fd, struct stat buf);
int lstat(const char pathname, struct stat buf);

int chmod(const char pathname, modet mode);
int fchmod(int fd, modet mode);
modet umask(modet mask);

int mkdir(const char pathname, modet mode);
int mkfifo(const char pathname, modet mode);

#ifdef _cplusplus
}
#endif

#endif / SYSSTATH /
`

stat.c source?

Usually, there is no standalone stat.c paired with sys/stat.h just because of the header. The header only declares:

• constants
• macros
• struct stat
• function prototypes

The actual implementations of stat(), fstat(), chmod(), mkdir(), and so on are part of the C library, typically wrapping Linux syscalls.

If you want a minimal illustrative implementation, here is one.

Example stat.c

`c
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
    /
      On Linux, mkfifo is usually implemented via mknod with SIFIFO,
      or via library logic depending on the libc.
     /
    return syscall(SYSmknod, pathname, SIFIFO | mode, 0);
}
`

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

If you want, I can give you a matching mini-libc set next so all the headers fit together:

• sys/types.h
• sys/stat.h
• fcntl.h
• unistd.h`

with consistent typedefs and syscall wrappers.