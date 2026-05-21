#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <stdint.h>
#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"

#define PROT_NONE    0x0
#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define PROT_EXEC    0x4
#define MAP_SHARED   0x01
#define MAP_PRIVATE  0x02
#define MAP_FIXED    0x10
#define MAP_ANONYMOUS 0x20
#define MIN_BRK      0x1000
#define MAX_BRK      0x7FFFFFFF
#define BRK_ALIGN    4096

void          *kernel_mmap    (void *addr, uix_size_t len, int prot,
                                int flags, int fd, uix_off_t off);
int            kernel_munmap  (void *addr, uix_size_t len);
int            kernel_mprotect(void *addr, uix_size_t len, int prot);
uix_uintptr_t  kernel_brk     (uix_uintptr_t newbrk);

#endif /* KERNEL_MM_H */
