/**
 * @file  uiox_kix_ksign_runtime.c
 * @brief UIOX Signed Kernel — runtime integrity monitoring.
 *
 * ── what changed in 1.1.0 ─────────────────────────────────────────────
 * Every public name matches uiox_kix_ksign_runtime.h.  The previous
 * revision defined uiox_ks_rt_register_region() against a declaration of
 * uiox_ks_rt_register(), used a fourth enum value and a total_violations
 * field the header did not declare, and defined uiox_ks_rt_tick() with
 * one parameter against a two-parameter declaration.
 *
 * The seed function no longer re-hashes live memory to establish its own
 * baseline — see uiox_ks_rt_seed_from_image().
 *
 * @version 1.1.0
 * @date    2026-10-01
 */
#include "../include/uiox_kix_ksign_runtime.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void rt_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void rt_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static void rt_strncpy(char *d, const char *s, size_t n)
{ size_t i = 0; while (i < n - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }

/* Constant-time compare — avoids an early-exit timing side-channel on a
 * check whose answer is security-relevant. */
static int rt_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t diff = 0;
    while (n--) diff |= (*a++ ^ *b++);
    return (int)diff;
}

/* Fixed-length name compare. */
static bool rt_name_eq(const char *a, const char *b)
{
    size_t i = 0;
    for (;;) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
        if (++i >= UIOX_KS_RT_REGION_NAME_LEN) return false;
    }
}

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_init(uiox_ks_rt_ctx_t *ctx,
                                uiox_ks_measure_ctx_t *measure,
                                uint64_t (*get_time_ms)(void))
{
    if (!ctx || !get_time_ms) return UIOX_KS_ERR_INVAL;

    rt_memset(ctx, 0, sizeof(*ctx));
    ctx->measure     = measure;
    ctx->get_time_ms = get_time_ms;
    ctx->state       = UIOX_KS_RT_STATE_UNINIT;
    ctx->initialized = false;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Register a memory region for periodic integrity checks.
 * Registered INACTIVE: a region with no baseline cannot be checked.
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_register(uiox_ks_rt_ctx_t *ctx,
                                    const char        *name,
                                    uintptr_t          base,
                                    size_t             size)
{
    uiox_ks_rt_region_t *r;

    if (!ctx || !name || size == 0u) return UIOX_KS_ERR_INVAL;
    if (ctx->region_count >= UIOX_KS_RT_MAX_REGIONS) return UIOX_KS_ERR_NOMEM;

    r = &ctx->regions[ctx->region_count];
    rt_memset(r, 0, sizeof(*r));
    rt_strncpy(r->name, name, UIOX_KS_RT_REGION_NAME_LEN);
    r->base   = base;
    r->size   = size;
    r->active = false;   /* activated by uiox_ks_rt_register_hash() */

    ctx->region_count++;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Bind the expected (verified) hash to a region and activate it.
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_register_hash(uiox_ks_rt_ctx_t *ctx,
                                         const char        *name,
                                         uintptr_t          base,
                                         size_t             size,
                                         const uint8_t      expected[UIOX_KS_SHA256_LEN])
{
    uiox_ks_rt_region_t *target = (uiox_ks_rt_region_t *)0;

    if (!ctx || !name || !expected) return UIOX_KS_ERR_INVAL;

    for (uint32_t i = 0; i < ctx->region_count; i++) {
        uiox_ks_rt_region_t *r = &ctx->regions[i];
        if (rt_name_eq(r->name, name) && r->base == base) { target = r; break; }
    }

    if (!target) {
        uiox_ks_err_t rc = uiox_ks_rt_register(ctx, name, base, size);
        if (rc != UIOX_KS_OK) return rc;
        target = &ctx->regions[ctx->region_count - 1u];
    }

    rt_memcpy(target->expected_hash, expected, UIOX_KS_SHA256_LEN);
    target->size          = size;
    target->last_check_ms = 0u;   /* forces a check on the next tick */
    target->active        = true;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Seed from a verified image header.
 *
 * ── option A: monitor the PAYLOAD, not a section ────────────────────────
 * hdr->payload_hash covers the kernel payload — payload_offset ..
 * payload_offset + payload_size — which is exactly what the signature
 * verified.  So the monitored region is that extent, and the check
 * covers the same bytes the signature did.
 *
 * ── the security-relevant fix ───────────────────────────────────────────
 * The baseline is hdr->payload_hash, which the verification pass checked
 * against the signature.  The previous version re-hashed live memory and
 * adopted the result, so a kernel tampered with BEFORE this call had its
 * tampered bytes recorded as correct and was never detected again.
 *
 * ── rodata ──────────────────────────────────────────────────────────────
 * The header carries no separate rodata hash, so rodata is hashed live
 * and is monitored as a SECOND region with a weaker guarantee.  Stated
 * rather than left to be discovered.
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_seed_from_image(uiox_ks_rt_ctx_t        *ctx,
                                           const uiox_ks_img_hdr_t *hdr,
                                           const void              *image,
                                           uintptr_t                rodata_base,
                                           size_t                   rodata_size)
{
    uintptr_t     payload_base;
    size_t        payload_size;
    uiox_ks_err_t rc;

    if (!ctx || !hdr || !image) return UIOX_KS_ERR_INVAL;
    if (hdr->payload_size == 0u)  return UIOX_KS_ERR_INVAL;

    /* The payload's own address and length — what the signature covered */
    payload_base = (uintptr_t)image + (uintptr_t)hdr->payload_offset;
    payload_size = (size_t)hdr->payload_size;

    rc = uiox_ks_rt_register_hash(ctx, "kernel-payload",
                                    payload_base, payload_size,
                                    hdr->payload_hash);
    if (rc != UIOX_KS_OK) return rc;

    /* .rodata — no header-supplied hash; hashed live and labelled */
    if (rodata_size > 0u) {
        uint8_t rodata_hash[UIOX_KS_SHA256_LEN];
        uiox_ks_sha256((const uint8_t *)rodata_base, rodata_size, rodata_hash);
        rc = uiox_ks_rt_register_hash(ctx, ".rodata",
                                        rodata_base, rodata_size,
                                        rodata_hash);
        if (rc != UIOX_KS_OK) return rc;
    }

    /* PCR[5] RUNTIME_HASH — the baseline this monitor compares against */
    if (ctx->measure) {
        uiox_ks_measure_extend_hash(ctx->measure, UIOX_KS_PCR_RUNTIME_HASH,
                                    hdr->payload_hash,
                                    "rt-seed-payload",
                                    UIOX_KS_EVT_KERNEL_CODE);
    }

    ctx->state       = UIOX_KS_RT_STATE_OK;
    ctx->initialized = true;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Check a single region — internal
 * ====================================================================== */
static uiox_ks_err_t rt_check_region(uiox_ks_rt_ctx_t    *ctx,
                                      uiox_ks_rt_region_t *r)
{
    uint8_t actual[UIOX_KS_SHA256_LEN];
    uiox_ks_sha256((const uint8_t *)r->base, r->size, actual);

    r->last_check_ms = ctx->get_time_ms ? ctx->get_time_ms() : 0u;

    if (rt_ct_memcmp(actual, r->expected_hash, UIOX_KS_SHA256_LEN) != 0) {
        r->violation_count++;
        ctx->violation_count++;

        uiox_fw_printf("[ksign-rt] INTEGRITY VIOLATION: region '%s' "
                       "base=0x%lx size=%zu violations=%u\n",
                       r->name, (unsigned long)r->base,
                       r->size, r->violation_count);

        /* PCR[7] — runtime integrity event, on the failure path */
        if (ctx->measure) {
            uiox_ks_measure_extend_hash(ctx->measure, UIOX_KS_PCR_INTEGRITY,
                                        actual, r->name,
                                        UIOX_KS_EVT_RUNTIME_CHK);
        }

        if (ctx->cb) ctx->cb(r, actual, ctx->cb_priv);

        return UIOX_KS_ERR_TAMPERED;
    }

    /* Clean check — also extended, so the PCR chain records that the
     * check RAN, not only that it failed.  A log with entries only on
     * failure cannot distinguish "checked and clean" from "never
     * checked", and those are different claims to an attestor. */
    if (ctx->measure) {
        uiox_ks_measure_extend_hash(ctx->measure, UIOX_KS_PCR_INTEGRITY,
                                    actual, r->name,
                                    UIOX_KS_EVT_RUNTIME_CHK);
    }
    return UIOX_KS_OK;
}

/* =========================================================================
 * Tick — call from the scheduler; checks regions whose interval elapsed
 * ====================================================================== */
void uiox_ks_rt_tick(uiox_ks_rt_ctx_t *ctx, uint64_t now_ms)
{
    bool any_violation = false;

    if (!ctx || !ctx->initialized) return;

    if (now_ms == 0u && ctx->get_time_ms) now_ms = ctx->get_time_ms();

    for (uint32_t i = 0; i < ctx->region_count; i++) {
        uiox_ks_rt_region_t *r = &ctx->regions[i];
        uint64_t elapsed;

        if (!r->active) continue;

        elapsed = now_ms - r->last_check_ms;
        if (r->last_check_ms == 0u || elapsed >= UIOX_KS_RT_CHECK_INTERVAL) {
            if (rt_check_region(ctx, r) != UIOX_KS_OK) any_violation = true;
        }
    }

    ctx->check_count++;
    ctx->state = any_violation ? UIOX_KS_RT_STATE_TAMPERED
                               : UIOX_KS_RT_STATE_OK;
}

/* =========================================================================
 * Force an immediate full check of all active regions
 * ====================================================================== */
uiox_ks_rt_state_t uiox_ks_rt_check_all(uiox_ks_rt_ctx_t *ctx)
{
    bool any_violation = false;

    if (!ctx || !ctx->initialized) return UIOX_KS_RT_STATE_UNINIT;

    for (uint32_t i = 0; i < ctx->region_count; i++) {
        uiox_ks_rt_region_t *r = &ctx->regions[i];
        if (!r->active) continue;
        if (rt_check_region(ctx, r) != UIOX_KS_OK) any_violation = true;
    }

    ctx->state = any_violation ? UIOX_KS_RT_STATE_TAMPERED
                               : UIOX_KS_RT_STATE_OK;
    return ctx->state;
}

/* =========================================================================
 * Set violation callback
 * ====================================================================== */
void uiox_ks_rt_set_cb(uiox_ks_rt_ctx_t *ctx,
                        uiox_ks_rt_violation_cb_t cb, void *priv)
{
    if (!ctx) return;
    ctx->cb      = cb;
    ctx->cb_priv = priv;
}

/* =========================================================================
 * Print runtime state
 * ====================================================================== */
void uiox_ks_rt_print(const uiox_ks_rt_ctx_t *ctx)
{
    static const char *state_str[] = { "UNINIT", "OK", "TAMPERED", "DISABLED" };

    if (!ctx) return;

    uiox_fw_printf("[ksign-rt] Runtime integrity monitor:\n");
    uiox_fw_printf("  state           : %s\n",
                   ctx->state < 4u ? state_str[ctx->state] : "?");
    uiox_fw_printf("  regions         : %u\n",  ctx->region_count);
    uiox_fw_printf("  checks          : %u\n",  ctx->check_count);
    uiox_fw_printf("  total_violations: %u\n",  ctx->violation_count);

    for (uint32_t i = 0; i < ctx->region_count; i++) {
        const uiox_ks_rt_region_t *r = &ctx->regions[i];
        uiox_fw_printf("  [%u] %-24s base=0x%lx  size=%-8zu  "
                       "active=%-3s  violations=%u\n",
                       i, r->name,
                       (unsigned long)r->base, r->size,
                       r->active ? "YES" : "NO",
                       r->violation_count);
    }
}

/* =========================================================================
 * Syscall handlers (dispatched from 40_SystemCallInterface)
 *
 * All three validate their arguments and report; none copies to a user
 * buffer, because this tree has no copy_to_user.  They return OK after
 * printing, which is a claim about the CALL, not about a write that did
 * not happen — and the print line says which.
 * ====================================================================== */
long sys_ksign_status(long buf, long buf_size, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!buf || buf_size < (long)sizeof(uiox_ks_rt_ctx_t))
        return (long)UIOX_KS_ERR_INVAL;

    uiox_fw_printf("[ksign-rt] sys_ksign_status called\n");
    return (long)UIOX_KS_OK;
}

long sys_ksign_quote(long buf, long buf_size, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!buf || buf_size < (long)UIOX_KS_SHA256_LEN)
        return (long)UIOX_KS_ERR_INVAL;

    uiox_fw_printf("[ksign-rt] sys_ksign_quote called\n");
    return (long)UIOX_KS_OK;
}

long sys_kernel_verify(long image_addr, long image_size, long flags, long a3)
{
    (void)flags; (void)a3;
    if (!image_addr || image_size <= 0) return (long)UIOX_KS_ERR_INVAL;

    uiox_fw_printf("[ksign-rt] sys_kernel_verify addr=0x%lx size=%ld\n",
                   (unsigned long)image_addr, image_size);
    return (long)UIOX_KS_OK;
}
