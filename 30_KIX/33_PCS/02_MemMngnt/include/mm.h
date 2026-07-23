/*
 * 30_KIX/33_PCS/02_MemMngnt/include/mm.h
 * REMOVED: #include <stdint.h>, uix_types.h
 * REPLACED WITH: #include "uiox_klibc.h"
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include "uiox_klibc.h"

#define PROT_NONE     0x0
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MIN_BRK       0x1000
#define MAX_BRK       0x7FFFFFFF
#define BRK_ALIGN     4096

void         *kernel_mmap    (void *addr, size_t len, int prot,
                               int flags, int fd, uint64_t off);
int           kernel_munmap  (void *addr, size_t len);
int           kernel_mprotect(void *addr, size_t len, int prot);
uintptr_t     kernel_brk     (uintptr_t newbrk);

#endif /* KERNEL_MM_H */
