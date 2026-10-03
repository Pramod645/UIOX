/**
 * @file  uiox_kix_jrnl_tx.h
 * @brief UIOX Filesystem Journaling — transaction and handle API.
 *
 * Usage pattern:
 *   uiox_jr_handle_t *h = uiox_jr_start(&jctx, 8);
 *   uiox_jr_get_write_access(h, &jctx, blocknr, buf);
 *   ... modify buf in place ...
 *   uiox_jr_dirty_metadata(h, &jctx, blocknr, buf);
 *   uiox_jr_stop(h, &jctx);
 *
 * ── the log pool ──────────────────────────────────────────────────────
 * The context owns a pool of UIOX_JR_LOG_POOL_BLOCKS × UIOX_JR_BLOCK_SIZE
 * bytes.  A logged entry names a slot in it rather than holding a pointer
 * to the caller's buffer: bwrite() releases that buffer, so a pointer
 * would dangle before the commit ever read it.
 *
 * 256 × 4096 = 1 MiB inside uiox_jr_ctx_t.  A real cost, stated.
 *
 * @version 1.1.0
 * @date    2026-10-03
 */
#ifndef UIOX_KIX_JRNL_TX_H
#define UIOX_KIX_JRNL_TX_H

#include "uiox_kix_jrnl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Handle — one atomic unit of work within a transaction
 * ====================================================================== */
typedef struct uiox_jr_handle {
    uint32_t               tx_id;
    uint32_t               reserved;
    uint32_t               used;
    bool                   aborted;
    bool                   sync;
} uiox_jr_handle_t;

/* =========================================================================
 * Transaction — in-memory state for one atomic commit unit
 * ====================================================================== */
#define UIOX_JR_TX_ID_NONE   0u

typedef struct uiox_jr_transaction {
    uint32_t                tid;
    uiox_jr_tx_state_t      state;
    uiox_jr_mode_t          mode;

    uiox_jr_logged_block_t  blocks[UIOX_JR_MAX_BLOCKS_PER_TX];
    uint32_t                block_count;

    uint64_t                revoked[UIOX_JR_MAX_REVOKE];
    uint32_t                revoke_count;

    uint32_t                handle_count;
    uint32_t                data_checksum;

    uint64_t                start_time;
    uint64_t                commit_time;
} uiox_jr_transaction_t;

/* =========================================================================
 * Journal context — one per mounted filesystem
 * ====================================================================== */
struct uiox_jr_ctx {
    uiox_jr_super_t         jsb;

    uint64_t                log_dev_base;
    uint32_t                log_blocks;
    uint32_t                log_head;
    uint32_t                log_tail;
    uint32_t                free_blocks;

    uiox_jr_transaction_t   current_tx;
    bool                    tx_open;

    uiox_jr_mode_t          mode;

    bool                    aborted;
    bool                    initialized;
    bool                    mounted;

    uint32_t                commit_interval_ms;
    uint64_t                last_commit_ms;

    uint64_t              (*get_time_ms)(void);

    /* ── the log data pool ─────────────────────────────────────────────
     * The bytes for every logged block in the current transaction.  The
     * CONTEXT owns them, not the entry, so a slot outlives the struct
     * that names it and an entry outliving its slot is a detectable
     * state rather than a dangling read. */
    uint8_t log_pool[UIOX_JR_LOG_POOL_BLOCKS][UIOX_JR_BLOCK_SIZE];

};

/* =========================================================================
 * Transaction API
 * ====================================================================== */

uiox_jr_handle_t *uiox_jr_start(uiox_jr_ctx_t *ctx, uint32_t nblocks);

uiox_jr_err_t uiox_jr_get_write_access(uiox_jr_handle_t *h,
                                         uiox_jr_ctx_t    *ctx,
                                         uint64_t          fs_blocknr,
                                         const void       *buf);

uiox_jr_err_t uiox_jr_dirty_metadata(uiox_jr_handle_t *h,
                                       uiox_jr_ctx_t    *ctx,
                                       uint64_t          fs_blocknr,
                                       const void       *buf);

uiox_jr_err_t uiox_jr_revoke(uiox_jr_handle_t *h,
                               uiox_jr_ctx_t    *ctx,
                               uint64_t          fs_blocknr);

uiox_jr_err_t uiox_jr_stop(uiox_jr_handle_t *h, uiox_jr_ctx_t *ctx);

uiox_jr_err_t uiox_jr_force_commit(uiox_jr_ctx_t *ctx);

bool          uiox_jr_is_aborted(const uiox_jr_ctx_t *ctx);

/* ── ADDED: the read-only face of the pool ────────────────────────────
 * The commit path and the checkpoint resolve an entry's data_index into
 * the pool's bytes through this, so the index-to-pointer translation
 * lives in ONE function.  Both parameters are const: reading a logged
 * block does not modify it, and keeping this separate from the writers'
 * helper stops a commit path acquiring a write capability it has no
 * business holding.
 */
const uint8_t *uiox_jr_block_data(const uiox_jr_ctx_t *ctx,
                                   const uiox_jr_logged_block_t *lb);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_JRNL_TX_H */
