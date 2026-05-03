
#ifndef __SYS_UIX_MMAN__H
#define __SYS_UIX_MMAN__H
/*
mman.h is another standard POSIX header, used for memory management functions such as mmap(), munmap(), mprotect(), and shmopen(). 


*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>   // for sizet, offt /
#include <fcntl.h>       // for O constants /


#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif


/* Protection flags for mmap() / mprotect() */
#define PROTNONE  0x0
#define PROTREAD  0x1
#define PROTWRITE 0x2
#define PROTEXEC  0x4

/* Flags for mmap() */
#define MAPSHARED    0x01
#define MAPPRIVATE   0x02
#define MAPFIXED     0x10
#define MAPANONYMOUS 0x20

/* Synchronization flags for msync() */
#define MSASYNC      1
#define MSINVALIDATE 2
#define MSSYNC       4

/* Advice flags for madvise() */
#define MADVNORMAL     0
#define MADVRANDOM     1
#define MADVSEQUENTIAL 2
#define MADVWILLNEED   3
#define MADVDONTNEED   4

/* Function prototypes */
void mmap(void addr, sizet length, int prot, int flags, int fd, offt offset);
int munmap(void addr, sizet length);
int mprotect(void addr, sizet length, int prot);
int msync(void addr, sizet length, int flags);
int shmopen(const char name, int oflag, modet mode);
int shmunlink(const char name);


#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */

#endif /* End of __SYS_UIX_MMAN__H */
/* ***This is End of file, there is no more line should be added after this line*** */