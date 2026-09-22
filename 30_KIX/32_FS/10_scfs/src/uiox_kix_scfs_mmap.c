/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mmap.c
 *
 * mmap / munmap
 *
 * Bach Ch.7 — mapped file access; the page cache supplies the backing pages.
 * msync() (the flush half of this family) lives in uiox_kix_scfs_sync.c with
 * the other durability calls.  A file mapping resolves the fd; an anonymous
 * mapping (MAP_ANONYMOUS) skips the fd entirely.
 *
 * @version 1.0.0  @date 2026-09-21
 */
#include "uiox_kix_scfs_internal.h"

/* mmap() — map a file (or anonymous memory) into the process address space. */
long uiox_kix_scfs_mmap(uiox_reg_t fd, uiox_reg_t addr, uiox_reg_t length,
                        uiox_reg_t prot, uiox_reg_t flags, uiox_reg_t offset)
{
    (void)addr;
    uiox_file_t *f = (uiox_file_t *)0;

    if (!((uint32_t)flags & UIOX_MAP_ANONYMOUS)) {
        long rc = scfs_fd_file(fd, &f);
        if (rc < 0) return rc;
    }

    void *out = (void *)0;
    long rc = vfs_mmap_file(f, (uint64_t)offset, (uint64_t)length,
                            (uint32_t)prot, &out);
    if (rc < 0) return rc;
    return (long)(uiox_reg_t)out;      /* mapped base for the caller */
}

/* munmap() — tear down a mapping and release its pages. */
long uiox_kix_scfs_munmap(uiox_reg_t addr, uiox_reg_t length,
                          uiox_reg_t a2, uiox_reg_t a3, uiox_reg_t a4, uiox_reg_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    if (addr == 0u || length == 0u) return -SCFS_EINVAL;
    return vfs_munmap((void *)addr, (uint64_t)length);
}
