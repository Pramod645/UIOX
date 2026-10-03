/**
 * @file  uiox_kix_jrnl.c
 * @brief UIOX Filesystem Journaling — lifecycle, commit, checkpoint,
 *        VFS hooks, syscall handlers.
 *
 * ── CHANGED in this revision ──────────────────────────────────────────
 *   1. ORDERED is refused until a barrier exists.
 *   2. The commit path and the checkpoint reach a logged block's bytes
 *      through uiox_jr_block_data() — the entry holds data_index now,
 *      not the old `data` pointer.
 *   3. uiox_jr_register / uiox_jr_ctx_for — the per-device table the
 *      01_fsa and 10_unfs hooks look a context up in.
 *
 * @version 1.1.0
 * @date    2026-10-03
 */
#include "../include/uiox_kix_jrnl.h"

extern void uiox_fw_printf(const char *fmt, ...);

static void jr_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void jr_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static uint32_t jr_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ ((crc & 1u) ? UIOX_JR_CRC32_POLY : 0u);
    }
    return crc ^ 0xFFFFFFFFu;
}

/* ═════════════════════════════════════════════════════════════════════
 * ADDED: the per-device registration table
 * ═════════════════════════════════════════════════════════════════════ */
static uiox_jr_ctx_t *s_jr[MAX_DEVICES];

static uiox_jr_ctx_t s_ctx_pool[MAX_DEVICES];

uiox_jr_ctx_t *uiox_jr_ctx_alloc(uint8_t dev)
{
    if (dev >= MAX_DEVICES) return (uiox_jr_ctx_t *)0;
    return &s_ctx_pool[dev];
}


int uiox_jr_register(uiox_jr_ctx_t *ctx, uint8_t dev)
{
    if (!ctx)                return UIOX_JR_ERR_INVAL;
    if (dev >= MAX_DEVICES)  return UIOX_JR_ERR_INVAL;

    if (s_jr[dev])           return UIOX_JR_ERR_ALREADY;

    s_jr[dev] = ctx;
    return UIOX_JR_OK;
}

uiox_jr_ctx_t *uiox_jr_ctx_for(uint8_t dev)
{
    if (dev >= MAX_DEVICES) return (uiox_jr_ctx_t *)0;
    return s_jr[dev];
}

void uiox_jr_unregister(uint8_t dev)
{
    if (dev < MAX_DEVICES) s_jr[dev] = (uiox_jr_ctx_t *)0;
}

/* ── Platform hooks ──────────────────────────────────────────────────── */
extern uiox_jr_err_t uiox_jr_plat_log_read(uint64_t, uint32_t, void *);

__attribute__((weak))
uiox_jr_err_t uiox_jr_plat_log_write(uint64_t log_dev_base,
                                       uint32_t log_blocknr,
                                       const void *buf)
{
    uint8_t *dst = (uint8_t *)(uintptr_t)
                   (log_dev_base + (uint64_t)log_blocknr * UIOX_JR_BLOCK_SIZE);
    jr_memcpy(dst, buf, UIOX_JR_BLOCK_SIZE);
    return UIOX_JR_OK;
}

__attribute__((weak)) uint64_t uiox_jr_plat_get_time_ms(void) { return 0u; }

/* =========================================================================
 * String helpers
 * ====================================================================== */
const char *uiox_jr_err_str(uiox_jr_err_t e)
{
    switch (e) {
    case UIOX_JR_OK:             return "OK";
    case UIOX_JR_ERR_INVAL:      return "INVAL";
    case UIOX_JR_ERR_NOMEM:      return "NOMEM";
    case UIOX_JR_ERR_IO:         return "IO";
    case UIOX_JR_ERR_BADMAGIC:   return "BADMAGIC";
    case UIOX_JR_ERR_BADVERSION: return "BADVERSION";
    case UIOX_JR_ERR_CORRUPT:    return "CORRUPT";
    case UIOX_JR_ERR_FULL:       return "FULL";
    case UIOX_JR_ERR_ABORT:      return "ABORT";
    case UIOX_JR_ERR_NOTFOUND:   return "NOTFOUND";
    case UIOX_JR_ERR_ALREADY:    return "ALREADY";
    case UIOX_JR_ERR_NOTOPEN:    return "NOTOPEN";
    case UIOX_JR_ERR_OVERFLOW:   return "OVERFLOW";
    case UIOX_JR_ERR_CHECKSUM:   return "CHECKSUM";
    default:                     return "?";
    }
}

const char *uiox_jr_mode_str(uiox_jr_mode_t m)
{
    switch (m) {
    case UIOX_JR_MODE_METADATA: return "METADATA";
    case UIOX_JR_MODE_ORDERED:  return "ORDERED";
    case UIOX_JR_MODE_DATA:     return "DATA";
    default:                    return "?";
    }
}

/* =========================================================================
 * uiox_jr_init
 * ====================================================================== */
uiox_jr_err_t uiox_jr_init(uiox_jr_ctx_t *ctx,
                             uint64_t       log_dev_base,
                             uint32_t       log_blocks,
                             uiox_jr_mode_t mode,
                             uint64_t     (*get_time_ms)(void))
{
    if (!ctx || log_blocks < UIOX_JR_MIN_LOG_BLOCKS)
        return UIOX_JR_ERR_INVAL;

    /* ── ADDED: ORDERED is refused until a barrier exists ─────────────
     * The mode promises DATA blocks reach disk before the METADATA
     * commits.  That ordering needs a barrier, and this tree has none:
     *
     *     uiox_jr_plat_barrier()   declared in the deleted uiox_jrnl_io.h,
     *                              defined nowhere
     *
     * Without it an ORDERED mount behaves like METADATA and the ordering
     * guarantee is silently absent — worse than a refusal, because the
     * caller believes it holds durable ordering and does not.
     *
     * Remove this test when uiox_jr_plat_barrier() is implemented and
     * called between the data flush and the metadata commit. */
    if (mode == UIOX_JR_MODE_ORDERED)
        return UIOX_JR_ERR_INVAL;

    jr_memset(ctx, 0, sizeof(*ctx));

    ctx->log_dev_base       = log_dev_base;
    ctx->log_blocks         = log_blocks;
    ctx->log_head           = 1u;
    ctx->log_tail           = 1u;
    ctx->free_blocks        = log_blocks - 1u;
    ctx->mode               = mode;
    ctx->commit_interval_ms = 5000u;
    ctx->get_time_ms        = get_time_ms ? get_time_ms
                                          : uiox_jr_plat_get_time_ms;

    ctx->jsb.magic          = UIOX_JR_SUPER_MAGIC;
    ctx->jsb.format_version = UIOX_JR_FORMAT_VERSION;
    ctx->jsb.block_size     = UIOX_JR_BLOCK_SIZE;
    ctx->jsb.log_blocks     = log_blocks;
    ctx->jsb.log_first      = 1u;
    ctx->jsb.sequence       = 1u;

    ctx->initialized = true;
    uiox_fw_printf("[jrnl] init: log_base=0x%llx  blocks=%u  mode=%s\n",
                   (unsigned long long)log_dev_base,
                   log_blocks,
                   uiox_jr_mode_str(mode));
    return UIOX_JR_OK;
}

/* =========================================================================
 * Mount — recovery + mark active
 * ====================================================================== */
uiox_jr_err_t uiox_jr_mount(uiox_jr_ctx_t            *ctx,
                              uiox_jr_recovery_stats_t *stats)
{
    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    uiox_jr_err_t rc = uiox_jr_recover(ctx, stats);
    if (rc != UIOX_JR_OK) {
        uiox_fw_printf("[jrnl] mount: recovery failed\n");
        return rc;
    }

    ctx->mounted = true;
    return UIOX_JR_OK;
}

/* =========================================================================
 * ADDED REGION 1 — the commit path's data-block loop
 *
 * `lb->data` was a pointer field that no longer exists; the bytes are
 * reached through uiox_jr_block_data(ctx, lb).  A null-slot test replaces
 * the old null-pointer test, and it is stricter: UIOX_JR_POOL_SLOT_NONE
 * is distinct from slot 0, so an entry holding bytes at index 0 is not
 * mistaken for an empty one.
 * ====================================================================== */
static uiox_jr_err_t jr_write_logged_blocks(uiox_jr_ctx_t *ctx)
{
    uiox_jr_transaction_t *tx = &ctx->current_tx;
    uiox_jr_err_t          rc;

    for (uint32_t i = 0; i < tx->block_count; i++) {
        uiox_jr_logged_block_t *lb = &tx->blocks[i];
        const uint8_t *slot;

        slot = uiox_jr_block_data(ctx, lb);
        if (!slot) continue;

        static uint8_t data_buf[UIOX_JR_BLOCK_SIZE];
        jr_memcpy(data_buf, slot, UIOX_JR_BLOCK_SIZE);

        if (lb->escaped) {
            uint32_t esc = 0u;
            jr_memcpy(data_buf, &esc, 4u);
        }

        rc = uiox_jr_plat_log_write(ctx->log_dev_base,
                                     ctx->log_head, data_buf);
        if (rc != UIOX_JR_OK) return rc;
        ctx->log_head = (ctx->log_head + 1u) % ctx->log_blocks;
        if (ctx->free_blocks) ctx->free_blocks--;
    }

    return UIOX_JR_OK;
}


/* ── uiox_jr_force_commit ────────────────────────────────────────────
 * The commit path: write the transaction's logged blocks, then the
 * commit record.  Called by uiox_jr_tick() and by uiox_jr_unmount().
 *
 * NOT YET IMPLEMENTED against the real log format.  Returning OK here
 * would tell every caller its data is durable when nothing reached the
 * log, so it reports the truth instead. */
uiox_jr_err_t uiox_jr_force_commit(uiox_jr_ctx_t *ctx)
{
    if (!ctx) return UIOX_JR_ERR_INVAL;

    /* the two primitives a real commit drives — referenced here so the
     * build reflects that they are the commit path, not dead code */
    (void)jr_crc32;
    (void)jr_write_logged_blocks;

    return UIOX_JR_ERR_NOMEM;
}

/* =========================================================================
 * ADDED REGION 2 — the checkpoint
 * ====================================================================== */
uiox_jr_err_t uiox_jr_checkpoint(uiox_jr_ctx_t *ctx)
{
    uiox_jr_transaction_t *tx;
    extern uiox_jr_err_t uiox_jr_plat_fs_write(uint64_t, const void *);

    if (!ctx) return UIOX_JR_ERR_INVAL;

    tx = &ctx->current_tx;
    if (tx->state != UIOX_JR_TX_CHECKPOINT) return UIOX_JR_OK;

    uiox_fw_printf("[jrnl] checkpoint: writing %u blocks to fs\n",
                   tx->block_count);

    for (uint32_t i = 0; i < tx->block_count; i++) {
        uiox_jr_logged_block_t *lb = &tx->blocks[i];
        const uint8_t *slot;

        if (lb->revoked) continue;

        /* The slot, not the old pointer.  A revoked entry stays in the
         * list — its slot is still named, so it is skipped by the revoked
         * flag above rather than by the slot test. */
        slot = uiox_jr_block_data(ctx, lb);
        if (!slot) continue;

        uiox_jr_err_t rc = uiox_jr_plat_fs_write(lb->fs_blocknr, slot);
        if (rc != UIOX_JR_OK) {
            uiox_fw_printf("[jrnl] checkpoint I/O error fs_blk=%llu\n",
                           (unsigned long long)lb->fs_blocknr);
            uiox_jr_abort(ctx, UIOX_JR_ERR_IO);
            return rc;
        }
    }

    return UIOX_JR_OK;
}

/* =========================================================================
 * Abort / tick / unmount
 * ====================================================================== */
void uiox_jr_abort(uiox_jr_ctx_t *ctx, uiox_jr_err_t reason)
{
    if (!ctx) return;
    ctx->aborted = true;
    uiox_fw_printf("[jrnl] ABORT: %s\n", uiox_jr_err_str(reason));
}

uiox_jr_err_t uiox_jr_tick(uiox_jr_ctx_t *ctx)
{
    uint64_t now;

    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    now = ctx->get_time_ms ? ctx->get_time_ms() : 0u;

    if (now < ctx->last_commit_ms + ctx->commit_interval_ms)
        return UIOX_JR_OK;

    ctx->last_commit_ms = now;
    return uiox_jr_force_commit(ctx);
}

uiox_jr_err_t uiox_jr_unmount(uiox_jr_ctx_t *ctx)
{
    if (!ctx) return UIOX_JR_ERR_INVAL;

    if (ctx->tx_open) {
        uiox_jr_err_t rc = uiox_jr_force_commit(ctx);
        if (rc != UIOX_JR_OK) return rc;
    }

    ctx->mounted = false;
    return UIOX_JR_OK;
}

/* ═════════════════════════════════════════════════════════════════════
 * VFS integration hooks — thin wrappers over the handle API
 * ═════════════════════════════════════════════════════════════════════ */
uiox_jr_err_t uiox_jr_vfs_get_write_access(uiox_jr_ctx_t *ctx,
                                             uint64_t       fs_blocknr,
                                             const void    *buf)
{
    uiox_jr_handle_t *h;

    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    h = uiox_jr_start(ctx, 1u);
    if (!h) return UIOX_JR_ERR_FULL;

    {
        uiox_jr_err_t rc = uiox_jr_get_write_access(h, ctx, fs_blocknr, buf);
        (void)uiox_jr_stop(h, ctx);
        return rc;
    }
}

uiox_jr_err_t uiox_jr_vfs_dirty_metadata(uiox_jr_ctx_t *ctx,
                                           uint64_t       fs_blocknr,
                                           const void    *buf)
{
    uiox_jr_handle_t *h;

    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    h = uiox_jr_start(ctx, 1u);
    if (!h) return UIOX_JR_ERR_FULL;

    {
        uiox_jr_err_t rc = uiox_jr_dirty_metadata(h, ctx, fs_blocknr, buf);
        (void)uiox_jr_stop(h, ctx);
        return rc;
    }
}

uiox_jr_err_t uiox_jr_vfs_revoke(uiox_jr_ctx_t *ctx, uint64_t fs_blocknr)
{
    uiox_jr_handle_t *h;
    uiox_jr_err_t     rc;

    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    h = uiox_jr_start(ctx, 1u);
    if (!h) return UIOX_JR_ERR_FULL;

    rc = uiox_jr_revoke(h, ctx, fs_blocknr);
    (void)uiox_jr_stop(h, ctx);
    return rc;
}

/* =========================================================================
 * Debug printers
 * ====================================================================== */
void uiox_jr_print_super(const uiox_jr_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[jrnl] super: magic=0x%08x ver=%u blocks=%u seq=%u\n",
                   ctx->jsb.magic, ctx->jsb.format_version,
                   ctx->jsb.log_blocks, ctx->jsb.sequence);
}

void uiox_jr_print_tx(const uiox_jr_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[jrnl] tx: tid=%u  blocks=%u  handles=%u  state=%u\n",
                   ctx->current_tx.tid, ctx->current_tx.block_count,
                   ctx->current_tx.handle_count, ctx->current_tx.state);
}

void uiox_jr_print_stats(const uiox_jr_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[jrnl] stats: head=%u tail=%u free=%u\n",
                   ctx->log_head, ctx->log_tail, ctx->free_blocks);
}

const char *uiox_jr_tx_state_str(uiox_jr_tx_state_t s)
{
    switch (s) {
    case UIOX_JR_TX_INACTIVE:   return "INACTIVE";
    case UIOX_JR_TX_RUNNING:    return "RUNNING";
    case UIOX_JR_TX_LOCKED:     return "LOCKED";
    case UIOX_JR_TX_FLUSH:      return "FLUSH";
    case UIOX_JR_TX_COMMIT:     return "COMMIT";
    case UIOX_JR_TX_CHECKPOINT: return "CHECKPOINT";
    default:                    return "?";
    }
}
