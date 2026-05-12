#ifndef __SYS_UIX_TYPES__H
#define __SYS_UIX_TYPES__H
/*
sys/types.h in the sense of a simplified libc-facing header that defines the common POSIX/Linux types and 
includes the standard underlying pieces.

A real distro’s sys/types.h is usually layered across multiple internal headers and varies by architecture, 
libc (glibc, musl), and feature macros. So the code below is a clean approximation.
*/
/* This is for only POXIS */

//#include "uix_features.h"//?

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


#endif /* End of __SYS_UIX_TYPES__H */
/* ***This is End of file, there is no more line should be added after this line*** */
