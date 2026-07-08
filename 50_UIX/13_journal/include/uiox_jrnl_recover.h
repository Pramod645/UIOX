/**
 * @file  uiox_jrnl_recover.h
 * @brief UIOX Filesystem Journaling — crash recovery and log replay.
 *
 * Recovery phases (identical to ext3/ext4 recovery model):
 *
 *   SCAN   — walk journal from head to tail, build transaction map
 *   REVOKE — load all revoke blocks, build revoke table
 *   REPLAY — re-apply committed transactions in sequence order,
 *             skipping revoked blocks
 *
 * Called automatically by uiox_jnl_mount() when the journal superblock
 * indicates an unclean shutdown (head != tail and no CLEAN flag).
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
/**
 * @file  uiox_jrnl_recovery.h
 * @brief UIOX Filesystem Journaling — crash recovery (replay) engine.
 *
 * On mount after an unclean shutdown, the recovery engine:
 *   1. Scans the circular log for valid descriptor → data → commit sequences.
 *   2. Builds a replay map (latest version of each block wins).
 *   3. Applies (replays) each block to the filesystem.
 *   4. Processes revoke records to suppress stale replays.
 *   5. Updates the journal superblock to mark the log clean.
 *
 * Recovery is always run before the filesystem is made writeable.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_JRNL_RECOVERY_H
#define UIOX_JRNL_RECOVERY_H

#include "uiox_jrnl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Replay map entry — tracks the latest log position for each fs block
 * ====================================================================== */
#define UIOX_JR_REPLAY_MAP_SIZE   512u   /**< Max distinct blocks to replay */

typedef struct {
    uint64_t  fs_blocknr;    /**< Filesystem block to be replayed           */
    uint32_t  log_blocknr;   /**< Block in journal log holding latest data  */
    uint32_t  tx_sequence;   /**< Sequence of the committing transaction    */
    bool      revoked;       /**< True → skip replay                        */
} uiox_jr_replay_entry_t;

/* =========================================================================
 * Recovery statistics (returned to caller for logging / fsck)
 * ====================================================================== */
typedef struct {
    uint32_t  txns_found;      /**< Complete committed transactions found   */
    uint32_t  txns_partial;    /**< Partial (uncommitted) transactions skipped */
    uint32_t  blocks_replayed; /**< Blocks written back to filesystem       */
    uint32_t  blocks_revoked;  /**< Blocks suppressed by revoke records     */
    uint32_t  corrupt_blocks;  /**< Blocks with bad CRC (skipped)           */
    bool      clean;           /**< True if journal was already clean       */
} uiox_jr_recovery_stats_t;

/* =========================================================================
 * Recovery API
 * ====================================================================== */

/**
 * @brief Run full journal recovery on @ctx.
 *        Must be called at mount time before any write is allowed.
 * @param stats  Optional — filled with recovery statistics if non-NULL.
 * @return UIOX_JR_OK on success (including already-clean journal).
 */
uiox_jr_err_t uiox_jr_recover(uiox_jr_ctx_t            *ctx,
                                uiox_jr_recovery_stats_t *stats);

/**
 * @brief Scan phase — walk circular log and catalogue all valid transactions.
 *        Populates @map with the latest log position of each fs block.
 */
uiox_jr_err_t uiox_jr_scan(uiox_jr_ctx_t       *ctx,
                             uiox_jr_replay_entry_t *map,
                             uint32_t            *map_count,
                             uint32_t            *last_committed_seq);

/**
 * @brief Replay phase — write each non-revoked map entry back to fs.
 */
uiox_jr_err_t uiox_jr_replay(uiox_jr_ctx_t             *ctx,
                               uiox_jr_replay_entry_t    *map,
                               uint32_t                   map_count,
                               uiox_jr_recovery_stats_t  *stats);

/** Print recovery statistics to console. */
void uiox_jr_recovery_print(const uiox_jr_recovery_stats_t *s);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_JRNL_RECOVERY_H */
