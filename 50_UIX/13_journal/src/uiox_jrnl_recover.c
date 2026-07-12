/**
 * @file  uiox_jrnl_recovery.c
 * @brief UIOX Filesystem Journaling — crash recovery (scan + replay).
 * @date  2026-07-08
 */
#include "../include/uiox_jrnl_recovery.h"
#include "../include/uiox_jrnl.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void rc_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void rc_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

/* CRC-32 */
static uint32_t rc_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ ((crc & 1u) ? UIOX_JR_CRC32_POLY : 0u);
    }
    return crc ^ 0xFFFFFFFFu;
}

/* ── Platform block I/O hooks — override in BSP ─────────────────────── */

/**
 * @brief Read one journal log block at @log_blocknr into @buf.
 *        @log_dev_base is the physical base of the journal area.
 */
__attribute__((weak))
uiox_jr_err_t uiox_jr_plat_log_read(uint64_t log_dev_base,
                                      uint32_t log_blocknr,
                                      void    *buf)
{
    /* Stub: memory-mapped (XIP) journal */
    uint8_t *src = (uint8_t *)(uintptr_t)
                   (log_dev_base + (uint64_t)log_blocknr * UIOX_JR_BLOCK_SIZE);
    rc_memcpy(buf, src, UIOX_JR_BLOCK_SIZE);
    return UIOX_JR_OK;
}

/**
 * @brief Write one block @buf to filesystem block @fs_blocknr.
 *        Called during replay to restore committed metadata.
 */
__attribute__((weak))
uiox_jr_err_t uiox_jr_plat_fs_write(uint64_t    fs_blocknr,
                                      const void *buf)
{
    /* Stub: in a real kernel this calls the block layer */
    (void)fs_blocknr; (void)buf;
    return UIOX_JR_OK;
}

/* =========================================================================
 * Helpers
 * ====================================================================== */

/** Wrap a log block index within the circular log. */
static uint32_t log_wrap(const uiox_jr_ctx_t *ctx, uint32_t blk)
{
    return blk % ctx->log_blocks;
}

/** Look up or insert an entry in the replay map. */
static uiox_jr_replay_entry_t *map_find_or_insert(
        uiox_jr_replay_entry_t *map,
        uint32_t               *count,
        uint64_t                fs_blocknr)
{
    for (uint32_t i = 0; i < *count; i++)
        if (map[i].fs_blocknr == fs_blocknr) return &map[i];

    if (*count >= UIOX_JR_REPLAY_MAP_SIZE) return NULL;

    uiox_jr_replay_entry_t *e = &map[*count];
    rc_memset(e, 0, sizeof(*e));
    e->fs_blocknr = fs_blocknr;
    (*count)++;
    return e;
}

/* =========================================================================
 * Scan phase — catalogue all committed transactions in the log
 * ====================================================================== */
uiox_jr_err_t uiox_jr_scan(uiox_jr_ctx_t          *ctx,
                             uiox_jr_replay_entry_t *map,
                             uint32_t               *map_count,
                             uint32_t               *last_seq)
{
    if (!ctx || !map || !map_count || !last_seq) return UIOX_JR_ERR_INVAL;

    *map_count = 0u;
    *last_seq  = 0u;

    static uint8_t blk_buf[UIOX_JR_BLOCK_SIZE];
    uint32_t pos = ctx->jsb.log_start;

    uiox_fw_printf("[jrnl-recovery] Scan: log_start=%u  log_blocks=%u\n",
                   pos, ctx->log_blocks);

    uint32_t scanned = 0u;

    while (scanned < ctx->log_blocks) {
        uint32_t cur = log_wrap(ctx, pos);

        if (uiox_jr_plat_log_read(ctx->log_dev_base, cur, blk_buf)
                != UIOX_JR_OK) {
            uiox_fw_printf("[jrnl-recovery] I/O error at log block %u\n",
                           cur);
            break;
        }

        uiox_jr_desc_hdr_t hdr;
        rc_memcpy(&hdr, blk_buf, sizeof(hdr));

        /* Not a journal descriptor block — end of valid log */
        if (hdr.magic != UIOX_JR_DESC_MAGIC &&
            hdr.magic != UIOX_JR_COMMIT_MAGIC &&
            hdr.magic != UIOX_JR_REVOKE_MAGIC) {
            break;
        }

        if (hdr.blocktype == 1u) {
            /* Descriptor block — parse block tags */
            uint32_t tag_off = (uint32_t)sizeof(uiox_jr_desc_hdr_t);
            uint32_t data_pos = cur + 1u;  /* data blocks follow descriptor */

            while (tag_off + (uint32_t)sizeof(uiox_jr_block_tag_t)
                   <= UIOX_JR_BLOCK_SIZE) {

                uiox_jr_block_tag_t tag;
                rc_memcpy(&tag, blk_buf + tag_off, sizeof(tag));

                if (tag.blocknr == 0u) break;

                /* Record latest log position for this fs block */
                uiox_jr_replay_entry_t *e =
                    map_find_or_insert(map, map_count, tag.blocknr);
                if (e) {
                    e->log_blocknr  = log_wrap(ctx, data_pos);
                    e->tx_sequence  = hdr.sequence;
                    e->revoked      = false;
                }

                data_pos++;
                tag_off += (uint32_t)sizeof(uiox_jr_block_tag_t);

                if (tag.flags & UIOX_JR_FLAG_LAST_TAG) break;
            }

        } else if (hdr.blocktype == 2u) {
            /* Commit block — this sequence is fully committed */
            if (hdr.sequence > *last_seq)
                *last_seq = hdr.sequence;
            uiox_fw_printf("[jrnl-recovery] Committed tx seq=%u\n",
                           hdr.sequence);

        } else if (hdr.blocktype == 3u) {
            /* Revoke block */
            uiox_jr_revoke_hdr_t rev;
            rc_memcpy(&rev, blk_buf, sizeof(rev));
            uint32_t roff = (uint32_t)sizeof(uiox_jr_revoke_hdr_t);

            for (uint32_t r = 0; r < rev.count &&
                                  roff + 8u <= UIOX_JR_BLOCK_SIZE;
                 r++, roff += 8u) {
                uint64_t blknr = 0u;
                rc_memcpy(&blknr, blk_buf + roff, 8u);

                for (uint32_t i = 0; i < *map_count; i++) {
                    if (map[i].fs_blocknr == blknr &&
                        map[i].tx_sequence <= hdr.sequence)
                        map[i].revoked = true;
                }
            }
        }

        pos    = log_wrap(ctx, pos + 1u);
        scanned++;
    }

    /* Remove map entries for sequences beyond last committed */
    uint32_t kept = 0u;
    for (uint32_t i = 0; i < *map_count; i++) {
        if (map[i].tx_sequence <= *last_seq && !map[i].revoked) {
            if (kept != i) map[kept] = map[i];
            kept++;
        }
    }
    *map_count = kept;

    uiox_fw_printf("[jrnl-recovery] Scan complete: "
                   "%u blocks to replay, last_seq=%u\n",
                   *map_count, *last_seq);
    return UIOX_JR_OK;
}

/* =========================================================================
 * Replay phase — write each non-revoked block back to filesystem
 * ====================================================================== */
uiox_jr_err_t uiox_jr_replay(uiox_jr_ctx_t            *ctx,
                               uiox_jr_replay_entry_t   *map,
                               uint32_t                  map_count,
                               uiox_jr_recovery_stats_t *stats)
{
    if (!ctx || !map) return UIOX_JR_ERR_INVAL;

    static uint8_t data_buf[UIOX_JR_BLOCK_SIZE];

    for (uint32_t i = 0; i < map_count; i++) {
        uiox_jr_replay_entry_t *e = &map[i];

        if (e->revoked) {
            if (stats) stats->blocks_revoked++;
            continue;
        }

        /* Read data block from journal log */
        if (uiox_jr_plat_log_read(ctx->log_dev_base,
                                   e->log_blocknr, data_buf)
                != UIOX_JR_OK) {
            uiox_fw_printf("[jrnl-recovery] I/O error reading log block %u\n",
                           e->log_blocknr);
            if (stats) stats->corrupt_blocks++;
            continue;
        }

        /* Write to filesystem */
        uiox_jr_err_t rc = uiox_jr_plat_fs_write(e->fs_blocknr, data_buf);
        if (rc != UIOX_JR_OK) {
            uiox_fw_printf("[jrnl-recovery] I/O error writing fs block %llu\n",
                           (unsigned long long)e->fs_blocknr);
            if (stats) stats->corrupt_blocks++;
        } else {
            if (stats) stats->blocks_replayed++;
            uiox_fw_printf("[jrnl-recovery]  replayed fs_blk=%llu "
                           "from log_blk=%u\n",
                           (unsigned long long)e->fs_blocknr,
                           e->log_blocknr);
        }
    }
    return UIOX_JR_OK;
}

/* =========================================================================
 * Full recovery entry point
 * ====================================================================== */
uiox_jr_err_t uiox_jr_recover(uiox_jr_ctx_t            *ctx,
                                uiox_jr_recovery_stats_t *stats)
{
    if (!ctx) return UIOX_JR_ERR_INVAL;

    uiox_jr_recovery_stats_t local_stats;
    rc_memset(&local_stats, 0, sizeof(local_stats));
    if (!stats) stats = &local_stats;

    uiox_fw_printf("[jrnl-recovery] === Journal Recovery Start ===\n");

    /* Already clean? */
    if (ctx->jsb.log_start == 0u) {
        uiox_fw_printf("[jrnl-recovery] Journal is clean — skipping.\n");
        stats->clean = true;
        if (stats != &local_stats) uiox_jr_recovery_print(stats);
        return UIOX_JR_OK;
    }

    /* Scan */
    static uiox_jr_replay_entry_t s_map[UIOX_JR_REPLAY_MAP_SIZE];
    uint32_t map_count = 0u;
    uint32_t last_seq  = 0u;

    uiox_jr_err_t rc = uiox_jr_scan(ctx, s_map, &map_count, &last_seq);
    if (rc != UIOX_JR_OK) {
        uiox_fw_printf("[jrnl-recovery] Scan failed: %d\n", rc);
        return rc;
    }

    stats->txns_found = (last_seq > 0u) ? last_seq : 0u;

    if (map_count == 0u) {
        uiox_fw_printf("[jrnl-recovery] Nothing to replay.\n");
        stats->clean = true;
        uiox_jr_recovery_print(stats);
        return UIOX_JR_OK;
    }

    /* Replay */
    rc = uiox_jr_replay(ctx, s_map, map_count, stats);
    if (rc != UIOX_JR_OK) {
        uiox_fw_printf("[jrnl-recovery] Replay failed: %d\n", rc);
        return rc;
    }

    /* Mark journal clean in superblock */
    ctx->jsb.log_start = 0u;
    ctx->jsb.sequence  = last_seq + 1u;

    uiox_fw_printf("[jrnl-recovery] === Recovery Complete ===\n");
    uiox_jr_recovery_print(stats);
    return UIOX_JR_OK;
}

/* =========================================================================
 * Print recovery stats
 * ====================================================================== */
void uiox_jr_recovery_print(const uiox_jr_recovery_stats_t *s)
{
    if (!s) return;
    uiox_fw_printf("[jrnl-recovery] Stats:\n");
    uiox_fw_printf("  clean          : %s\n",  s->clean ? "YES" : "NO");
    uiox_fw_printf("  txns_found     : %u\n",  s->txns_found);
    uiox_fw_printf("  txns_partial   : %u\n",  s->txns_partial);
    uiox_fw_printf("  blocks_replayed: %u\n",  s->blocks_replayed);
    uiox_fw_printf("  blocks_revoked : %u\n",  s->blocks_revoked);
    uiox_fw_printf("  corrupt_blocks : %u\n",  s->corrupt_blocks);
}
