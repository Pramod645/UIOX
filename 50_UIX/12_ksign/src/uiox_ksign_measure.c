/**
 * @file  uiox_ksign_measure.c
 * @brief UIOX Signed Kernel — TPM-style PCR measurement log.
 *
 * PCR extend semantics (identical to TPM 2.0):
 *   PCR_new = SHA-256(PCR_old || SHA-256(data))
 *
 * PCR assignment:
 *   PCR[0] — Firmware / bootloader
 *   PCR[1] — Kernel code (.text)
 *   PCR[2] — Kernel data / rodata
 *   PCR[3] — Kernel command-line
 *   PCR[4] — Loaded modules
 *   PCR[5] — Key load events
 *   PCR[6] — Signature verification events
 *   PCR[7] — Runtime integrity checks
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#include "../include/uiox_ksign_measure.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void ms_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void ms_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static void ms_strncpy(char *d, const char *s, size_t n)
{ size_t i = 0; while (i < n - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }

/* =========================================================================
 * Init — all PCRs start at all-zeros (same as TPM power-on state)
 * ====================================================================== */
uiox_ks_err_t uiox_ks_measure_init(uiox_ks_measure_ctx_t *ctx,
                                     uint64_t (*get_time_ms)(void))
{
    if (!ctx || !get_time_ms) return UIOX_KS_ERR_INVAL;

    ms_memset(ctx, 0, sizeof(*ctx));
    ctx->magic       = UIOX_KS_LOG_MAGIC;
    ctx->get_time_ms = get_time_ms;
    ctx->locked      = false;
    ctx->entry_count = 0u;
    /* PCRs already zero-initialised by memset */
    return UIOX_KS_OK;
}

/* =========================================================================
 * Internal: extend PCR[index] with a pre-computed hash
 *   PCR_new = SHA-256(PCR_old || measurement)
 * ====================================================================== */
static uiox_ks_err_t ms_extend_raw(uiox_ks_measure_ctx_t *ctx,
                                    uint32_t               index,
                                    const uint8_t          measurement[UIOX_KS_SHA256_LEN],
                                    const char            *event_name,
                                    uiox_ks_evt_type_t     evt_type)
{
    if (index >= UIOX_KS_PCR_MAX) return UIOX_KS_ERR_INVAL;
    if (ctx->locked)               return UIOX_KS_ERR_INVAL; /* frozen */

    /* Extend: concat(PCR_old, measurement) → SHA-256 */
    uiox_ks_sha256_ctx_t hctx;
    uiox_ks_sha256_init(&hctx);
    uiox_ks_sha256_update(&hctx, ctx->pcr[index], UIOX_KS_SHA256_LEN);
    uiox_ks_sha256_update(&hctx, measurement,     UIOX_KS_SHA256_LEN);
    uiox_ks_sha256_final (&hctx, ctx->pcr[index]);

    /* Append log entry if space available */
    if (ctx->entry_count < UIOX_KS_LOG_MAX_ENTRIES) {
        uiox_ks_log_entry_t *e = &ctx->entries[ctx->entry_count];
        ms_memset(e, 0, sizeof(*e));
        e->pcr_index   = index;
        e->event_type  = evt_type;
        ms_strncpy(e->event_name, event_name ? event_name : "?",
                   UIOX_KS_EVENT_NAME_LEN);
        ms_memcpy(e->measurement, measurement, UIOX_KS_SHA256_LEN);
        ms_memcpy(e->pcr_after,   ctx->pcr[index], UIOX_KS_SHA256_LEN);
        e->timestamp_ms = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
        ctx->entry_count++;
    }

    return UIOX_KS_OK;
}

/* =========================================================================
 * Public: extend PCR with raw data (hashed internally)
 * ====================================================================== */
uiox_ks_err_t uiox_ks_measure_extend(uiox_ks_measure_ctx_t *ctx,
                                       uint32_t               pcr_index,
                                       const uint8_t         *data,
                                       size_t                 data_len,
                                       const char            *event_name,
                                       uiox_ks_evt_type_t     evt_type)
{
    if (!ctx || !data || data_len == 0u) return UIOX_KS_ERR_INVAL;

    uint8_t hash[UIOX_KS_SHA256_LEN];
    uiox_ks_sha256(data, data_len, hash);
    return ms_extend_raw(ctx, pcr_index, hash, event_name, evt_type);
}

/* =========================================================================
 * Public: extend PCR with a pre-computed hash
 * ====================================================================== */
uiox_ks_err_t uiox_ks_measure_extend_hash(uiox_ks_measure_ctx_t *ctx,
                                            uint32_t               pcr_index,
                                            const uint8_t          hash[UIOX_KS_SHA256_LEN],
                                            const char            *event_name,
                                            uiox_ks_evt_type_t     evt_type)
{
    if (!ctx || !hash) return UIOX_KS_ERR_INVAL;
    return ms_extend_raw(ctx, pcr_index, hash, event_name, evt_type);
}

/* =========================================================================
 * Read PCR
 * ====================================================================== */
void uiox_ks_measure_read_pcr(const uiox_ks_measure_ctx_t *ctx,
                                uint32_t                     pcr_index,
                                uint8_t                      out[UIOX_KS_SHA256_LEN])
{
    if (!ctx || !out || pcr_index >= UIOX_KS_PCR_MAX) return;
    ms_memcpy(out, ctx->pcr[pcr_index], UIOX_KS_SHA256_LEN);
}

/* =========================================================================
 * Lock PCRs — called once the kernel is fully up
 * No further extends are accepted after this point.
 * ====================================================================== */
void uiox_ks_measure_lock(uiox_ks_measure_ctx_t *ctx)
{
    if (!ctx) return;
    ctx->locked = true;
    uiox_fw_printf("[ksign-measure] PCRs locked — boot measurement complete "
                   "(%u events)\n", ctx->entry_count);
}

/* =========================================================================
 * Attestation quote — SHA-256 over the concatenation of all PCRs
 *   quote = SHA-256(PCR[0] || PCR[1] || ... || PCR[PCR_MAX-1])
 * ====================================================================== */
void uiox_ks_measure_quote(const uiox_ks_measure_ctx_t *ctx,
                             uint8_t                      quote[UIOX_KS_SHA256_LEN])
{
    if (!ctx || !quote) return;

    uiox_ks_sha256_ctx_t hctx;
    uiox_ks_sha256_init(&hctx);
    for (uint32_t i = 0; i < UIOX_KS_PCR_MAX; i++)
        uiox_ks_sha256_update(&hctx, ctx->pcr[i], UIOX_KS_SHA256_LEN);
    uiox_ks_sha256_final(&hctx, quote);
}

/* =========================================================================
 * Print measurement log
 * ====================================================================== */
void uiox_ks_measure_print(const uiox_ks_measure_ctx_t *ctx)
{
    if (!ctx) return;

    static const char *evt_names[] = {
        "FIRMWARE", "KERNEL_CODE", "KERNEL_DATA",
        "CMDLINE", "MODULE", "RUNTIME_CHK", "KEY_LOAD", "SIG_VERIFY"
    };

    uiox_fw_printf("[ksign-measure] PCR log (%u entries, locked=%s):\n",
                   ctx->entry_count, ctx->locked ? "YES" : "NO");

    for (uint32_t i = 0; i < ctx->entry_count; i++) {
        const uiox_ks_log_entry_t *e = &ctx->entries[i];
        const char *etype = (e->event_type < 8u)
                            ? evt_names[e->event_type] : "?";
        uiox_fw_printf("  [%02u] PCR[%u]  %-14s  %s  t=%llums\n",
                       i, e->pcr_index, etype, e->event_name,
                       (unsigned long long)e->timestamp_ms);
    }

    /* Print current PCR values */
    uiox_fw_printf("[ksign-measure] Current PCR values:\n");
    for (uint32_t i = 0; i < UIOX_KS_PCR_MAX; i++) {
        uiox_fw_printf("  PCR[%u]: ", i);
        for (uint32_t j = 0; j < 8u; j++)
            uiox_fw_printf("%02x", ctx->pcr[i][j]);
        uiox_fw_printf("...\n");
    }
}
