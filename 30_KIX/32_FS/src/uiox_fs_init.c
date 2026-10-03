/*
 *  30_KIX/32_FS/src/uiox_fs_init.c
 *
 *  File-system subsystem entry point — freestanding, no system headers.
 *
 *  Called by uiox_kernel_main() during bring-up, after uiox_mm_init()
 *  and before the first process opens a file.
 *
 *  Bring-up sequence:
 *    0. uiox_pc_init()          — page cache (VFS and SCFS use it)
 *    1. vfs_init()              — inode / dentry / file caches
 *    2. uiox_kix_scfs_register()— register SCFS on ROOT_DEV
 *    3. uiox_jr_ctx_alloc()     — obtain a journal context (opaque type,
 *                                 so the module owns the storage)
 *    4. uiox_jr_init()          — geometry + time hook, METADATA mode
 *    5. uiox_jr_mount()         — runs recovery on the log tail
 *    6. uiox_jr_register()      — binds the context to ROOT_DEV, and this
 *                                 is what makes uiox_jr_ctx_for() resolve
 *    7. vfs_mount_root()        — NOT attempted; see the note below
 *
 *  @version 1.4.0  @date 2026-10-03
 */
#include "vfs.h"                /* vfs_init, vfs_mount_root, VFS_OK       */
#include "uiox_kix_scfs.h"      /* uiox_kix_scfs_register                 */
#include "uiox_kix_jrnl.h"      /* uiox_jr_ctx_alloc / init / mount /
                                 * register / uiox_jr_ctx_t              */
#include "inode.h"              /* ROOT_DEV                               */

extern void uiox_pc_init(void);

/* ── the one journal context, RECOVERED and REGISTERED on ROOT_DEV ────
 * uiox_jr_ctx_t is OPAQUE here: uiox_kix_jrnl.h names it only as a
 * pointer, so this file cannot declare or allocate the storage.  The
 * journal module owns a per-device pool and hands one out; this is the
 * handle, and uiox_jr_register() stores it in s_jr[ROOT_DEV], which is
 * what every hook reaches through uiox_jr_ctx_for(dev). */
static uiox_jr_ctx_t *s_root_journal;

/* ── weak stubs — overridden at link time ─────────────────────────── */
__attribute__((weak)) void vfs_init(void)
{
    /* silent: early_puts is a BSP symbol and is not visible here */
}

__attribute__((weak)) int vfs_mount_root(uint8_t dev)   /* vfs.h:134 */
{
    (void)dev;
    return VFS_OK;
}

/* ── uiox_fs_init ─────────────────────────────────────────────────── */
int uiox_fs_init(void)
{
    uiox_jr_err_t jrc;
    int           rc;

    /* 0 — Page cache.  Must be first: VFS and SCFS allocate through it. */
    uiox_pc_init();

    /* 1 — VFS layer: inode / dentry / file caches */
    vfs_init();

    /* 2 — Register the filesystems.  ROOT_DEV is 0 (inode.h), so the
     *     root filesystem registers on device 0 and that is the index
     *     every dispatch uses. */
    rc = uiox_kix_scfs_register(ROOT_DEV);
    if (rc != VFS_OK)
        return rc;

    /* 3 — unfs_register(1) goes here when a second volume exists. */

    /* 4 — Obtain the journal context.  The module owns the storage, so
     *     a NULL here means the device index is out of range — not that
     *     the log is broken. */
    s_root_journal = uiox_jr_ctx_alloc((uint8_t)ROOT_DEV);

    if (s_root_journal) {
        /* 5 — Journal init: sets geometry, binds the time hook.  MODE_
         *     METADATA, not ORDERED: ORDERED is refused by uiox_jr_init
         *     until uiox_jr_plat_barrier() exists. */
        jrc = uiox_jr_init(s_root_journal,
                           0u,                      /* log_dev_base       */
                           UIOX_JR_MIN_LOG_BLOCKS,
                           UIOX_JR_MODE_METADATA,
                           (uint64_t (*)(void))0);

        if (jrc == UIOX_JR_OK) {
            /* 6 — Journal MOUNT, which runs RECOVERY.  Must precede the
             *     registration below: a hook firing against a journal
             *     whose log tail is still dirty from the last crash
             *     would log into an unrecovered log. */
            jrc = uiox_jr_mount(s_root_journal,
                                (uiox_jr_recovery_stats_t *)0);

            if (jrc == UIOX_JR_OK) {
                /* 7 — REGISTER.  This is the call that makes every hook
                 *     in 01_fsa and 10_unfs reach a live context through
                 *     uiox_jr_ctx_for(dev).  Without it s_jr[dev] stays
                 *     NULL and all 31 hook sites silently no-op. */
                (void)uiox_jr_register(s_root_journal, (uint8_t)ROOT_DEV);
            }
            /* mount failed → the FS runs unjournalled; non-fatal,
             * because every hook's `if (jr)` guard handles a NULL
             * context. */
        }
        /* init failed → same: unjournalled. */
    }
    /* no context → same: unjournalled.  All three paths fall through to
     * the return below, because a filesystem without a journal is still
     * a usable filesystem. */

    /* 8 — Mount the root device.  NOT ATTEMPTED, and reported rather
     *     than faked: vfs_mount_root() dispatches to the backend's mount
     *     hook, and both tables carry NULL because a block special file
     *     cannot be created — mknod refuses SCFS_S_IFBLK since
     *     InCoreInode has no i_major/i_minor. */
    return 0;
}
