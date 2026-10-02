/*
 * 30_KIX/33_PCS/src/uiox_sys_mmap.c
 *
 * sys_mmap / sys_munmap — the file half of zero-copy.
 *
 *   THIS FILE      decides WHICH physical page backs the mapping
 *   uiox_mmap.c    puts that page into the process's page table
 *
 * ── the fd is the discriminator, and nothing else is ──────────────────
 *   fd >= 0 and not MAP_ANONYMOUS   →  file-backed, via vfs_mmap_page()
 *   MAP_ANONYMOUS, or no fd         →  anonymous DRAM
 *
 * @version 1.0.0  @date 2026-10-03
 */
#include "uiox_kix_pagemap.h"
#include "uiox_uaccess.h"
#include "uiox_klibc.h"

#define UIOX_MAP_SHARED    0x01u
#define UIOX_MAP_PRIVATE   0x02u
#define UIOX_MAP_FIXED     0x10u
#define UIOX_MAP_ANONYMOUS 0x20u

#define UIOX_PROT_NONE     0x00u
#define UIOX_PROT_READ     0x01u
#define UIOX_PROT_WRITE    0x02u
#define UIOX_PROT_EXEC     0x04u

#define UIOX_MAP_FAILED    ((void *)-1)

#define EINVAL  22

extern uintptr_t uiox_mm_phys_alloc(size_t size);
extern void      uiox_mm_phys_free (uintptr_t pa, size_t size);

extern uintptr_t uiox_mm_map_user_phys(struct uiox_proc *proc,
                                       uintptr_t         va_hint,
                                       uintptr_t         pa,
                                       size_t            size,
                                       uint32_t          prot);
extern int        uiox_mm_unmap_user   (struct uiox_proc *proc,
                                        uintptr_t         va,
                                        size_t            size);
extern struct uiox_proc *uiox_current_proc(void);

struct uiox_inode;
struct uiox_file;

extern struct uiox_file *fd_lookup_global(int fd);
extern uintptr_t vfs_mmap_page(struct uiox_inode *ip, uint64_t off);
extern long vfs_read(struct uiox_file *f, void *kbuf, size_t count);

long sys_mmap(void *addr, unsigned long len, int prot, int flags,
              int fd, long off)
{
    size_t    aligned_len;
    uintptr_t pa;
    int       file_backed;
    struct uiox_file *file = (struct uiox_file *)0;

    if (len == 0u) return (long)UIOX_MAP_FAILED;

    aligned_len = (len + UIOX_PAGE_SIZE - 1u) & UIOX_PAGE_MASK;
    file_backed = (!(flags & UIOX_MAP_ANONYMOUS)) && (fd >= 0);

    if (file_backed) {
        file = fd_lookup_global(fd);
        if (!file) return (long)UIOX_MAP_FAILED;

        pa = vfs_mmap_page(file->f_inode, (uint64_t)off);

        if (pa == 0u) {
            pa = uiox_mm_phys_alloc(aligned_len);
            if (pa == 0u) return (long)UIOX_MAP_FAILED;

            if (vfs_read(file, (void *)(uintptr_t)pa, len) < 0) {
                uiox_mm_phys_free(pa, aligned_len);
                return (long)UIOX_MAP_FAILED;
            }
        }
    } else {
        pa = uiox_mm_phys_alloc(aligned_len);
        if (pa == 0u) return (long)UIOX_MAP_FAILED;
    }

    {
        uintptr_t va = uiox_mm_map_user_phys(uiox_current_proc(),
                                             (uintptr_t)addr,
                                             pa, aligned_len,
                                             (uint32_t)prot);

        if (va == 0u) {
            if (!file_backed)
                uiox_mm_phys_free(pa, aligned_len);
            return (long)UIOX_MAP_FAILED;
        }
        return (long)va;
    }
}

long sys_munmap(void *addr, unsigned long len)
{
    if (len == 0u)                   return -EINVAL;
    if (!uiox_uaccess_ok(addr, len)) return -EINVAL;

    return (long)uiox_mm_unmap_user(uiox_current_proc(),
                                    (uintptr_t)addr, (size_t)len);
}
