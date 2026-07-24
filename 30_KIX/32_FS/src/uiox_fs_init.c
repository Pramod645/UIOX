/*
 *  30_KIX/32_FS/src/uiox_fs_init.c
 *
 *  File-System subsystem entry point — freestanding, no system headers.
 *
 *  uiox_fs_init() is called by uiox_kernel_main() during kernel bring-up,
 *  after the memory manager (uiox_mm_init) and before the first process
 *  opens any file.  It registers all compiled-in filesystems with the VFS
 *  and mounts the root device.
 *
 *  Bring-up sequence:
 *    1. vfs_init()       — initialise inode / dentry / file caches
 *    2. scfs_init()      — register the Simple Contiguous FS
 *    3. vfs_mount_root() — mount the root device (from BSP platform stub)
 *
 *  Each of those functions is a weak symbol here; the real implementations
 *  live in 01_fsa/ and 10_scfs/ and override these stubs at link time once
 *  those layers are written.
 *
 *  @version 1.0.0  @date 2026-07-24
 */
#include "../../33_PCS/include/uiox_klibc.h"   /* provided by 32_FS/include/ — same pattern as 33_PCS */

/* ── weak stubs — overridden by 01_fsa/ and 10_scfs/ at link time ─────────── */

__attribute__((weak))
void vfs_init(void)
{
    printf("[fs] vfs_init: stub (01_fsa not yet linked)\n");
}

__attribute__((weak))
void scfs_init(void)
{
    printf("[fs] scfs_init: stub (10_scfs not yet linked)\n");
}

__attribute__((weak))
int vfs_mount_root(void)
{
    printf("[fs] vfs_mount_root: stub — no root device\n");
    return 0;
}

/* ── uiox_fs_init ───────────────────────────────────────────────────────────
 *
 *  Called once by uiox_kernel_main() before the first open() syscall.
 *  Returns 0 on success, negative error code on failure.
 * ───────────────────────────────────────────────────────────────────────── */
int uiox_fs_init(void)
{
    printf("[fs] uiox_fs_init: starting\n");

    vfs_init();        /* inode / dentry / file caches          */
    scfs_init();       /* register Simple Contiguous FS          */

    int ret = vfs_mount_root();
    if (ret < 0) {
        printf("[fs] ERROR: vfs_mount_root failed (%d)\n", ret);
        return ret;
    }

    printf("[fs] uiox_fs_init: done\n");
    return 0;
}
