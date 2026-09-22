/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_mmap.h
 *
 * Memory-mapped file I/O (Bach Ch.7 mapped access, via the page cache).
 *
 * @version 1.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_MMAP_H
#define UIOX_KIX_SCFS_MMAP_H

#include "uiox_kix_scfs_file.h"

/* PROT_* — memory protection */
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

/* MAP_* — mapping flags */
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

/* msync flags */
#define MS_ASYNC      0x01
#define MS_INVALIDATE 0x02
#define MS_SYNC       0x04

int  fs_munmap(uint32_t addr, uint32_t length);
int  fs_msync (uint32_t addr, uint32_t length, int flags);

#endif /* UIOX_KIX_SCFS_MMAP_H */
