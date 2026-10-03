/**
 * @file  uiox_kix_jrnl_recover.h
 * @brief UIOX Filesystem Journaling — crash recovery (replay) engine.
 *
 * On mount after an unclean shutdown, the recovery engine:
 *   1. Scans the circular log for valid descriptor → data → commit sequences.
 *   2. Builds a replay map (latest version of each block wins).
 *   3. Applies (replays) each block to the filesystem.
 *   4. Processes revoke records to suppress stale replays.
 *   5. Updates the journal superblock to mark the log clean.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_KIX_JRNL_RECOVER_H
#define UIOX_KIX_JRNL_RECOVER_H

#include "uiox_kix_jrnl_types.h"



#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Replay map entry — tracks the latest log position for each fs block
 * ====================================================================== */
#define UIOX_JR_REPLAY_MAP_SIZE   512u

typedef struct {
    uint64_t  fs_blocknr;
    uint32_t  log_blocknr;
    uint32_t  tx_sequence;
    bool      revoked;
} uiox_jr_replay_entry_t;

/* =========================================================================
 * Recovery statistics
 * ====================================================================== */
/*
struct uiox_jr_recovery_stats {
    uint32_t  txns_found;
    uint32_t  txns_partial;
    uint32_t  blocks_replayed;
    uint32_t  blocks_revoked;
    uint32_t  corrupt_blocks;
    bool      clean;
};
*/
/* =========================================================================
 * Recovery API
 * ====================================================================== */

uiox_jr_err_t uiox_jr_recover(uiox_jr_ctx_t            *ctx,
                                uiox_jr_recovery_stats_t *stats);

uiox_jr_err_t uiox_jr_scan(uiox_jr_ctx_t          *ctx,
                             uiox_jr_replay_entry_t *map,
                             uint32_t               *map_count,
                             uint32_t               *last_committed_seq);

uiox_jr_err_t uiox_jr_replay(uiox_jr_ctx_t             *ctx,
                               uiox_jr_replay_entry_t    *map,
                               uint32_t                   map_count,
                               uiox_jr_recovery_stats_t  *stats);

void uiox_jr_recovery_print(const uiox_jr_recovery_stats_t *s);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KIX_JRNL_RECOVER_H */
