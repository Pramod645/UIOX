/**
 * @file  uiox_jrnl_tx.c
 * @brief UIOX Filesystem Journaling — transaction and handle implementation.
 * @date  2026-07-08
 */
#include "../include/uiox_jrnl_tx.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void tx_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void tx_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

/* ── CRC-32 (IEEE 802.3) ─────────────────────────────────────────────── */
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

/* ── Static handle pool (no heap dependency) ─────────────────────────── */
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
        if (&s_handle_pool[i] == h) {
            s_handle_used[i] = false;
            return;
        }
    }
}

/* =========================================================================
 * Internal: begin a new transaction
 * ====================================================================== */
static uiox_jr_err_t tx_begin(uiox_jr_ctx_t *ctx)
{
    if (ctx->tx_open)   return UIOX_JR_ERR_ALREADY;
    if (ctx->aborted)   return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    tx_memset(tx, 0, sizeof(*tx));
    tx->tid          = ctx->jsb.sequence++;
    tx->state        = UIOX_JR_TX_RUNNING;
    tx->mode         = ctx->mode;
    tx->start_time   = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
    tx->data_checksum = 0xFFFFFFFFu;

    ctx->tx_open     = true;
    return UIOX_JR_OK;
}

/* =========================================================================
 * uiox_jr_start — open a handle in the current (or new) transaction
 * ====================================================================== */
uiox_jr_handle_t *uiox_jr_start(uiox_jr_ctx_t *ctx, uint32_t nblocks)
{
    if (!ctx || ctx->aborted) return NULL;
    if (nblocks == 0u || nblocks > UIOX_JR_MAX_BLOCKS_PER_TX) return NULL;

    /* Start a transaction if none is running */
    if (!ctx->tx_open) {
        if (tx_begin(ctx) != UIOX_JR_OK) return NULL;
    }

    uiox_jr_transaction_t *tx = &ctx->current_tx;

    /* Check there is room for this handle's reservation */
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

/* =========================================================================
 * uiox_jr_get_write_access — copy block into journal log before modification
 * ====================================================================== */
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

    /* Check if this block is already logged — update in place */
    for (uint32_t i = 0; i < tx->block_count; i++) {
        if (tx->blocks[i].fs_blocknr == fs_blocknr) {
            tx_memcpy(tx->blocks[i].data, buf, UIOX_JR_BLOCK_SIZE);
            tx->blocks[i].checksum = tx_crc32((const uint8_t *)buf,
                                               UIOX_JR_BLOCK_SIZE);
            return UIOX_JR_OK;
        }
    }

    /* New entry — use a static data store inside the transaction */
    uiox_jr_logged_block_t *lb = &tx->blocks[tx->block_count];
    lb->fs_blocknr = fs_blocknr;
    lb->revoked    = false;
    lb->escaped    = false;

    /*
     * In a real kernel: lb->data points into a buffer_head page.
     * Here we store the pointer and rely on the caller keeping the
     * buffer alive until commit. For a fully self-contained
     * implementation, replace with a static block data pool.
     */
    lb->data     = (uint8_t *)(uintptr_t)buf;  /* non-owning reference */
    lb->checksum = tx_crc32((const uint8_t *)buf, UIOX_JR_BLOCK_SIZE);

    /* Flag escape if block starts with a journal magic word */
    uint32_t first_word = 0u;
    tx_memcpy(&first_word, buf, 4u);
    if (first_word == UIOX_JR_DESC_MAGIC ||
        first_word == UIOX_JR_COMMIT_MAGIC)
        lb->escaped = true;

    tx->block_count++;
    h->used++;

    /* Rolling checksum over all block data */
    tx->data_checksum = tx_crc32((const uint8_t *)buf, UIOX_JR_BLOCK_SIZE)
                        ^ tx->data_checksum;

    return UIOX_JR_OK;
}

/* =========================================================================
 * uiox_jr_dirty_metadata — update log entry after modification
 * ====================================================================== */
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
            /* Update checksum to reflect new content */
            tx->blocks[i].checksum = tx_crc32((const uint8_t *)buf,
                                               UIOX_JR_BLOCK_SIZE);
            return UIOX_JR_OK;
        }
    }
    /* Block was not pre-logged — log it now (relaxed: metadata-only mode) */
    return uiox_jr_get_write_access(h, ctx, fs_blocknr, buf);
}

/* =========================================================================
 * uiox_jr_revoke — suppress future replay of a filesystem block
 * ====================================================================== */
uiox_jr_err_t uiox_jr_revoke(uiox_jr_handle_t *h,
                               uiox_jr_ctx_t    *ctx,
                               uint64_t          fs_blocknr)
{
    if (!h || !ctx)                 return UIOX_JR_ERR_INVAL;
    if (h->aborted || ctx->aborted) return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    if (tx->revoke_count >= UIOX_JR_MAX_REVOKE) return UIOX_JR_ERR_FULL;

    /* Avoid duplicates */
    for (uint32_t i = 0; i < tx->revoke_count; i++)
        if (tx->revoked[i] == fs_blocknr) return UIOX_JR_OK;

    tx->revoked[tx->revoke_count++] = fs_blocknr;

    /* Also mark any in-log copy of this block as revoked */
    for (uint32_t i = 0; i < tx->block_count; i++)
        if (tx->blocks[i].fs_blocknr == fs_blocknr)
            tx->blocks[i].revoked = true;

    return UIOX_JR_OK;
}

/* =========================================================================
 * uiox_jr_stop — release a handle; commit if last handle in tx
 * ====================================================================== */
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

/* =========================================================================
 * uiox_jr_is_aborted
 * ====================================================================== */
bool uiox_jr_is_aborted(const uiox_jr_ctx_t *ctx)
{
    return ctx ? ctx->aborted : true;
}
