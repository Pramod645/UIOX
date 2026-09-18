/*
 * 30_KIX/32_FS/src/uiox_fs_init.c  — v1.1.0
 *
 * Fixed: replaced printf() (libc — banned under -ffreestanding)
 * with early_puts() from uiox_soc_stdio.h.
 * Added: uiox_fs_init() now also calls uiox_jr_init() for journal.
 */

 #include "uiox_vfs.h"
 #include "uiox_soc_stdio.h"    /* early_puts — freestanding          */
 
 /* ── weak stubs — overridden at link time ──────────────────────────── */
 __attribute__((weak)) void vfs_init(void)
 {
     early_puts("[fs] vfs_init: stub\n");
 }
 
 __attribute__((weak)) void scfs_init(void)
 {
     early_puts("[fs] scfs_init: stub\n");
 }
 
 __attribute__((weak)) int vfs_mount_root(void)
 {
     early_puts("[fs] vfs_mount_root: stub\n");
     return 0;
 }
 
 __attribute__((weak)) void uiox_jr_init(void)
 {
     early_puts("[fs] uiox_jr_init: stub (02_journal not linked)\n");
 }
 
 /* ── uiox_fs_init ──────────────────────────────────────────────────── */
 int uiox_fs_init(void)
 {
     early_puts("[fs] uiox_fs_init starting...\n");
 
     /* 1 — VFS layer: inode/dentry/file caches */
     vfs_init();
 
     /* 2 — Register SCFS (Simple Contiguous FS) */
     scfs_init();
 
     /* 3 — Journal (02_journal) */
     uiox_jr_init();
 
     /* 4 — Mount root device */
     int rc = vfs_mount_root();
     if (rc != 0) {
         early_puts("[fs] WARNING: root mount failed\n");
         /* non-fatal — system can run without persistent FS */
         return rc;
     }
 
     early_puts("[fs] uiox_fs_init complete\n");
     return 0;
 }
 