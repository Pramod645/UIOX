#ifndef __SYS_UIX_TYPES__H
#define __SYS_UIX_TYPES__H
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
typedef signed char         int8t;
typedef unsigned char       uint8t;

typedef signed short        int16t;
typedef unsigned short      uint16t;

typedef signed int          int32t;
typedef unsigned int        uint32t;

typedef signed long long    int64t;
typedef unsigned long long  uint64t;

/* Size-related types */
typedef unsigned long       sizet;
typedef signed long         ssizet;

/* Pointer/integer types */
typedef signed long         intptrt;
typedef unsigned long       uintptrt;

/* Common system types */
typedef unsigned int        modet;
typedef long                offt;
typedef int                 pidt;
typedef long                timet;
typedef long                clockt;

/* Device/inode-like types */
typedef unsigned long       devt;
typedef unsigned long       inot;
typedef unsigned long       nlinkt;

/* User/group IDs */
typedef unsigned int        uidt;
typedef unsigned int        gidt;

/* Block/count types */
typedef long                blkcntt;
typedef long                blksizet;

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

#ifndef UIX_TYPES_H // Include guard — prevents double inclusion, standard practice in all POSIX headers
#define UIX_TYPES_H

typedef unsigned char      uix_uint8_t; // Maps to uint8_t from <stdint.h> — 8-bit unsigned integer, POSIX requirement
typedef signed   char      uix_int8_t; // Maps to int8_t — signed 8-bit, C99/POSIX
typedef unsigned short     uix_uint16_t; // Maps to uint16_t — 16-bit unsigned
typedef signed   short     uix_int16_t;
typedef unsigned int       uix_uint32_t; // Maps to uint32_t — 32-bit unsigned
typedef signed   int       uix_int32_t;
typedef unsigned long long uix_uint64_t; // Maps to uint64_t — 64-bit unsigned
typedef signed   long long uix_int64_t;

typedef unsigned long  uix_size_t; // Maps to size_t from <stddef.h> — result of sizeof, used in all memory/string functions
typedef signed   long  uix_ssize_t; // Maps to ssize_t — POSIX signed size, returned by read()/write()
typedef unsigned long  uix_uintptr_t;
typedef signed   long  uix_ptrdiff_t;

typedef int            uix_pid_t; // Maps to pid_t — POSIX process ID type, used in fork(), getpid(), kill()
typedef unsigned int   uix_uid_t; // Maps to uid_t — POSIX user ID, used in getuid(), setuid(), chown()
typedef unsigned int   uix_gid_t; // Maps to gid_t — POSIX group ID, used in getgid(), setgid()
typedef unsigned int   uix_mode_t; // Maps to mode_t — POSIX file permission bits, used in open(), mkdir(), chmod()
typedef unsigned long  uix_ino_t; // Maps to ino_t — POSIX inode number in struct stat
typedef long           uix_off_t; // Maps to off_t — POSIX file offset, used in lseek(), fseek()
typedef unsigned long  uix_dev_t; // Maps to dev_t — POSIX device number, identifies block/char devices
typedef unsigned long  uix_nlink_t; // 
typedef long           uix_time_t; // Maps to time_t — seconds since Unix epoch (Jan 1 1970 UTC), used in time()
typedef long           uix_clock_t;
typedef unsigned int   uix_blksize_t;
typedef unsigned long  uix_blkcnt_t;

typedef int            uix_bool_t; // Boolean type, equivalent to _Bool in C99 or stdbool.h
#define UIX_TRUE  1 // Boolean true constant
#define UIX_FALSE 0

#ifndef NULL
#define NULL ((void*)0) /* POSIX/C null pointer constant */
#endif

#endif /* UIX_TYPES_H */


#endif /* End of __SYS_UIX_TYPES__H */
/* ***This is End of file, there is no more line should be added after this line*** */