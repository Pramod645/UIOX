#ifndef __UIX_FCNTL__H
#define __UIX_FCNTL__H
/*
fcntl.h header and a breakdown of what's inside.

fcntl.h — Header File (POSIX / glibc)

fcntl.h is typically a header-only interface, not a .c implementation file you’d write yourself. 
The actual function bodies for open, fcntl, creat, etc. live in the C library and ultimately call into the kernel.

*/
/* This is for only POXIS */

#include "uix_features.h"//??

#include "uix_types.h"

#define UIX_O_RDONLY    0x0000  // Open for reading only
#define UIX_O_WRONLY    0x0001   // Open for writing only
#define UIX_O_RDWR      0x0002   // Open for reading and writing
#define UIX_O_CREAT     0x0040  // Create file if it doesn't exist
#define UIX_O_EXCL      0x0080
#define UIX_O_NOCTTY    0x0100
#define UIX_O_TRUNC     0x0200   // Truncate file to zero length on open
#define UIX_O_APPEND    0x0400  // All writes go to end of file atomically
#define UIX_O_NONBLOCK  0x0800   // Non-blocking I/O mode
#define UIX_O_SYNC      0x1000
#define UIX_O_NOFOLLOW  0x2000
#define UIX_O_DIRECTORY 0x4000
#define UIX_O_CLOEXEC   0x80000   // Close on exec — prevents fd leak to child processes

#define UIX_F_DUPFD   0
#define UIX_F_GETFD   1 //  Get file descriptor flags (FD_CLOEXEC)
#define UIX_F_SETFD   2  //Set file descriptor flags
#define UIX_F_GETFL   3   // Get file status flags
#define UIX_F_SETFL   4    // Set file status flags
#define UIX_F_GETLK   5
#define UIX_F_SETLK   6
#define UIX_F_SETLKW  7
#define UIX_F_GETOWN  9
#define UIX_F_SETOWN  8

#define UIX_FD_CLOEXEC 1

#define UIX_F_RDLCK 0  // Shared/read lock
#define UIX_F_WRLCK 1   // Exclusive/write lock
#define UIX_F_UNLCK 2   // Unlock

typedef struct uix_flock {
    short     l_type;
    short     l_whence;
    uix_off_t l_start;
    uix_off_t l_len;
    uix_pid_t l_pid;
} uix_flock_t;

int uix_open (const char *path, int flags, ...);  // Opens or creates file, returns fd
int uix_creat(const char *path, uix_mode_t mode); // Equivalent to open(path, O_CREAT|O_WRONLY|O_TRUNC, mode)
int uix_fcntl(int fd, int cmd, ...); // File control — get/set flags, file locking


#endif /* End of __UIX_FCNTL__H */
/* ***This is End of file, there is no more line should be added after this line*** */
