#ifndef __SYS_UIX_STAT__H
#define __SYS_UIX_STAT__H
/*
sys/stat.h in simplified form.
 Linux-style sys/stat.h in simplified form, plus the important note that it normally has no .c source file of its own.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* File type bits */
#define SIFMT   0170000
#define SIFSOCK 0140000
#define SIFLNK  0120000
#define SIFREG  0100000
#define SIFBLK  0060000
#define SIFDIR  0040000
#define SIFCHR  0020000
#define SIFIFO  0010000

/* Permission bits */
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

/* File type test macros */
#define SISREG(m)  (((m) & SIFMT) == SIFREG)
#define SISDIR(m)  (((m) & SIFMT) == SIFDIR)
#define SISCHR(m)  (((m) & SIFMT) == SIFCHR)
#define SISBLK(m)  (((m) & SIFMT) == SIFBLK)
#define SISFIFO(m) (((m) & SIFMT) == SIFIFO)
#define SISLNK(m)  (((m) & SIFMT) == SIFLNK)
#define SISSOCK(m) (((m) & SIFMT) == SIFSOCK)

/* Simplified Linux-style stat structure */
struct stat {
    devt     stdev;     // ID of device containing file /
    inot     stino;     // inode number /
    modet    stmode;    // file type and mode /
    nlinkt   stnlink;   // number of hard links /
    uidt     stuid;     // user ID of owner /
    gidt     stgid;     // group ID of owner /
    devt     strdev;    // device ID (if special file) /
    offt     stsize;    // total size, in bytes /
    blksizet stblksize; // blocksize for filesystem I/O /
    blkcntt  stblocks;  // number of 512B blocks allocated /
    timet    statime;   // time of last access /
    timet    stmtime;   // time of last modification /
    timet    stctime;   // time of last status change /
};

/* Function declarations */
int stat(const char pathname, struct stat buf);
int fstat(int fd, struct stat buf);
int lstat(const char pathname, struct stat buf);

int chmod(const char pathname, modet mode);
int fchmod(int fd, modet mode);
modet umask(modet mask);

int mkdir(const char pathname, modet mode);
int mkfifo(const char pathname, modet mode);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_UIX_STAT__H */
/* ***This is End of file, there is no more line should be added after this line*** */