
#ifndef __UIX_ERRNO__H
#define __UIX_ERRNO__H
/*
errno.h> works and what its typical structure and usage look like in POSIX systems.  

Overview
• <errno.h> provides the error reporting interface in C and POSIX.  
• It defines the errno variable (or macro) that stores an integer value when library/system calls fail.  
• It also defines macros for standard error codes like EACCES, ENOENT, EINVAL, etc.

This header is part of every POSIX-conforming system and included from the system's C library — but you can look at a simplified representation and see how it’s used.

errno.h (simplified illustrative version)
*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

/* Declare errno as an external variable */
extern int errno;

/* Common error codes (simplified for demonstration) */
#define EPERM           1   // Operation not permitted /
#define ENOENT          2   // No such file or directory /
#define ESRCH           3   // No such process /
#define EINTR           4   // Interrupted system call /
#define EIO             5   // I/O error /
#define ENXIO           6   // No such device or address /
#define E2BIG           7   // Argument list too long /
#define ENOEXEC         8   // Exec format error /
#define EBADF           9   // Bad file number /
#define ECHILD         10   // No child processes /
#define EAGAIN         11   // Try again /
#define ENOMEM         12   // Out of memory /
#define EACCES         13   // Permission denied /
#define EFAULT         14   // Bad address /
#define EBUSY          16   // Device or resource busy /
#define EEXIST         17   // File exists /
#define EXDEV          18   // Cross-device link /
#define ENODEV         19   // No such device /
#define ENOTDIR        20   // Not a directory /
#define EISDIR         21   // Is a directory /
#define EINVAL         22   // Invalid argument /

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __UIX_ERRNO__H */
/* ***This is End of file, there is no more line should be added after this line*** */