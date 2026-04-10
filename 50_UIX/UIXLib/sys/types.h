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

#endif /* End of __SYS_TYPES__H */
/* ***This is End of file, there is no more line should be added after this line*** */