/**
 * @file  uiox_fboot_defer.c
 * @brief UIOX Fast Boot — deferred / lazy driver init registry.
 * @date  2026-07-08
 */
#include "../include/uiox_fboot_defer.h"

extern void uiox_fw_printf(const char *fmt, ...);

static void df_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void df_strncpy(char *d, const char *s, size_t n)
{ size_t i = 0; while (i < n - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_fb_err_t uiox_fb_defer_init(uiox_fb_defer_ctx_t *ctx)
{
    if (!ctx) return UIOX_FB_ERR_INVAL;
    df_memset(ctx, 0, sizeof(*ctx));
    return UIOX_FB_OK;
}

/* =========================================================================
 * Register a deferred initialiser
 * ====================================================================== */
uiox_fb_err_t uiox_fb_defer_register(uiox_fb_defer_ctx_t *ctx,
                                       const char          *name,
                                       uiox_fb_init_fn_t    fn,
                                       void                *arg,
                                       uint32_t             priority)
{
    if (!ctx || !fn || !name)              return UIOX_FB_ERR_INVAL;
    if (ctx->count >= UIOX_FB_MAX_DEFERRED) return UIOX_FB_ERR_NOMEM;

    uiox_fb_deferred_t *d = &ctx->queue[ctx->count];
    df_memset(d, 0, sizeof(*d));
    df_strncpy(d->name, name, UIOX_FB_DEFER_NAME_LEN);
    d->fn         = fn;
    d->arg        = arg;
    d->priority   = priority;
    d->registered = true;
    ctx->count++;
    return UIOX_FB_OK;
}

/* =========================================================================
 * Insertion-sort by priority (stable, small N — no malloc needed)
 * ====================================================================== */
static void df_sort(uiox_fb_defer_ctx_t *ctx)
{
    for (uint32_t i = 1; i < ctx->count; i++) {
        uiox_fb_deferred_t tmp = ctx->queue[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && ctx->queue[j].priority > tmp.priority) {
            ctx->queue[j + 1] = ctx->queue[j];
            j--;
        }
        ctx->queue[j + 1] = tmp;
    }
}

/* =========================================================================
 * Run all registered deferred inits in priority order
 * ====================================================================== */
uiox_fb_err_t uiox_fb_defer_run_all(uiox_fb_defer_ctx_t   *ctx,
                                      const uiox_fb_timer_t *timer)
{
    if (!ctx) return UIOX_FB_ERR_INVAL;
    if (ctx->running) return UIOX_FB_ERR_ALREADY;

    ctx->running = true;
    df_sort(ctx);

    uiox_fw_printf("[fboot-defer] Running %u deferred initialisers...\n",
                   ctx->count);

    for (uint32_t i = 0; i < ctx->count; i++) {
        uiox_fb_deferred_t *d = &ctx->queue[i];
        if (!d->registered || d->completed) continue;

        uint64_t t0 = timer ? uiox_fb_timer_now_us(timer) : 0u;
        d->result   = d->fn(d->arg);
        uint64_t t1 = timer ? uiox_fb_timer_now_us(timer) : 0u;

        d->duration_us = (t1 > t0) ? (t1 - t0) : 0u;
        d->completed   = true;

        if (d->result == UIOX_FB_OK) {
            ctx->completed++;
            uiox_fw_printf("[fboot-defer]  ✓ %-48s  %llu µs\n",
                           d->name,
                           (unsigned long long)d->duration_us);
        } else {
            ctx->failed++;
            uiox_fw_printf("[fboot-defer]  ✗ %-48s  err=%d\n",
                           d->name, d->result);
        }
    }

    uiox_fw_printf("[fboot-defer] Done: %u OK, %u failed.\n",
                   ctx->completed, ctx->failed);
    return (ctx->failed == 0u) ? UIOX_FB_OK : UIOX_FB_ERR_IO;
}

/* =========================================================================
 * Query
 * ====================================================================== */
bool uiox_fb_defer_all_done(const uiox_fb_defer_ctx_t *ctx)
{
    if (!ctx) return false;
    return ctx->completed + ctx->failed == ctx->count;
}

/* =========================================================================
 * Print status table
 * ====================================================================== */
void uiox_fb_defer_print(const uiox_fb_defer_ctx_t *ctx)
{
    if (!ctx) return;
    uiox_fw_printf("[fboot-defer] Deferred init table (%u entries):\n",
                   ctx->count);
    uiox_fw_printf("  %-3s  %-48s  %-4s  %-10s  %s\n",
                   "PRI", "NAME", "DONE", "DURATION", "RESULT");
    for (uint32_t i = 0; i < ctx->count; i++) {
        const uiox_fb_deferred_t *d = &ctx->queue[i];
        uiox_fw_printf("  %-3u  %-48s  %-4s  %-8llu µs  %d\n",
                       d->priority, d->name,
                       d->completed ? "YES" : "NO",
                       (unsigned long long)d->duration_us,
                       d->result);
    }
}
