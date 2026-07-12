/**
 * @file  uiox_jrnl.c
 * @brief UIOX Filesystem Journaling — lifecycle, commit, checkpoint,
 *        VFS hooks, syscall handlers.
 * @date  2026-07-08
 */
#include "../include/uiox_jrnl.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void jr_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void jr_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

/* CRC-32 */
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

/* ── Platform write hook ─────────────────────────────────────────────── */
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

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_jr_err_t uiox_jr_init(uiox_jr_ctx_t *ctx,
                             uint64_t       log_dev_base,
                             uint32_t       log_blocks,
                             uiox_jr_mode_t mode,
                             uint64_t     (*get_time_ms)(void))
{
    if (!ctx || log_blocks < UIOX_JR_MIN_LOG_BLOCKS)
        return UIOX_JR_ERR_INVAL;

    jr_memset(ctx, 0, sizeof(*ctx));

    ctx->log_dev_base       = log_dev_base;
    ctx->log_blocks         = log_blocks;
    ctx->log_head           = 1u;    /* block 0 = superblock */
    ctx->log_tail           = 1u;
    ctx->free_blocks        = log_blocks - 1u;
    ctx->mode               = mode;
    ctx->commit_interval_ms = 5000u; /* 5 s default */
    ctx->get_time_ms        = get_time_ms ? get_time_ms
                                          : uiox_jr_plat_get_time_ms;

    /* Populate in-memory superblock */
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

    uiox_fw_printf("[jrnl] mount — running recovery...\n");

    uiox_jr_err_t rc = uiox_jr_recover(ctx, stats);
    if (rc != UIOX_JR_OK) {
        uiox_fw_printf("[jrnl] recovery failed: %s\n", uiox_jr_err_str(rc));
        return rc;
    }

    ctx->mounted         = true;
    ctx->last_commit_ms  = ctx->get_time_ms();
    uiox_fw_printf("[jrnl] mounted OK (mode=%s)\n",
                   uiox_jr_mode_str(ctx->mode));
    return UIOX_JR_OK;
}

/* =========================================================================
 * Internal: write descriptor + data blocks + commit to log
 * ====================================================================== */
static uiox_jr_err_t do_commit(uiox_jr_ctx_t *ctx)
{
    if (!ctx->tx_open) return UIOX_JR_OK;  /* nothing to commit */

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    if (tx->block_count == 0u && tx->revoke_count == 0u) {
        ctx->tx_open      = false;
        tx->state         = UIOX_JR_TX_INACTIVE;
        return UIOX_JR_OK;
    }

    tx->state = UIOX_JR_TX_LOCKED;

    /* ── 1. Write descriptor block ───────────────────────────────────── */
    static uint8_t desc_buf[UIOX_JR_BLOCK_SIZE];
    jr_memset(desc_buf, 0, UIOX_JR_BLOCK_SIZE);

    uiox_jr_desc_hdr_t *dh = (uiox_jr_desc_hdr_t *)desc_buf;
    dh->magic     = UIOX_JR_DESC_MAGIC;
    dh->blocktype = 1u;
    dh->sequence  = tx->tid;

    /* Write block tags */
    uint32_t tag_off = (uint32_t)sizeof(uiox_jr_desc_hdr_t);
    for (uint32_t i = 0; i < tx->block_count; i++) {
        if (tag_off + (uint32_t)sizeof(uiox_jr_block_tag_t)
                > UIOX_JR_BLOCK_SIZE) break;

        uiox_jr_block_tag_t *tag =
            (uiox_jr_block_tag_t *)(desc_buf + tag_off);
        tag->blocknr  = tx->blocks[i].fs_blocknr;
        tag->checksum = tx->blocks[i].checksum;
        tag->flags    = (tx->blocks[i].escaped ? UIOX_JR_FLAG_ESCAPED : 0u)
                      | (i == tx->block_count - 1u
                         ? UIOX_JR_FLAG_LAST_TAG : 0u);
        tag_off += (uint32_t)sizeof(uiox_jr_block_tag_t);
    }

    dh->checksum = jr_crc32(desc_buf, UIOX_JR_BLOCK_SIZE);

    uiox_jr_err_t rc = uiox_jr_plat_log_write(ctx->log_dev_base,
                                                ctx->log_head, desc_buf);
    if (rc != UIOX_JR_OK) goto io_err;
    ctx->log_head = (ctx->log_head + 1u) % ctx->log_blocks;
    if (ctx->free_blocks) ctx->free_blocks--;

    /* ── 2. Write data blocks ────────────────────────────────────────── */
    tx->state = UIOX_JR_TX_FLUSH;
    for (uint32_t i = 0; i < tx->block_count; i++) {
        uiox_jr_logged_block_t *lb = &tx->blocks[i];
        if (!lb->data) continue;

        static uint8_t data_buf[UIOX_JR_BLOCK_SIZE];
        jr_memcpy(data_buf, lb->data, UIOX_JR_BLOCK_SIZE);

        /* Escape: overwrite first 4 bytes so recovery doesn't mistake it
         * for a descriptor block */
        if (lb->escaped) {
            uint32_t esc = 0u;  /* zeroed — recovery un-escapes on read */
            jr_memcpy(data_buf, &esc, 4u);
        }

        rc = uiox_jr_plat_log_write(ctx->log_dev_base,
                                     ctx->log_head, data_buf);
        if (rc != UIOX_JR_OK) goto io_err;
        ctx->log_head = (ctx->log_head + 1u) % ctx->log_blocks;
        if (ctx->free_blocks) ctx->free_blocks--;
    }

    /* ── 3. Write revoke block (if needed) ───────────────────────────── */
    if (tx->revoke_count > 0u) {
        static uint8_t rev_buf[UIOX_JR_BLOCK_SIZE];
        jr_memset(rev_buf, 0, UIOX_JR_BLOCK_SIZE);

        uiox_jr_revoke_hdr_t *rh = (uiox_jr_revoke_hdr_t *)rev_buf;
        rh->magic     = UIOX_JR_REVOKE_MAGIC;
        rh->blocktype = 3u;
        rh->sequence  = tx->tid;
        rh->count     = tx->revoke_count;

        uint32_t roff = (uint32_t)sizeof(uiox_jr_revoke_hdr_t);
        for (uint32_t i = 0; i < tx->revoke_count &&
                              roff + 8u <= UIOX_JR_BLOCK_SIZE;
             i++, roff += 8u) {
            jr_memcpy(rev_buf + roff, &tx->revoked[i], 8u);
        }

        rc = uiox_jr_plat_log_write(ctx->log_dev_base,
                                     ctx->log_head, rev_buf);
        if (rc != UIOX_JR_OK) goto io_err;
        ctx->log_head = (ctx->log_head + 1u) % ctx->log_blocks;
        if (ctx->free_blocks) ctx->free_blocks--;
    }

    /* ── 4. Write commit block ───────────────────────────────────────── */
    tx->state = UIOX_JR_TX_COMMIT;
    static uint8_t cmit_buf[UIOX_JR_BLOCK_SIZE];
    jr_memset(cmit_buf, 0, UIOX_JR_BLOCK_SIZE);

    uiox_jr_commit_t *cm = (uiox_jr_commit_t *)cmit_buf;
    cm->magic       = UIOX_JR_COMMIT_MAGIC;
    cm->blocktype   = 2u;
    cm->sequence    = tx->tid;
    cm->commit_time = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
    cm->checksum    = tx->data_checksum ^ 0xFFFFFFFFu;

    rc = uiox_jr_plat_log_write(ctx->log_dev_base,
                                 ctx->log_head, cmit_buf);
    if (rc != UIOX_JR_OK) goto io_err;

    tx->commit_time = cm->commit_time;
    ctx->log_head   = (ctx->log_head + 1u) % ctx->log_blocks;
    ctx->last_commit_ms = tx->commit_time;

    uiox_fw_printf("[jrnl] committed tx=%u  blocks=%u  revokes=%u\n",
                   tx->tid, tx->block_count, tx->revoke_count);

    tx->state    = UIOX_JR_TX_CHECKPOINT;
    ctx->tx_open = false;
    return UIOX_JR_OK;

io_err:
    uiox_fw_printf("[jrnl] I/O error during commit tx=%u\n", tx->tid);
    uiox_jr_abort(ctx, UIOX_JR_ERR_IO);
    return UIOX_JR_ERR_IO;
}

/* =========================================================================
 * uiox_jr_force_commit
 * ====================================================================== */
uiox_jr_err_t uiox_jr_force_commit(uiox_jr_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;
    if (ctx->aborted)              return UIOX_JR_ERR_ABORT;

    uiox_fw_printf("[jrnl] force commit\n");
    return do_commit(ctx);
}

/* =========================================================================
 * uiox_jr_tick — periodic commit check (called from scheduler)
 * ====================================================================== */
uiox_jr_err_t uiox_jr_tick(uiox_jr_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized || ctx->aborted) return UIOX_JR_ERR_INVAL;
    if (!ctx->tx_open) return UIOX_JR_OK;

    uint64_t now_ms  = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
    uint64_t elapsed = now_ms - ctx->last_commit_ms;

    if (elapsed >= ctx->commit_interval_ms) {
        uiox_fw_printf("[jrnl] tick: commit interval elapsed (%llu ms)\n",
                       (unsigned long long)elapsed);
        return do_commit(ctx);
    }
    return UIOX_JR_OK;
}

/* =========================================================================
 * uiox_jr_checkpoint — write committed blocks to home locations
 * ====================================================================== */
uiox_jr_err_t uiox_jr_checkpoint(uiox_jr_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;
    if (ctx->aborted)              return UIOX_JR_ERR_ABORT;

    uiox_jr_transaction_t *tx = &ctx->current_tx;
    if (tx->state != UIOX_JR_TX_CHECKPOINT) return UIOX_JR_OK;

    extern uiox_jr_err_t uiox_jr_plat_fs_write(uint64_t, const void *);

    uiox_fw_printf("[jrnl] checkpoint: writing %u blocks to fs\n",
                   tx->block_count);

    for (uint32_t i = 0; i < tx->block_count; i++) {
        uiox_jr_logged_block_t *lb = &tx->blocks[i];
        if (lb->revoked || !lb->data) continue;

        uiox_jr_err_t rc = uiox_jr_plat_fs_write(lb->fs_blocknr, lb->data);
        if (rc != UIOX_JR_OK) {
            uiox_fw_printf("[jrnl] checkpoint I/O error fs_blk=%llu\n",
                           (unsigned long long)lb->fs_blocknr);
            uiox_jr_abort(ctx, UIOX_JR_ERR_IO);
            return rc;
        }
    }

    /* Advance tail to free log space */
    ctx->log_tail  = ctx->log_head;
    ctx->free_blocks = ctx->log_blocks - 1u;
    tx->state      = UIOX_JR_TX_INACTIVE;

    /* Mark journal clean */
    ctx->jsb.log_start = 0u;
    uiox_fw_printf("[jrnl] checkpoint complete — log free blocks: %u\n",
                   ctx->free_blocks);
    return UIOX_JR_OK;
}

/* =========================================================================
 * uiox_jr_unmount — flush + clean superblock
 * ====================================================================== */
uiox_jr_err_t uiox_jr_unmount(uiox_jr_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_JR_ERR_INVAL;

    uiox_fw_printf("[jrnl] unmount — flushing...\n");

    uiox_jr_err_t rc = uiox_jr_force_commit(ctx);
    if (rc == UIOX_JR_OK) rc = uiox_jr_checkpoint(ctx);

    ctx->mounted = false;
    uiox_fw_printf("[jrnl] unmounted %s\n", uiox_jr_err_str(rc));
    return rc;
}

/* =========================================================================
 * uiox_jr_abort
 * ====================================================================== */
void uiox_jr_abort(uiox_jr_ctx_t *ctx, uiox_jr_err_t reason)
{
    if (!ctx) return;
    ctx->aborted         = true;
    ctx->jsb.errno       = (uint32_t)(-(int)reason);
    ctx->mounted         = false;
    uiox_fw_printf("[jrnl] ABORT: reason=%s — journal is read-only\n",
                   uiox_jr_err_str(reason));
}

/* =========================================================================
 * VFS integration hooks
 * ====================================================================== */

/* One shared implicit handle for VFS paths that don't manage their own */
static uiox_jr_handle_t *s_vfs_handle = NULL;

uiox_jr_err_t uiox_jr_vfs_get_write_access(uiox_jr_ctx_t *ctx,
                                             uint64_t       fs_blocknr,
                                             void          *buf)
{
    if (!ctx || !ctx->mounted) return UIOX_JR_ERR_INVAL;
    if (!s_vfs_handle) {
        s_vfs_handle = uiox_jr_start(ctx, UIOX_JR_MAX_BLOCKS_PER_TX / 4u);
        if (!s_vfs_handle) return UIOX_JR_ERR_NOMEM;
    }
    return uiox_jr_get_write_access(s_vfs_handle, ctx, fs_blocknr, buf);
}

uiox_jr_err_t uiox_jr_vfs_dirty_metadata(uiox_jr_ctx_t *ctx,
                                           uint64_t       fs_blocknr,
                                           const void    *buf)
{
    if (!ctx || !ctx->mounted) return UIOX_JR_ERR_INVAL;
    if (!s_vfs_handle) return UIOX_JR_ERR_NOTOPEN;
    return uiox_jr_dirty_metadata(s_vfs_handle, ctx, fs_blocknr, buf);
}

uiox_jr_err_t uiox_jr_vfs_revoke(uiox_jr_ctx_t *ctx, uint64_t fs_blocknr)
{
    if (!ctx || !ctx->mounted) return UIOX_JR_ERR_INVAL;
    if (!s_vfs_handle) return UIOX_JR_ERR_NOTOPEN;
    return uiox_jr_revoke(s_vfs_handle, ctx, fs_blocknr);
}

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */
static uiox_jr_ctx_t *g_jrnl_ctx = NULL;
void uiox_jr_set_global_ctx(uiox_jr_ctx_t *ctx) { g_jrnl_ctx = ctx; }

long sys_sync(long a0, long a1, long a2, long a3)
{
    (void)a0; (void)a1; (void)a2; (void)a3;
    if (!g_jrnl_ctx) return (long)UIOX_JR_ERR_INVAL;
    uiox_jr_err_t rc = uiox_jr_force_commit(g_jrnl_ctx);
    if (rc == UIOX_JR_OK) rc = uiox_jr_checkpoint(g_jrnl_ctx);
    return (long)rc;
}

long sys_fsync(long fd, long a1, long a2, long a3)
{
    (void)fd; (void)a1; (void)a2; (void)a3;
    if (!g_jrnl_ctx) return (long)UIOX_JR_ERR_INVAL;
    return (long)uiox_jr_force_commit(g_jrnl_ctx);
}

long sys_fdatasync(long fd, long a1, long a2, long a3)
{
    (void)fd; (void)a1; (void)a2; (void)a3;
    /* Data-only: commit but skip superblock metadata flush */
    if (!g_jrnl_ctx) return (long)UIOX_JR_ERR_INVAL;
    return (long)uiox_jr_force_commit(g_jrnl_ctx);
}

long sys_syncfs(long fd, long a1, long a2, long a3)
{
    (void)fd; (void)a1; (void)a2; (void)a3;
    return sys_sync(0, 0, 0, 0);
}

/* =========================================================================
 * Diagnostics
 * ====================================================================== */
void uiox_jr_print_super(const uiox_jr_ctx_t *ctx)
{
    if (!ctx) return;
    const uiox_jr_super_t *s = &ctx->jsb;
    uiox_fw_printf("[jrnl] Superblock:\n");
    uiox_fw_printf("  version    : %u\n",  s->format_version);
    uiox_fw_printf("  block_size : %u\n",  s->block_size);
    uiox_fw_printf("  log_blocks : %u\n",  s->log_blocks);
    uiox_fw_printf("  sequence   : %u\n",  s->sequence);
    uiox_fw_printf("  log_start  : %u\n",  s->log_start);
    uiox_fw_printf("  errno      : %u\n",  s->errno);
    uiox_fw_printf("  mode       : %s\n",  uiox_jr_mode_str(ctx->mode));
    uiox_fw_printf("  aborted    : %s\n",  ctx->aborted ? "YES" : "NO");
}

void uiox_jr_print_tx(const uiox_jr_ctx_t *ctx)
{
    if (!ctx || !ctx->tx_open) {
        uiox_fw_printf("[jrnl] No open transaction.\n");
        return;
    }
    const uiox_jr_transaction_t *tx = &ctx->current_tx;
    uiox_fw_printf("[jrnl] Transaction tid=%u  state=%s  "
                   "blocks=%u  revokes=%u  handles=%u\n",
                   tx->tid,
                   uiox_jr_tx_state_str(tx->state),
                   tx->block_count,
                   tx->revoke_count,
                   tx->handle_count);
}

void uiox_jr_print_stats(const uiox_jr_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[jrnl] Stats:\n");
    uiox_fw_printf("  log_head   : %u\n",  ctx->log_head);
    uiox_fw_printf("  log_tail   : %u\n",  ctx->log_tail);
    uiox_fw_printf("  free_blocks: %u\n",  ctx->free_blocks);
    uiox_fw_printf("  mounted    : %s\n",  ctx->mounted  ? "YES" : "NO");
    uiox_fw_printf("  aborted    : %s\n",  ctx->aborted  ? "YES" : "NO");
    uiox_fw_printf("  tx_open    : %s\n",  ctx->tx_open  ? "YES" : "NO");
}
