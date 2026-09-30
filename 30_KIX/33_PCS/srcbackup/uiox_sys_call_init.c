/*
 * 30_KIX/33_PCS/src/uiox_sys_call_init.c(New)
 *
 * uiox_sys_call_init() — called by uiox_proc_init() step 5.
 *
 * Initialises:
 *   1. File descriptor table
 *   2. Filesystem subsystem (uiox_fs_init)
 *   3. Confirms syscall dispatch table is live
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_syscall.h"
 #include "uiox_sys_fd.h"
 #include "uiox_soc_stdio.h"
 
 /* Forward from 32_FS */
 extern int uiox_fs_init(void);
 
 void uiox_sys_call_init(void)
 {
     early_puts("[pcs] uiox_sys_call_init\n");
 
     /* 1 — file descriptor table */
     uiox_fd_init();
 
     /* 2 — filesystem: VFS + SCFS + journal + mount root */
     int rc = uiox_fs_init();
     if (rc != 0)
         early_puts("[pcs] WARNING: fs init failed\n");
 
     early_puts("[pcs] syscall table live\n");
 }
 