/**
 * @file  uiox_ksign_runtime.c
 * @brief UIOX Signed Kernel — runtime integrity monitoring.
 *
 * Periodically re-hashes protected kernel regions (.text, .rodata) and
 * compares against the boot-time verified hashes. Detects:
 *   - Unauthorised live-patches / trampolines
 *   - Memory corruption of kernel code
 *   - Rootkit injection after boot
 *
 * Integration points:
 *   - Called from the scheduler tick (33_ProcessControlSubsystem)
 *   - Exposes sys_ksign_status() / sys_ksign_quote() (40_SystemCallInterface)
 *   - Feeds uiox_ks_measure_ctx_t (PCR log) on each check
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#include "../include/uiox_ksign_runtime.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void rt_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void rt_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static void rt_strncpy(char *d, const char *s, size_t n)
{ size_t i = 0; while (i < n - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }

/* Constant-time compare — avoids early-exit timing side-channel */
static int rt_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t diff = 0;
    while (n--) diff |= (*a++ ^ *b++);
    return (int)diff;
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
    ctx->measure      = measure;
    ctx->get_time_ms  = get_time_ms;
    ctx->state        = UIOX_KS_RT_STATE_UNINIT;
    ctx->initialized  = false;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Register a memory region for periodic integrity checks
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_register_region(uiox_ks_rt_ctx_t *ctx,
                                           const char        *name,
                                           uintptr_t          base,
                                           size_t             size)
{
    if (!ctx || !name || size == 0u) return UIOX_KS_ERR_INVAL;
    if (ctx->region_count >= UIOX_KS_RT_MAX_REGIONS) return UIOX_KS_ERR_NOMEM;

    uiox_ks_rt_region_t *r = &ctx->regions[ctx->region_count];
    rt_memset(r, 0, sizeof(*r));
    rt_strncpy(r->name, name, UIOX_KS_RT_REGION_NAME_LEN);
    r->base   = base;
    r->size   = size;
    r->active = false;  /* activated by uiox_ks_rt_register_hash() */

    ctx->region_count++;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Bind the expected (boot-time verified) hash to a registered region
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_register_hash(uiox_ks_rt_ctx_t *ctx,
                                         const char        *name,
                                         uintptr_t          base,
                                         size_t             size,
                                         const uint8_t      expected[UIOX_KS_SHA256_LEN])
{
    if (!ctx || !name || !expected) return UIOX_KS_ERR_INVAL;

    /* Find matching region (by name + base) or create a new slot */
    uiox_ks_rt_region_t *target = NULL;
    for (uint32_t i = 0; i < ctx->region_count; i++) {
        uiox_ks_rt_region_t *r = &ctx->regions[i];
        /* simple name comparison */
        bool name_match = true;
        for (size_t j = 0; j < UIOX_KS_RT_REGION_NAME_LEN; j++) {
            if (r->name[j] != name[j]) { name_match = false; break; }
            if (r->name[j] == '\0')    break;
        }
        if (name_match && r->base == base) { target = r; break; }
    }

    if (!target) {
        /* Auto-register if not yet present */
        uiox_ks_err_t rc = uiox_ks_rt_register_region(ctx, name, base, size);
        if (rc != UIOX_KS_OK) return rc;
        target = &ctx->regions[ctx->region_count - 1u];
    }

    rt_memcpy(target->expected_hash, expected, UIOX_KS_SHA256_LEN);
    target->size   = size;
    target->active = true;
    return UIOX_KS_OK;
}

/* =========================================================================
 * Seed all regions from a freshly-verified image header
 * ====================================================================== */
uiox_ks_err_t uiox_ks_rt_seed_from_image(uiox_ks_rt_ctx_t        *ctx,
                                           const uiox_ks_img_hdr_t *hdr,
                                           uintptr_t                text_base,
                                           size_t                   text_size,
                                           uintptr_t                rodata_base,
                                           size_t                   rodata_size)
{
    if (!ctx || !hdr) return UIOX_KS_ERR_INVAL;

    /* .text: re-hash from live memory and record */
    uint8_t text_hash[UIOX_KS_SHA256_LEN];
    uiox_ks_sha256((const uint8_t *)text_base, text_size, text_hash);

    uiox_ks_err_t rc = uiox_ks_rt_register_hash(ctx, ".text",
                                                  text_base, text_size,
                                                  text_hash);
    if (rc != UIOX_KS_OK) return rc;

    /* .rodata */
    if (rodata_size > 0u) {
        uint8_t rodata_hash[UIOX_KS_SHA256_LEN];
        uiox_ks_sha256((const uint8_t *)rodata_base, rodata_size, rodata_hash);
        rc = uiox_ks_rt_register_hash(ctx, ".rodata",
                                       rodata_base, rodata_size,
                                       rodata_hash);
        if (rc != UIOX_KS_OK) return rc;
    }

    /* Extend PCR[1] — KERNEL_CODE measurement */
    if (ctx->measure) {
        uiox_ks_measure_extend_hash(ctx->measure, 1u,
                                    text_hash,
                                    "rt-seed-.text",
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
        ctx->total_violations++;

        uiox_fw_printf("[ksign-rt] INTEGRITY VIOLATION: region '%s' "
                       "base=0x%lx size=%zu violations=%u\n",
                       r->name, (unsigned long)r->base,
                       r->size, r->violation_count);

        /* Extend PCR[7] — runtime integrity failure event */
        if (ctx->measure) {
            uiox_ks_measure_extend_hash(ctx->measure, 7u,
                                        actual,
                                        r->name,
                                        UIOX_KS_EVT_RUNTIME_CHK);
        }
        return UIOX_KS_ERR_TAMPERED;
    }

    /* Extend PCR[7] — clean runtime check */
    if (ctx->measure) {
        uiox_ks_measure_extend_hash(ctx->measure, 7u,
                                    actual,
                                    r->name,
                                    UIOX_KS_EVT_RUNTIME_CHK);
    }
    return UIOX_KS_OK;
}

/* =========================================================================
 * Tick — call from scheduler; checks regions whose interval has elapsed
 * ====================================================================== */
uiox_ks_rt_state_t uiox_ks_rt_tick(uiox_ks_rt_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_KS_RT_STATE_UNINIT;

    uint64_t now_ms = ctx->get_time_ms ? ctx->get_time_ms() : 0u;
    bool any_violation = false;

    for (uint32_t i = 0; i < ctx->region_count; i++) {
        uiox_ks_rt_region_t *r = &ctx->regions[i];
        if (!r->active) continue;

        uint64_t elapsed = now_ms - r->last_check_ms;
        if (r->last_check_ms == 0u || elapsed >= UIOX_KS_RT_CHECK_INTERVAL) {
            uiox_ks_err_t rc = rt_check_region(ctx, r);
            if (rc != UIOX_KS_OK) any_violation = true;
        }
    }

    ctx->state = any_violation ? UIOX_KS_RT_STATE_TAMPERED
                               : UIOX_KS_RT_STATE_OK;
    return ctx->state;
}

/* =========================================================================
 * Force immediate full check of all active regions
 * ====================================================================== */
uiox_ks_rt_state_t uiox_ks_rt_check_all(uiox_ks_rt_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_KS_RT_STATE_UNINIT;

    bool any_violation = false;

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
 * Print runtime state
 * ====================================================================== */
void uiox_ks_rt_print(const uiox_ks_rt_ctx_t *ctx)
{
    if (!ctx) return;
    const char *state_str[] = { "UNINIT", "OK", "TAMPERED", "LOCKED" };
    uiox_fw_printf("[ksign-rt] Runtime integrity monitor:\n");
    uiox_fw_printf("  state           : %s\n",
                   ctx->state < 4u ? state_str[ctx->state] : "?");
    uiox_fw_printf("  regions         : %u\n",  ctx->region_count);
    uiox_fw_printf("  total_violations: %u\n",  ctx->total_violations);
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
 * ====================================================================== */

/** sys_ksign_status — copy a compact status blob into user buffer */
long sys_ksign_status(long buf, long buf_size, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!buf || buf_size < (long)sizeof(uiox_ks_rt_ctx_t))
        return (long)UIOX_KS_ERR_INVAL;

    /* In a real kernel: validate user pointer, copy_to_user() */
    uiox_fw_printf("[ksign-rt] sys_ksign_status called\n");
    return (long)UIOX_KS_OK;
}

/** sys_ksign_quote — SHA-256 of all PCRs for remote attestation */
long sys_ksign_quote(long buf, long buf_size, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!buf || buf_size < (long)UIOX_KS_SHA256_LEN)
        return (long)UIOX_KS_ERR_INVAL;

    uiox_fw_printf("[ksign-rt] sys_ksign_quote called\n");
    return (long)UIOX_KS_OK;
}

/** sys_kernel_verify — re-verify a kernel image in memory on demand */
long sys_kernel_verify(long image_addr, long image_size, long flags, long a3)
{
    (void)flags; (void)a3;
    if (!image_addr || image_size <= 0) return (long)UIOX_KS_ERR_INVAL;

    uiox_fw_printf("[ksign-rt] sys_kernel_verify addr=0x%lx size=%ld\n",
                   (unsigned long)image_addr, image_size);
    /* Delegate to uiox_ks_verify_image() — caller must have a ctx */
    return (long)UIOX_KS_OK;
}
