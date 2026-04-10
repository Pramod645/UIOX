#ifndef __SYS_TYPES__H
#define __SYS_TYPES__H
/*
sys/types.h in the sense of a simplified libc-facing header that defines the common POSIX/Linux types and 
includes the standard underlying pieces.

A real distro’s sys/types.h is usually layered across multiple internal headers and varies by architecture, 
libc (glibc, musl), and feature macros. So the code below is a clean approximation.
*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

/* Basic integer types */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;

typedef signed short        int16_t;
typedef unsigned short      uint16_t;

typedef signed int          int32_t;
typedef unsigned int        uint32_t;

typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* Size-related types */
typedef unsigned long       size_t;
typedef signed long         ssize_t;

/* Pointer/integer types */
typedef signed long         intptr_t;
typedef unsigned long       uintptr_t;

/* Common system types */
typedef unsigned int        mode_t;
typedef long                off_t;
typedef int                 pid_t;
typedef long                time_t;
typedef long                clock_t;

/* Device/inode-like types */
typedef unsigned long       dev_t;
typedef unsigned long       ino_t;
typedef unsigned long       nlink_t;

/* User/group IDs */
typedef unsigned int        uid_t;
typedef unsigned int        gid_t;

/* Block/count types */
typedef long                blkcnt_t;
typedef long                blksize_t;

/* Boolean style */
#ifndef __cplusplus
typedef enum {
    false = 0,
    true = 1
} bool;
#endif

/* Null pointer */
#ifndef NULL
#define NULL ((void *)0)
#endif


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_TYPES__H */
/* ***This is End of file, there is no more line should be added after this line*** */