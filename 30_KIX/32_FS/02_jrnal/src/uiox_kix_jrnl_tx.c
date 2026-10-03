/**
 * @file  uiox_kix_jrnl_tx.c
 * @brief UIOX Filesystem Journaling — transaction and handle implementation.
 *
 * ── CHANGED: THE LOG OWNS ITS DATA ────────────────────────────────────
 * uiox_jr_get_write_access used to store the CALLER'S buffer address.
 * bwrite(buf, true, false) RELEASES that buffer, so the commit read a
 * page that had already been handed to another claimant — foreign bytes
 * under this transaction's checksum, silently, because the checksum was
 * taken while the page was still ours.  The entry now holds data_index
 * into ctx->log_pool and the bytes are COPIED in.
 *
 * ── WHAT dirty_metadata NOW DOES ──────────────────────────────────────
 * It copies the NEW image into the same slot and recomputes the checksum.
 * The previous body only touched the checksum.
 *
 * @version 1.1.0
 * @date    2026-10-03
 */
#include "../include/uiox_kix_jrnl_tx.h"

extern void uiox_fw_printf(const char *fmt, ...);

static void tx_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void tx_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static uint32_t tx_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ ((crc & 1u) ? UIOX_JR_CRC32_POLY : 0u);
    }
    return crc ^ 0xFFFFFFFFu;
}

static uiox_jr_handle_t s_handle_pool[UIOX_JR_MAX_HANDLES];
static bool             s_handle_used[UIOX_JR_MAX_HANDLES];

static uiox_jr_handle_t *alloc_handle(void)
{
    for (uint32_t i = 0; i < UIOX_JR_MAX_HANDLES; i++) {
        if (!s_handle_used[i]) {
            s_handle_used[i] = true;
            tx_memset(&s_handle_pool[i], 0, sizeof(uiox_jr_handle_t));
            return &s_handle_pool[i];
        }
    }
    return NULL;
}

static void free_handle(uiox_jr_handle_t *h)
{
    if (!h) return;
    for (uint32_t i = 0; i < UIOX_JR_MAX_HANDLES; i++) {
        if (&s_handle_pool[i] == h) { s_handle_used[i] = false; return; }
    }
}

/* ── Log-pool helpers — the only code that touches ctx->log_pool ─────── */
static uint8_t *pool_slot(uiox_jr_ctx_t *ctx, uint16_t idx)
{
    if (!ctx)                          return (uint8_t *)0;
    if (idx == UIOX_JR_POOL_SLOT_NONE) return (uint8_t *)0;
    if (idx >= UIOX_JR_LOG_POOL_BLOCKS) return (uint8_t *)0;
    return &ctx->log_pool[idx][0];
}

static uint16_t pool_alloc(uiox_jr_ctx_t *ctx)
{
    const uiox_jr_transaction_t *tx = &ctx->current_tx;

    for (uint16_t cand = 0u; cand < (uint16_t)UIOX_JR_LOG_POOL_BLOCKS; cand++) {
        bool taken = false;

        for (uint32_t i = 0u; i < tx->block_count; i++) {
            if (tx->blocks[i].data_index == cand) { taken = true; break; }
        }

        if (!taken) return cand;
    }

    return UIOX_JR_POOL_SLOT_NONE;
}

static void pool_release_all(uiox_jr_ctx_t *ctx)
{
    uiox_jr_transaction_t *tx = &ctx->current_tx;

    for (uint32_t i = 0u; i < tx->block_count; i++)
        tx->blocks[i].data_index = UIOX_JR_POOL_SLOT_NONE;

    tx->block_count = 0u;
}

/* ── the read-only face of the pool ───────────────────────────────────
 * Used by the commit path and the checkpoint, which read a logged block's
 * bytes but never write them. */
const uint8_t *uiox_jr_block_data(const uiox_jr_ctx_t *ctx,
                                   const uiox_jr_logged_block_t *lb)
{
    if (!ctx || !lb)                                  return (const uint8_t *)0;
    if (lb->data_index == UIOX_JR_POOL_SLOT_NONE)     return (const uint8_t *)0;
    if (lb->data_index >= UIOX_JR_LOG_POOL_BLOCKS)    return (const uint8_t *)0;

    return &ctx->log_pool[lb->data_index][0];
}

static uiox_jr_err_t tx_begin(uiox_jr_ctx_t *ctx)
{
    if (ctx->tx_open)   return UIOX_JR_ERR_ALREADY;
    if (ctx->aborted)   return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;

    pool_release_all(ctx);

    tx_memset(tx, 0, sizeof(*tx));
    tx->tid          = ctx->jsb.sequence++;
    tx->state        = UIOX_JR_TX_RUNNING;
    tx->mode         = ctx->mode;
    tx->start_time   = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
    tx->data_checksum = 0xFFFFFFFFu;

    ctx->tx_open     = true;
    return UIOX_JR_OK;
}

uiox_jr_handle_t *uiox_jr_start(uiox_jr_ctx_t *ctx, uint32_t nblocks)
{
    if (!ctx || ctx->aborted) return NULL;
    if (nblocks == 0u || nblocks > UIOX_JR_MAX_BLOCKS_PER_TX) return NULL;

    if (!ctx->tx_open) {
        if (tx_begin(ctx) != UIOX_JR_OK) return NULL;
    }

    uiox_jr_transaction_t *tx = &ctx->current_tx;

    if (tx->block_count + nblocks > UIOX_JR_MAX_BLOCKS_PER_TX) {
        uiox_fw_printf("[jrnl] tx %u full — forcing commit before start\n",
                       tx->tid);
        if (uiox_jr_force_commit(ctx) != UIOX_JR_OK) return NULL;
        if (tx_begin(ctx) != UIOX_JR_OK)              return NULL;
        tx = &ctx->current_tx;
    }

    uiox_jr_handle_t *h = alloc_handle();
    if (!h) return NULL;

    h->tx_id    = tx->tid;
    h->reserved = nblocks;
    h->used     = 0u;
    h->aborted  = false;

    tx->handle_count++;
    return h;
}

uiox_jr_err_t uiox_jr_get_write_access(uiox_jr_handle_t *h,
                                         uiox_jr_ctx_t    *ctx,
                                         uint64_t          fs_blocknr,
                                         const void       *buf)
{
    if (!h || !ctx || !buf)                return UIOX_JR_ERR_INVAL;
    if (h->aborted || ctx->aborted)        return UIOX_JR_ERR_ABORT;
    if (h->used >= h->reserved)            return UIOX_JR_ERR_FULL;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    if (tx->block_count >= UIOX_JR_MAX_BLOCKS_PER_TX)
        return UIOX_JR_ERR_FULL;

    for (uint32_t i = 0; i < tx->block_count; i++) {
        if (tx->blocks[i].fs_blocknr == fs_blocknr) {
            uint8_t *slot = pool_slot(ctx, tx->blocks[i].data_index);

            if (!slot) return UIOX_JR_ERR_CORRUPT;

            tx_memcpy(slot, buf, UIOX_JR_BLOCK_SIZE);
            tx->blocks[i].checksum = tx_crc32((const uint8_t *)buf,
                                               UIOX_JR_BLOCK_SIZE);
            tx->blocks[i].revoked  = false;
            return UIOX_JR_OK;
        }
    }

    {
        uint16_t idx = pool_alloc(ctx);
        uint8_t *slot;

        if (idx == UIOX_JR_POOL_SLOT_NONE) {
            uiox_fw_printf("[jrnl] log pool exhausted at tx %u "
                           "(%u blocks)\n", tx->tid, tx->block_count);
            return UIOX_JR_ERR_FULL;
        }

        slot = pool_slot(ctx, idx);
        if (!slot) return UIOX_JR_ERR_CORRUPT;

        tx_memcpy(slot, buf, UIOX_JR_BLOCK_SIZE);

        uiox_jr_logged_block_t *lb = &tx->blocks[tx->block_count];
        lb->fs_blocknr = fs_blocknr;
        lb->data_index = idx;
        lb->revoked    = false;
        lb->escaped    = false;
        lb->checksum   = tx_crc32((const uint8_t *)buf, UIOX_JR_BLOCK_SIZE);

        {
            uint32_t first_word = 0u;
            tx_memcpy(&first_word, slot, 4u);
            if (first_word == UIOX_JR_DESC_MAGIC ||
                first_word == UIOX_JR_COMMIT_MAGIC)
                lb->escaped = true;
        }

        tx->block_count++;
        h->used++;

        tx->data_checksum = tx_crc32(slot, UIOX_JR_BLOCK_SIZE)
                            ^ tx->data_checksum;
    }

    return UIOX_JR_OK;
}

uiox_jr_err_t uiox_jr_dirty_metadata(uiox_jr_handle_t *h,
                                       uiox_jr_ctx_t    *ctx,
                                       uint64_t          fs_blocknr,
                                       const void       *buf)
{
    if (!h || !ctx || !buf)         return UIOX_JR_ERR_INVAL;
    if (h->aborted || ctx->aborted) return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;

    for (uint32_t i = 0; i < tx->block_count; i++) {
        if (tx->blocks[i].fs_blocknr == fs_blocknr) {
            uint8_t *slot = pool_slot(ctx, tx->blocks[i].data_index);
            uint32_t newsum;

            if (!slot) return UIOX_JR_ERR_CORRUPT;

            tx->data_checksum ^= tx->blocks[i].checksum;

            tx_memcpy(slot, buf, UIOX_JR_BLOCK_SIZE);
            newsum = tx_crc32(slot, UIOX_JR_BLOCK_SIZE);
            tx->blocks[i].checksum = newsum;

            tx->data_checksum ^= newsum;

            return UIOX_JR_OK;
        }
    }

    return uiox_jr_get_write_access(h, ctx, fs_blocknr, buf);
}

uiox_jr_err_t uiox_jr_revoke(uiox_jr_handle_t *h,
                               uiox_jr_ctx_t    *ctx,
                               uint64_t          fs_blocknr)
{
    if (!h || !ctx)                 return UIOX_JR_ERR_INVAL;
    if (h->aborted || ctx->aborted) return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    if (tx->revoke_count >= UIOX_JR_MAX_REVOKE) return UIOX_JR_ERR_FULL;

    for (uint32_t i = 0; i < tx->revoke_count; i++)
        if (tx->revoked[i] == fs_blocknr) return UIOX_JR_OK;

    tx->revoked[tx->revoke_count++] = fs_blocknr;

    for (uint32_t i = 0; i < tx->block_count; i++)
        if (tx->blocks[i].fs_blocknr == fs_blocknr)
            tx->blocks[i].revoked = true;

    return UIOX_JR_OK;
}

uiox_jr_err_t uiox_jr_stop(uiox_jr_handle_t *h, uiox_jr_ctx_t *ctx)
{
    if (!h || !ctx) return UIOX_JR_ERR_INVAL;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    bool sync = h->sync;

    if (tx->handle_count > 0u) tx->handle_count--;
    free_handle(h);

    if (sync && tx->handle_count == 0u)
        return uiox_jr_force_commit(ctx);

    return UIOX_JR_OK;
}

bool uiox_jr_is_aborted(const uiox_jr_ctx_t *ctx)
{
    return ctx ? ctx->aborted : true;
}
