#ifndef __UIX_FCNTL__H
#define __UIX_FCNTL__H
/*
fcntl.h header and a breakdown of what's inside.

fcntl.h — Header File (POSIX / glibc)

fcntl.h is typically a header-only interface, not a .c implementation file you’d write yourself. 
The actual function bodies for open, fcntl, creat, etc. live in the C library and ultimately call into the kernel.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

/* Get modet, devt, _offt */
#include <bits/types.h>

/* Get O, F, FD flag definitions */
#include <bits/fcntl.h>

/* Detect if open() needs a mode argument */
#ifdef OTMPFILE
define _OPENNEEDSMODE(oflag) \
    (((oflag) & OCREAT) != 0 || ((oflag) & OTMPFILE) == OTMPFILE)
#else
define _OPENNEEDSMODE(oflag) (((oflag) & OCREAT) != 0)
#endif

/* Type definitions */
#ifndef _modetdefined
typedef modet modet;
define modetdefined
#endif

#ifndef offtdefined
ifndef USEFILEOFFSET64
typedef offt offt;
else
typedef off64t offt;
endif
define offtdefined
#endif

#if defined USELARGEFILE64 && !defined off64tdefined
typedef _off64t off64t;
define off64tdefined
#endif

#ifndef pidtdefined
typedef pidt pidt;
define pidtdefined
#endif

/* flock structure for advisory file locking */
struct flock {
    short int ltype;    // FRDLCK, FWRLCK, or FUNLCK /
    short int lwhence;  // SEEKSET, SEEKCUR, or SEEKEND /
    offt   lstart;   // Offset where the lock begins /
    offt   llen;     // Length of the locked region; 0 = to EOF /
    pidt   lpid;     // PID of process holding lock (FGETLK only) /
};

/* Function prototypes */
extern int open(const char file, int oflag, ...) _nonnull((1));
extern int openat(int fd, const char file, int oflag, ...) nonnull((2));
extern int creat(const char file, modet mode) _nonnull((1));
extern int fcntl(int fd, int cmd, ...);

#ifdef USEXOPEN2K8
extern int openat64(int fd, const char file, int oflag, ...) nonnull((2));
#endif


#endif /* End  of POXIS */

#ifdef fcntl
#ifdef _cplusplus
extern "C" {
#endif

#include <sys/types.h>

/* File access modes */
#define ORDONLY    0x0000
#define OWRONLY    0x0001
#define ORDWR      0x0002
#define OACCMODE   0x0003

// File creation/status flags /
#define OCREAT     0x0100
#define OEXCL      0x0200
#define ONOCTTY    0x0400
#define OTRUNC     0x0800
#define OAPPEND    0x1000
#define ONONBLOCK  0x2000
#define OSYNC      0x4000
#define OCLOEXEC   0x8000

// fcntl commands /
#define FDUPFD     0
#define FGETFD     1
#define FSETFD     2
#define FGETFL     3
#define FSETFL     4
#define FGETLK     5
#define FSETLK     6
#define FSETLKW    7

// Descriptor flags /
#define FDCLOEXEC  1

// Record locking /
#define FRDLCK     0
#define FWRLCK     1
#define FUNLCK     2

struct flock {
    short ltype;
    short lwhence;
    offt lstart;
    offt llen;
    pidt lpid;
};

int open(const char pathname, int flags, ...);
int creat(const char pathname, modet mode);
int fcntl(int fd, int cmd, ...);

#ifdef _cplusplus
}
#endif

#endif

#ifndef UIX_FCNTL_H
#define UIX_FCNTL_H

#include "uix_types.h"

#define UIX_O_RDONLY    0x0000
#define UIX_O_WRONLY    0x0001
#define UIX_O_RDWR      0x0002
#define UIX_O_CREAT     0x0040
#define UIX_O_EXCL      0x0080
#define UIX_O_NOCTTY    0x0100
#define UIX_O_TRUNC     0x0200
#define UIX_O_APPEND    0x0400
#define UIX_O_NONBLOCK  0x0800
#define UIX_O_SYNC      0x1000
#define UIX_O_NOFOLLOW  0x2000
#define UIX_O_DIRECTORY 0x4000
#define UIX_O_CLOEXEC   0x80000

#define UIX_F_DUPFD   0
#define UIX_F_GETFD   1
#define UIX_F_SETFD   2
#define UIX_F_GETFL   3
#define UIX_F_SETFL   4
#define UIX_F_GETLK   5
#define UIX_F_SETLK   6
#define UIX_F_SETLKW  7
#define UIX_F_GETOWN  9
#define UIX_F_SETOWN  8

#define UIX_FD_CLOEXEC 1

#define UIX_F_RDLCK 0
#define UIX_F_WRLCK 1
#define UIX_F_UNLCK 2

typedef struct uix_flock {
    short     l_type;
    short     l_whence;
    uix_off_t l_start;
    uix_off_t l_len;
    uix_pid_t l_pid;
} uix_flock_t;

int uix_open (const char *path, int flags, ...);
int uix_creat(const char *path, uix_mode_t mode);
int uix_fcntl(int fd, int cmd, ...);

#endif /* UIX_FCNTL_H */


#endif /* End of __UIX_FCNTL__H */
/* ***This is End of file, there is no more line should be added after this line*** */