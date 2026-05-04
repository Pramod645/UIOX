
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

#ifndef UIX_MMAN_H
#define UIX_MMAN_H

#include "uix_types.h"

#define UIX_PROT_NONE  0x0
#define UIX_PROT_READ  0x1
#define UIX_PROT_WRITE 0x2
#define UIX_PROT_EXEC  0x4

#define UIX_MAP_SHARED    0x01
#define UIX_MAP_PRIVATE   0x02
#define UIX_MAP_FIXED     0x10
#define UIX_MAP_ANONYMOUS 0x20
#define UIX_MAP_ANON      UIX_MAP_ANONYMOUS
#define UIX_MAP_FAILED    ((void *)-1)

#define UIX_MS_ASYNC      1
#define UIX_MS_SYNC       4
#define UIX_MS_INVALIDATE 2

#define UIX_MADV_NORMAL     0
#define UIX_MADV_RANDOM     1
#define UIX_MADV_SEQUENTIAL 2
#define UIX_MADV_WILLNEED   3
#define UIX_MADV_DONTNEED   4

void  *uix_mmap      (void *addr, uix_size_t len, int prot, int flags,
                       int fd, uix_off_t offset);
int    uix_munmap    (void *addr, uix_size_t len);
int    uix_mprotect  (void *addr, uix_size_t len, int prot);
int    uix_msync     (void *addr, uix_size_t len, int flags);
int    uix_madvise   (void *addr, uix_size_t len, int advice);
int    uix_mlock     (const void *addr, uix_size_t len);
int    uix_munlock   (const void *addr, uix_size_t len);
int    uix_mlockall  (int flags);
int    uix_munlockall(void);
void  *uix_mremap    (void *old_addr, uix_size_t old_size,
                       uix_size_t new_size, int flags);
int    uix_shm_open  (const char *name, int oflag, uix_mode_t mode);
int    uix_shm_unlink(const char *name);

#endif /* UIX_MMAN_H */


#endif /* End of __SYS_UIX_MMAN__H */
/* ***This is End of file, there is no more line should be added after this line*** */