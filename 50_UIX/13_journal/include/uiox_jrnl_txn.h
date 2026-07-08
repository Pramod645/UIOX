/**
 * @file  uiox_jrnl_txn.h
 * @brief UIOX Filesystem Journaling — transaction and handle lifecycle.
 *
 * Transaction lifecycle:
 *   uiox_jnl_txn_begin()
 *       │
 *       ▼
 *   uiox_jnl_handle_start()    ← filesystem caller gets a handle
 *   uiox_jnl_buf_get()         ← marks a buffer as part of this txn
 *   uiox_jnl_buf_dirty()       ← signals write intent
 *   uiox_jnl_handle_stop()     ← releases credits back to txn
 *       │
 *       ▼
 *   uiox_jnl_txn_commit()      ← writes descriptor + data + commit blocks
 *       │
 *       ▼
 *   uiox_jnl_txn_checkpoint()  ← writes dirty buffers back to main fs
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
/**
 * @file  uiox_jrnl_tx.h
 * @brief UIOX Filesystem Journaling — transaction and handle API.
 *
 * A transaction groups a set of block writes that must appear atomically
 * on disk. Callers (VFS write paths) open a handle, log their dirty
 * blocks, then stop the handle. The journal commits on a timer or when
 * the log fills.
 *
 * Usage pattern:
 *   uiox_jr_handle_t *h = uiox_jr_start(&jctx, 8);   // reserve 8 blocks
 *   uiox_jr_get_write_access(h, blocknr, buf);        // log before modify
 *   ... modify buf in place ...
 *   uiox_jr_dirty_metadata(h, blocknr, buf);          // mark dirty
 *   uiox_jr_stop(h);                                  // release handle
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_JRNL_TX_H
#define UIOX_JRNL_TX_H

#include "uiox_jrnl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Handle — one atomic unit of work within a transaction
 * ====================================================================== */
typedef struct uiox_jr_handle {
    uint32_t               tx_id;         /**< Owning transaction ID        */
    uint32_t               reserved;      /**< Blocks reserved for this hdl */
    uint32_t               used;          /**< Blocks actually logged       */
    bool                   aborted;
    bool                   sync;          /**< Force sync commit on stop    */
} uiox_jr_handle_t;

/* =========================================================================
 * Transaction — in-memory state for one atomic commit unit
 * ====================================================================== */
#define UIOX_JR_TX_ID_NONE   0u

typedef struct uiox_jr_transaction {
    uint32_t                tid;          /**< Sequence number              */
    uiox_jr_tx_state_t      state;
    uiox_jr_mode_t          mode;

    /* Logged blocks (metadata + optionally data) */
    uiox_jr_logged_block_t  blocks[UIOX_JR_MAX_BLOCKS_PER_TX];
    uint32_t                block_count;

    /* Revoke list — blocks that must NOT be replayed on recovery */
    uint64_t                revoked[UIOX_JR_MAX_REVOKE];
    uint32_t                revoke_count;

    /* Open handle count — commit waits for this to reach 0 */
    uint32_t                handle_count;

    /* Running checksum over all block data */
    uint32_t                data_checksum;

    /* Timestamps */
    uint64_t                start_time;
    uint64_t                commit_time;
} uiox_jr_transaction_t;

/* =========================================================================
 * Journal context — one per mounted filesystem
 * ====================================================================== */
typedef struct {
    /* On-disk superblock (mirrored in RAM) */
    uiox_jr_super_t         jsb;

    /* Log area geometry */
    uint64_t                log_dev_base;  /**< Physical base addr of log   */
    uint32_t                log_blocks;    /**< Total log blocks available  */
    uint32_t                log_head;      /**< Next free log block (wrap)  */
    uint32_t                log_tail;      /**< Oldest uncommitted log blk  */
    uint32_t                free_blocks;   /**< Blocks available to write   */

    /* Current running transaction */
    uiox_jr_transaction_t   current_tx;
    bool                    tx_open;

    /* Journal mode */
    uiox_jr_mode_t          mode;

    /* State flags */
    bool                    aborted;       /**< Fatal I/O error seen        */
    bool                    initialized;
    bool                    mounted;

    /* Commit interval in ms (default 5000) */
    uint32_t                commit_interval_ms;
    uint64_t                last_commit_ms;

    /* Platform time hook */
    uint64_t              (*get_time_ms)(void);
} uiox_jr_ctx_t;

/* =========================================================================
 * Transaction API
 * ====================================================================== */

/**
 * @brief Open a new handle, reserving @nblocks credits in current tx.
 *        If no transaction is running, one is started automatically.
 * @return Pointer to handle, or NULL on error (journal full / aborted).
 */
uiox_jr_handle_t *uiox_jr_start(uiox_jr_ctx_t *ctx, uint32_t nblocks);

/**
 * @brief Log a block before the caller modifies it (copy-on-write into log).
 *        Must be called before any modification to @buf.
 */
uiox_jr_err_t uiox_jr_get_write_access(uiox_jr_handle_t *h,
                                         uiox_jr_ctx_t    *ctx,
                                         uint64_t          fs_blocknr,
                                         const void       *buf);

/**
 * @brief Mark a previously logged block as dirty (metadata path).
 *        Updates the in-memory log entry with the current contents of @buf.
 */
uiox_jr_err_t uiox_jr_dirty_metadata(uiox_jr_handle_t *h,
                                       uiox_jr_ctx_t    *ctx,
                                       uint64_t          fs_blocknr,
                                       const void       *buf);

/**
 * @brief Add @fs_blocknr to the revoke list for this transaction.
 *        Prevents replay of an old version of this block on recovery.
 */
uiox_jr_err_t uiox_jr_revoke(uiox_jr_handle_t *h,
                               uiox_jr_ctx_t    *ctx,
                               uint64_t          fs_blocknr);

/**
 * @brief Release a handle. When the last handle in a transaction is
 *        stopped, the transaction is eligible for commit.
 */
uiox_jr_err_t uiox_jr_stop(uiox_jr_handle_t *h, uiox_jr_ctx_t *ctx);

/**
 * @brief Force an immediate synchronous commit of the current transaction.
 *        Blocks until the commit block is written to disk.
 */
uiox_jr_err_t uiox_jr_force_commit(uiox_jr_ctx_t *ctx);

/** Query whether the journal has been aborted. */
bool          uiox_jr_is_aborted(const uiox_jr_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_JRNL_TX_H */
