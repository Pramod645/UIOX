/*
 * 30_KIX/33_PCS/src/uiox_sys_mmap.c(new — zero-copy FS mmap)
 *
 * sys_mmap — maps file data or anonymous DRAM directly
 * into user page table. Zero copy — no copy_to_user needed.
 *
 * Two paths:
 *
 * [1] File-backed mmap (MAP_SHARED):
 *     vfs_mmap_page(file, off) → physical page PA
 *     uiox_mm_map_user_phys(proc, va, PA, size, prot)
 *     user reads VA → reads DRAM directly (same physical page)
 *
 * [2] Anonymous mmap (MAP_ANONYMOUS):
 *     uiox_mm_phys_alloc(size) → fresh zeroed DRAM pages
 *     uiox_mm_map_user_phys(proc, va, PA, size, prot)
 *     user reads/writes VA → reads/writes DRAM directly
 *
 * @version 1.0.0  @date 2026-07-29
 */

#include "uiox_syscall.h"
#include "uiox_uaccess.h"
#include "uiox_vfs.h"
#include "uiox_soc_string.h"
#include "uiox_soc_stdio.h"

/* mmap flags */
#define UIOX_MAP_SHARED    0x01u
#define UIOX_MAP_PRIVATE   0x02u
#define UIOX_MAP_ANONYMOUS 0x20u
#define UIOX_MAP_FIXED     0x10u

/* mmap protection */
#define UIOX_PROT_NONE     0x00u
#define UIOX_PROT_READ     0x01u
#define UIOX_PROT_WRITE    0x02u
#define UIOX_PROT_EXEC     0x04u

#define PAGE_SIZE          4096u
#define PAGE_ALIGN_UP(x)   (((uintptr_t)(x) + PAGE_SIZE - 1u) \
                             & ~(PAGE_SIZE - 1u))

/* ── Physical memory allocator stub ────────────────────────────────── */
/*
 * In production: calls uiox_mm_phys_alloc() from 33_PCS/02_MemMngnt.
 * Returns zeroed physical pages.
 */
extern uintptr_t uiox_mm_phys_alloc(size_t size);
extern void      uiox_mm_phys_free (uintptr_t pa, size_t size);
extern uintptr_t uiox_mm_map_user_phys(void     *proc,
                                        uintptr_t va_hint,
                                        uintptr_t pa,
                                        size_t    size,
                                        uint32_t  prot);
extern void     *uiox_current_proc(void);

/* ── sys_mmap ──────────────────────────────────────────────────────── */
uiox_syscall_ret_t sys_mmap(void    *addr,
                              size_t   len,
                              int      prot,
                              int      flags,
                              int      fd,
                              long     off)
{
    if (len == 0u)
        return UIOX_SYSCALL_ERR(-22);   /* EINVAL */

    size_t    aligned_len = PAGE_ALIGN_UP(len);
    uintptr_t pa          = 0u;
    int       file_backed = !(flags & UIOX_MAP_ANONYMOUS) && (fd >= 0);

    if (file_backed) {
        /*
         * File-backed mmap — zero copy.
         *
         * Step 1: get physical address of file page from VFS.
         *         vfs_mmap_page() calls scfs_mmap_page() which
         *         returns the PA of the DRAM page holding file data.
         *
         * Step 2: insert PTE into user page table pointing at that PA.
         *         User then reads the file data directly from DRAM —
         *         NO copy_to_user, NO memcpy.
         */
        extern uiox_file_t *fd_lookup_global(int fd);
        uiox_file_t *file = fd_lookup_global(fd);
        if (!file) return UIOX_SYSCALL_ERR(-9);  /* EBADF */

        pa = vfs_mmap_page(file, (uint64_t)off);
        if (pa == 0u) {
            /*
             * File doesn't support direct page mapping —
             * fall back to copy-based mmap:
             * allocate anonymous pages + read file into them.
             */
            pa = uiox_mm_phys_alloc(aligned_len);
            if (pa == 0u) return UIOX_SYSCALL_ERR(-12);  /* ENOMEM */

            /* Read file data into freshly allocated DRAM pages */
            uint8_t *kbuf = (uint8_t *)(uintptr_t)pa;
            vfs_read(file, kbuf, len);
        }
    } else {
        /*
         * Anonymous mmap — allocate fresh zeroed DRAM pages.
         * uiox_mm_phys_alloc zeroes the pages before returning.
         */
        pa = uiox_mm_phys_alloc(aligned_len);
        if (pa == 0u) return UIOX_SYSCALL_ERR(-12);  /* ENOMEM */
    }

    /*
     * Step 3: map physical pages into user page table.
     * After this returns, user VA → physical DRAM page.
     * No further kernel involvement for reads/writes.
     */
    void     *proc = uiox_current_proc();
    uintptr_t va   = uiox_mm_map_user_phys(proc,
                                             (uintptr_t)addr,
                                             pa,
                                             aligned_len,
                                             (uint32_t)prot);
    if (va == 0u) {
        if (!file_backed)
            uiox_mm_phys_free(pa, aligned_len);
        return UIOX_SYSCALL_ERR(-12);  /* ENOMEM */
    }

    return (uiox_syscall_ret_t)va;
}

/* ── sys_munmap ────────────────────────────────────────────────────── */
uiox_syscall_ret_t sys_munmap(void *addr, size_t len)
{
    if (!uiox_uaccess_ok(addr, len))
        return UIOX_SYSCALL_ERR(-22);  /* EINVAL */

    /*
     * Remove PTE from user page table.
     * Physical pages are NOT freed here if file-backed
     * (they belong to the page cache).
     * Anonymous pages are freed by tracking in VMA list.
     */
    /* TODO: uiox_mm_unmap_user(current_proc, va, len) */
    (void)addr; (void)len;
    return 0;
}
