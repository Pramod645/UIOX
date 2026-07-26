/**
 * @file  uiox_fboot.c
 * @brief UIOX Fast Boot — master orchestration, phase tracking, reporting.
 *
 * Integrates:
 *   02_FwHal        — uiox_fw_printf, hardware timer
 *   12_ksign        — FW_VERIFY phase feeds uiox_ks_boot_entry()
 *   33_PCS          — uiox_fb_defer_run_all() called after shell spawn
 *   40_SCI          — SYS_BOOT_STATUS, SYS_BOOT_SNAP_SAVE, SYS_BOOT_SNAP_CLEAR
 *
 * @date  2026-07-08
 */
#include "../include/uiox_fboot.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void fb_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void fb_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

/* =========================================================================
 * String helpers
 * ====================================================================== */
const char *uiox_fb_err_str(uiox_fb_err_t e)
{
    switch (e) {
    case UIOX_FB_OK:            return "OK";
    case UIOX_FB_ERR_INVAL:     return "INVAL";
    case UIOX_FB_ERR_NOMEM:     return "NOMEM";
    case UIOX_FB_ERR_TIMEOUT:   return "TIMEOUT";
    case UIOX_FB_ERR_BADMAGIC:  return "BADMAGIC";
    case UIOX_FB_ERR_BADVERSION:return "BADVERSION";
    case UIOX_FB_ERR_DEP:       return "DEP";
    case UIOX_FB_ERR_SKIP:      return "SKIP";
    case UIOX_FB_ERR_ALREADY:   return "ALREADY";
    case UIOX_FB_ERR_IO:        return "IO";
    case UIOX_FB_ERR_OVERFLOW:  return "OVERFLOW";
    default:                    return "?";
    }
}

const char *uiox_fb_phase_str(uiox_fb_phase_t p)
{
    static const char *names[UIOX_FB_PHASE__COUNT] = {
        "RESET",        "CLK_PLL",     "DDR_INIT",
        "FW_VERIFY",    "DECOMPRESS",  "DEVTREE",
        "EARLY_DRIVERS","FS_MOUNT",    "SUSPEND_RESUME",
        "INIT_SPAWN",   "SHELL_READY",
    };
    return (p < UIOX_FB_PHASE__COUNT) ? names[p] : "?";
}

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_fb_err_t uiox_fb_init(uiox_fb_master_ctx_t *ctx,
                             uiox_fb_mode_t        mode,
                             uint64_t              target_us)
{
    if (!ctx) return UIOX_FB_ERR_INVAL;
    fb_memset(ctx, 0, sizeof(*ctx));

    /* Timer — must come first */
    uiox_fb_err_t rc = uiox_fb_timer_init(&ctx->timer);
    if (rc != UIOX_FB_OK) return rc;

    /* Timing context */
    ctx->timing.magic          = UIOX_FB_MAGIC;
    ctx->timing.version        = UIOX_FB_VERSION;
    ctx->timing.mode           = mode;
    ctx->timing.target_us      = (target_us > 0u) ? target_us : 3000000u;
    ctx->timing.boot_start_us  = uiox_fb_timer_now_us(&ctx->timer);

    /* Deferred init registry */
    rc = uiox_fb_defer_init(&ctx->defer);
    if (rc != UIOX_FB_OK) return rc;

    /* Label each phase record */
    for (uint32_t i = 0; i < UIOX_FB_MAX_PHASES; i++) {
        ctx->timing.phases[i].phase = (uiox_fb_phase_t)i;
        const char *s = uiox_fb_phase_str((uiox_fb_phase_t)i);
        /* strncpy without libc */
        for (uint32_t j = 0; j < 31u && s[j]; j++)
            ctx->timing.phases[i].label[j] = s[j];
    }

    ctx->initialized = true;
    uiox_fw_printf("[fboot] init: mode=%d  target=%llu µs\n",
                   mode, (unsigned long long)ctx->timing.target_us);
    return UIOX_FB_OK;
}

/* =========================================================================
 * Phase begin — record start timestamp
 * ====================================================================== */
uiox_fb_err_t uiox_fb_phase_begin(uiox_fb_master_ctx_t *ctx,
                                    uiox_fb_phase_t       phase)
{
    if (!ctx || !ctx->initialized)    return UIOX_FB_ERR_INVAL;
    if (phase >= UIOX_FB_PHASE__COUNT) return UIOX_FB_ERR_INVAL;

    uiox_fb_phase_record_t *r = &ctx->timing.phases[phase];
    if (r->completed)  return UIOX_FB_ERR_ALREADY;

    r->start_us = uiox_fb_timer_now_us(&ctx->timer);
    uiox_fw_printf("[fboot] %-16s BEGIN  t=%llu µs\n",
                   r->label,
                   (unsigned long long)r->start_us);
    return UIOX_FB_OK;
}

/* =========================================================================
 * Phase end — record duration and budget check
 * ====================================================================== */
uiox_fb_err_t uiox_fb_phase_end(uiox_fb_master_ctx_t *ctx,
                                  uiox_fb_phase_t       phase,
                                  uiox_fb_err_t         result)
{
    if (!ctx || !ctx->initialized)    return UIOX_FB_ERR_INVAL;
    if (phase >= UIOX_FB_PHASE__COUNT) return UIOX_FB_ERR_INVAL;

    uiox_fb_phase_record_t *r = &ctx->timing.phases[phase];
    r->end_us     = uiox_fb_timer_now_us(&ctx->timer);
    r->duration_us = (r->end_us > r->start_us)
                     ? (r->end_us - r->start_us) : 0u;
    r->result     = result;
    r->completed  = true;
    ctx->timing.phases_done++;

    uiox_fw_printf("[fboot] %-16s END    t=%llu µs  dur=%llu µs  %s\n",
                   r->label,
                   (unsigned long long)r->end_us,
                   (unsigned long long)r->duration_us,
                   uiox_fb_err_str(result));

    return UIOX_FB_OK;
}

/* =========================================================================
 * Phase skip (e.g. DDR_INIT on snapshot resume)
 * ====================================================================== */
uiox_fb_err_t uiox_fb_phase_skip(uiox_fb_master_ctx_t *ctx,
                                   uiox_fb_phase_t       phase)
{
    if (!ctx || !ctx->initialized)    return UIOX_FB_ERR_INVAL;
    if (phase >= UIOX_FB_PHASE__COUNT) return UIOX_FB_ERR_INVAL;

    uiox_fb_phase_record_t *r = &ctx->timing.phases[phase];
    r->skipped   = true;
    r->completed = true;
    r->result    = UIOX_FB_ERR_SKIP;
    ctx->timing.phases_done++;

    uiox_fw_printf("[fboot] %-16s SKIP\n", r->label);
    return UIOX_FB_OK;
}

/* =========================================================================
 * Shell ready — record final milestone
 * ====================================================================== */
uiox_fb_err_t uiox_fb_shell_ready(uiox_fb_master_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_FB_ERR_INVAL;

    uiox_fb_phase_begin(ctx, UIOX_FB_PHASE_SHELL_READY);
    uiox_fb_phase_end  (ctx, UIOX_FB_PHASE_SHELL_READY, UIOX_FB_OK);

    ctx->timing.shell_ready_us = uiox_fb_timer_now_us(&ctx->timer);

    if (ctx->timing.shell_ready_us > ctx->timing.target_us) {
        ctx->timing.budget_exceeded = true;
        uiox_fw_printf("[fboot] WARNING: target %llu µs exceeded "
                       "(actual %llu µs, delta +%llu µs)\n",
                       (unsigned long long)ctx->timing.target_us,
                       (unsigned long long)ctx->timing.shell_ready_us,
                       (unsigned long long)(ctx->timing.shell_ready_us
                                            - ctx->timing.target_us));
    } else {
        uiox_fw_printf("[fboot] Boot target MET: %llu µs "
                       "(budget %llu µs, margin %llu µs)\n",
                       (unsigned long long)ctx->timing.shell_ready_us,
                       (unsigned long long)ctx->timing.target_us,
                       (unsigned long long)(ctx->timing.target_us
                                            - ctx->timing.shell_ready_us));
    }

    return UIOX_FB_OK;
}

/* =========================================================================
 * Full boot timing report
 * ====================================================================== */
void uiox_fb_report(const uiox_fb_master_ctx_t *ctx)
{
    if (!ctx) return;
    const uiox_fb_ctx_t *t = &ctx->timing;

    static const char *mode_str[] = {
        "COLD", "RESUME", "SNAPSHOT", "WARMRESET"
    };

    uiox_fw_printf("\n[fboot] ══ Boot Timing Report ══════════════════\n");
    uiox_fw_printf("  Mode          : %s\n",
                   t->mode < 4u ? mode_str[t->mode] : "?");
    uiox_fw_printf("  Shell ready   : %llu µs  (%.1f ms)\n",
                   (unsigned long long)t->shell_ready_us,
                   (float)t->shell_ready_us / 1000.0f);
    uiox_fw_printf("  Target        : %llu µs  (%.1f ms)\n",
                   (unsigned long long)t->target_us,
                   (float)t->target_us / 1000.0f);
    uiox_fw_printf("  Budget        : %s\n",
                   t->budget_exceeded ? "EXCEEDED" : "OK");
    uiox_fw_printf("\n");
    uiox_fw_printf("  %-18s  %8s  %8s  %8s  %s\n",
                   "PHASE", "START µs", "DUR µs", "CUM µs", "STATUS");
    uiox_fw_printf("  %-18s  %8s  %8s  %8s  %s\n",
                   "──────────────────",
                   "────────","────────","────────","──────");

    for (uint32_t i = 0; i < UIOX_FB_MAX_PHASES; i++) {
        const uiox_fb_phase_record_t *r = &t->phases[i];
        const char *status = r->skipped   ? "SKIP"
                           : !r->completed ? "----"
                           : (r->result == UIOX_FB_OK) ? "OK" : "ERR";
        uiox_fw_printf("  %-18s  %8llu  %8llu  %8llu  %s\n",
                       r->label,
                       (unsigned long long)r->start_us,
                       (unsigned long long)r->duration_us,
                       (unsigned long long)r->end_us,
                       status);
    }
    uiox_fw_printf("[fboot] ════════════════════════════════════════\n\n");
}

/* =========================================================================
 * Fill status buffer for sys_boot_status() syscall
 * ====================================================================== */
void uiox_fb_fill_status(const uiox_fb_master_ctx_t *ctx,
                          uint8_t *buf, size_t buf_size)
{
    if (!ctx || !buf || buf_size < sizeof(uiox_fb_ctx_t)) return;
    fb_memcpy(buf, &ctx->timing, sizeof(uiox_fb_ctx_t));
}

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */

/* Global context pointer — set during boot by uiox_fb_init() */
static uiox_fb_master_ctx_t *g_fb_ctx = NULL;

void uiox_fb_set_global_ctx(uiox_fb_master_ctx_t *ctx) { g_fb_ctx = ctx; }

long sys_boot_status(long buf, long buf_size, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!g_fb_ctx || !buf || buf_size < (long)sizeof(uiox_fb_ctx_t))
        return (long)UIOX_FB_ERR_INVAL;
    /* Production: copy_to_user() */
    uiox_fb_fill_status(g_fb_ctx, (uint8_t *)(uintptr_t)buf,
                         (size_t)buf_size);
    return (long)UIOX_FB_OK;
}

long sys_boot_snap_save(long kver, long a1, long a2, long a3)
{
    (void)a1; (void)a2; (void)a3;
    if (!g_fb_ctx) return (long)UIOX_FB_ERR_INVAL;
    return (long)uiox_fb_snap_capture(&g_fb_ctx->snap, (uint32_t)kver);
}

long sys_boot_snap_clear(long a0, long a1, long a2, long a3)
{
    (void)a0; (void)a1; (void)a2; (void)a3;
    if (!g_fb_ctx) return (long)UIOX_FB_ERR_INVAL;
    return (long)uiox_fb_snap_invalidate(&g_fb_ctx->snap);
}
