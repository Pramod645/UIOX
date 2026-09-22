/**
 * @file  uiox_jrnl.h
 * @brief UIOX Filesystem Journaling — master header and top-level API.
 *
 * Single include for all consumers (filesystem layer, VFS hooks, syscalls).
 *
 * Typical filesystem caller sequence:
 *
 *   // Mount
 *   uiox_jnl_init(&jnl, &sb, mode);
 *   uiox_jnl_mount(&jnl);              // recovers if needed
 *
 *   // Atomic metadata update (e.g. create file)
 *   uiox_jnl_handle_t h;
 *   uiox_jnl_handle_start(&jnl, &h, 4);   // 4 block credits
 *   uiox_jnl_buf_get  (&jnl, &h, inode_block, old_data);
 *   // ... modify inode_block in memory ...
 *   uiox_jnl_buf_dirty(&jnl, &h, inode_block, new_data);
 *   uiox_jnl_handle_stop(&jnl, &h);
 *
 *   // Commit (typically done by a background commit thread every 5 s)
 *   uiox_jnl_commit(&jnl);
 *
 *   // Unmount
 *   uiox_jnl_unmount(&jnl);
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
/**
 * @file  uiox_jrnl.h
 * @brief UIOX Filesystem Journaling — master header and lifecycle API.
 *
 * Single include for all consumers. Covers:
 *   - Journal init / mount / unmount
 *   - Transaction handle API  (uiox_jrnl_tx.h)
 *   - Crash recovery          (uiox_jrnl_recovery.h)
 *   - VFS integration hooks   (called from 32_FileSystem write paths)
 *   - Syscall numbers for sys_sync / sys_fsync
 *
 * Journaling modes:
 *   METADATA  — only inode/directory metadata is journaled (fastest, default)
 *   ORDERED   — data blocks flushed before metadata committed (safe default)
 *   DATA      — data blocks also journaled (slowest, highest integrity)
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_JRNL_H
#define UIOX_JRNL_H

#include "uiox_jrnl_types.h"
#include "uiox_jrnl_tx.h"
#include "uiox_jrnl_recovery.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Syscall numbers
 * ====================================================================== */
#define SYS_SYNC          162u  /**< Flush all dirty buffers to disk        */
#define SYS_FSYNC         74u   /**< Flush one fd's dirty buffers           */
#define SYS_FDATASYNC     75u   /**< Flush data but not metadata            */
#define SYS_SYNCFS        306u  /**< Flush all dirty buffers on one fs      */

/* =========================================================================
 * Journal lifecycle API
 * ====================================================================== */

/**
 * @brief Initialise a journal context from an on-disk journal device/area.
 * @param ctx           Caller-allocated context (zeroed on success).
 * @param log_dev_base  Physical base address of journal log area.
 * @param log_blocks    Size of journal log in blocks.
 * @param mode          Journaling mode (METADATA / ORDERED / DATA).
 * @param get_time_ms   Platform monotonic time hook.
 */
uiox_jr_err_t uiox_jr_init(uiox_jr_ctx_t *ctx,
                             uint64_t       log_dev_base,
                             uint32_t       log_blocks,
                             uiox_jr_mode_t mode,
                             uint64_t     (*get_time_ms)(void));

/**
 * @brief Mount — run recovery, then mark journal active.
 *        Must be called before any transaction is opened.
 */
uiox_jr_err_t uiox_jr_mount(uiox_jr_ctx_t            *ctx,
                              uiox_jr_recovery_stats_t *stats);

/**
 * @brief Unmount — flush and commit all pending transactions,
 *        then write a clean superblock.
 */
uiox_jr_err_t uiox_jr_unmount(uiox_jr_ctx_t *ctx);

/**
 * @brief Abort the journal — called on unrecoverable I/O error.
 *        All future handle operations return UIOX_JR_ERR_ABORT.
 */
void uiox_jr_abort(uiox_jr_ctx_t *ctx, uiox_jr_err_t reason);

/**
 * @brief Periodic tick — commits the current transaction if the
 *        commit interval has elapsed. Call from scheduler or timer ISR.
 */
uiox_jr_err_t uiox_jr_tick(uiox_jr_ctx_t *ctx);

/**
 * @brief Checkpoint — write all committed but not yet checkpointed blocks
 *        back to their home locations on the filesystem.
 *        This frees journal log space.
 */
uiox_jr_err_t uiox_jr_checkpoint(uiox_jr_ctx_t *ctx);

/* =========================================================================
 * VFS integration hooks
 *
 * Called from the VFS write paths in 32_FileSystem. These wrap the
 * handle API with buffer_head semantics to match the existing VFS layer.
 * ====================================================================== */

/**
 * @brief Called by VFS before writing a metadata block (inode, dir, bitmap).
 *        Equivalent to jbd2_journal_get_write_access().
 */
uiox_jr_err_t uiox_jr_vfs_get_write_access(uiox_jr_ctx_t *ctx,
                                             uint64_t       fs_blocknr,
                                             void          *buf);

/**
 * @brief Called by VFS after modifying a metadata block.
 *        Equivalent to jbd2_journal_dirty_metadata().
 */
uiox_jr_err_t uiox_jr_vfs_dirty_metadata(uiox_jr_ctx_t *ctx,
                                           uint64_t       fs_blocknr,
                                           const void    *buf);

/**
 * @brief Called by VFS on truncate / unlink to prevent stale replay.
 *        Equivalent to jbd2_journal_revoke().
 */
uiox_jr_err_t uiox_jr_vfs_revoke(uiox_jr_ctx_t *ctx, uint64_t fs_blocknr);

/* =========================================================================
 * Syscall handlers (registered in 40_SystemCallInterface)
 * ====================================================================== */
long sys_sync    (long a0, long a1, long a2, long a3);
long sys_fsync   (long fd, long a1, long a2, long a3);
long sys_fdatasync(long fd, long a1, long a2, long a3);
long sys_syncfs  (long fd, long a1, long a2, long a3);

/* =========================================================================
 * Diagnostic helpers
 * ====================================================================== */
void          uiox_jr_print_super (const uiox_jr_ctx_t *ctx);
void          uiox_jr_print_tx    (const uiox_jr_ctx_t *ctx);
void          uiox_jr_print_stats (const uiox_jr_ctx_t *ctx);
const char   *uiox_jr_err_str     (uiox_jr_err_t e);
const char   *uiox_jr_mode_str    (uiox_jr_mode_t m);
const char   *uiox_jr_tx_state_str(uiox_jr_tx_state_t s);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_JRNL_H */
