/**
 * @file  uiox_kix_jrnl.h
 * @brief UIOX Filesystem Journaling — master header and lifecycle API.
 *
 * ── NOTE ON THE INCLUDE BELOW ─────────────────────────────────────────
 * This file previously included "uiox_jrnl_recovery.h" — a name that
 * never existed on disk.  The header is uiox_kix_jrnl_recover.h.  Two of
 * the module's own files carried the wrong name and could not compile.
 *
 * @version 1.1.0
 * @date    2026-10-03
 */
#ifndef UIOX_KIX_JRNL_H
#define UIOX_KIX_JRNL_H

#include "uiox_kix_jrnl_types.h"
#include "uiox_kix_jrnl_tx.h"
#include "uiox_kix_jrnl_recover.h"
#include "uiox_kix_jrnl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Syscall numbers
 * ====================================================================== */
#define SYS_SYNC          162u
#define SYS_FSYNC         74u
#define SYS_FDATASYNC     75u
#define SYS_SYNCFS        306u
/* ── how many devices may carry a journal ────────────────────────────
 * s_jr[] in the implementation is one slot per device, and both
 * uiox_jr_register() and uiox_jr_ctx_for() range-check against this.
 * ROOT_DEV is 0, so the device index must stay below it.
 *
 * jrnl.md suggested fs_types.h as this macro's home; it is not defined
 * there, so it lives here until superblock.c's s_sb[] and vfs.c's
 * s_fsops[] are reconciled onto the same constant. */
#ifndef MAX_DEVICES
#define MAX_DEVICES   4
#endif

/* =========================================================================
 * Journal lifecycle API
 * ====================================================================== */

uiox_jr_err_t uiox_jr_init(uiox_jr_ctx_t *ctx,
                             uint64_t       log_dev_base,
                             uint32_t       log_blocks,
                             uiox_jr_mode_t mode,
                             uint64_t     (*get_time_ms)(void));

uiox_jr_err_t uiox_jr_mount(uiox_jr_ctx_t            *ctx,
                              uiox_jr_recovery_stats_t *stats);

uiox_jr_err_t uiox_jr_unmount(uiox_jr_ctx_t *ctx);

void          uiox_jr_abort(uiox_jr_ctx_t *ctx, uiox_jr_err_t reason);

uiox_jr_err_t uiox_jr_tick(uiox_jr_ctx_t *ctx);

uiox_jr_err_t uiox_jr_checkpoint(uiox_jr_ctx_t *ctx);

/* ═════════════════════════════════════════════════════════════════════
 * ADDED: registration, so 01_fsa and 10_unfs can reach the context
 *
 * Parallel to superblock.c's s_sb[MAX_DEVICES] and vfs.c's s_fsops —
 * one array per device, same index space.  A device with no journal
 * returns NULL and every hook tests for it, because a filesystem mounted
 * without journaling is legal.
 * ═════════════════════════════════════════════════════════════════════ */
int             uiox_jr_register(uiox_jr_ctx_t *ctx, uint8_t dev);
uiox_jr_ctx_t  *uiox_jr_ctx_for(uint8_t dev);
void            uiox_jr_unregister(uint8_t dev);

/* =========================================================================
 * VFS integration hooks — called from the 32_FS write paths
 * ====================================================================== */

uiox_jr_err_t uiox_jr_vfs_get_write_access(uiox_jr_ctx_t *ctx,
                                             uint64_t       fs_blocknr,
                                             const void    *buf);

uiox_jr_err_t uiox_jr_vfs_dirty_metadata(uiox_jr_ctx_t *ctx,
                                           uint64_t       fs_blocknr,
                                           const void    *buf);

uiox_jr_err_t uiox_jr_vfs_revoke(uiox_jr_ctx_t *ctx, uint64_t fs_blocknr);

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */
long sys_sync      (long a0, long a1, long a2, long a3);
long sys_fsync     (long fd, long a1, long a2, long a3);
long sys_fdatasync (long fd, long a1, long a2, long a3);
long sys_syncfs    (long fd, long a1, long a2, long a3);

/* =========================================================================
 * Diagnostic helpers
 * ====================================================================== */
void          uiox_jr_print_super (const uiox_jr_ctx_t *ctx);
void          uiox_jr_print_tx    (const uiox_jr_ctx_t *ctx);
void          uiox_jr_print_stats (const uiox_jr_ctx_t *ctx);
const char   *uiox_jr_err_str     (uiox_jr_err_t e);
const char   *uiox_jr_mode_str    (uiox_jr_mode_t m);
const char   *uiox_jr_tx_state_str(uiox_jr_tx_state_t s);

uiox_jr_ctx_t *uiox_jr_ctx_alloc(uint8_t dev);


#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_JRNL_H */
