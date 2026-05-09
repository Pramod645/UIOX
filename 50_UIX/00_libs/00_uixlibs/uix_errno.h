
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

#include "uix_features.h"//??

#define UIX_EPERM     1     // Operation not permitted — requires root/capability
#define UIX_ENOENT    2     // File/directory does not exist
#define UIX_ESRCH     3
#define UIX_EINTR     4    // System call interrupted by signal — retry required
#define UIX_EIO       5    // Hardware I/O error
#define UIX_ENXIO     6    
#define UIX_E2BIG     7
#define UIX_ENOEXEC   8
#define UIX_EBADF     9
#define UIX_ECHILD    10
#define UIX_EAGAIN    11
#define UIX_ENOMEM    12
#define UIX_EACCES    13  // File permission denied
#define UIX_EFAULT    14
#define UIX_ENOTBLK   15
#define UIX_EBUSY     16   // Resource in use by another process
#define UIX_EEXIST    17
#define UIX_EXDEV     18
#define UIX_ENODEV    19
#define UIX_ENOTDIR   20
#define UIX_EISDIR    21
#define UIX_EINVAL    22    // Resource in use by another process
#define UIX_ENFILE    23    //System-wide open file limit reached
#define UIX_EMFILE    24    // Per-process open file limit reached
#define UIX_ENOTTY    25
#define UIX_ETXTBSY   26
#define UIX_EFBIG     27
#define UIX_ENOSPC    28    // No space left on device
#define UIX_ESPIPE    29   // Illegal seek on pipe/FIFO
#define UIX_EROFS     30
#define UIX_EMLINK    31
#define UIX_EPIPE     32   // Write to pipe with no readers — also sends SIGPIPE
#define UIX_EDOM      33
#define UIX_ERANGE    34
#define UIX_ENOSYS    38    // Function not implemented
#define UIX_ENOTEMPTY 39
#define UIX_ELOOP     40
#define UIX_ENAMETOOLONG 36
#define UIX_EDEADLK   35
#define UIX_ENOLCK    37
#define UIX_ENOTSUP   95

extern int uix_errno;    // Thread-local error code — set by system calls on failure

const char *uix_strerror(int errnum);   // Returns human-readable error string


#endif /* End of __UIX_ERRNO__H */
/* ***This is End of file, there is no more line should be added after this line*** */
