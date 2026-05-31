/*
 * uiox_ld_diag.c - UIOX linker diagnostics
 */
#include "../include/uiox_ld_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static const char *level_tag[]    = { "note",    "warning", "error",  "fatal"  };
static const char *level_colour[] = { "\033[36m", "\033[33m","\033[31m","\033[35m" };
#define COL_RESET "\033[0m"

void uld_diag_init(uld_diag_ctx_t *ctx, int verbose)
{
    ctx->head        = NULL;
    ctx->tail        = NULL;
    ctx->error_count = 0;
    ctx->warn_count  = 0;
    ctx->verbose     = verbose;
}

void uld_diag_free(uld_diag_ctx_t *ctx)
{
    uld_diag_t *d = ctx->head;
    while (d) { uld_diag_t *nx = d->next; free(d); d = nx; }
    ctx->head = ctx->tail = NULL;
}

void uld_diag_emit(uld_diag_ctx_t *ctx, uld_diag_level_t level,
                    const char *file, int line,
                    const char *fmt, ...)
{
    uld_diag_t *d = (uld_diag_t *)calloc(1, sizeof(*d));
    if (!d) return;
    d->level = level;
    d->line  = line;
    strncpy(d->file, file ? file : "<linker>", ULD_NAME_MAX - 1);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(d->msg, sizeof(d->msg), fmt, ap);
    va_end(ap);
    d->next = NULL;
    if (!ctx->head) ctx->head = d; else ctx->tail->next = d;
    ctx->tail = d;
    if (level >= ULD_ERROR)   ctx->error_count++;
    if (level == ULD_WARN)    ctx->warn_count++;
    /* print immediately for fatal */
    if (level == ULD_FATAL) {
        fprintf(stderr, "%suioxld: fatal:%s %s\n",
                level_colour[level], COL_RESET, d->msg);
    }
}

void uld_diag_print(const uld_diag_ctx_t *ctx)
{
    for (uld_diag_t *d = ctx->head; d; d = d->next) {
        if (d->line > 0)
            fprintf(stderr, "%suioxld:%s %s:%d: %s: %s\n",
                    level_colour[d->level], COL_RESET,
                    d->file, d->line,
                    level_tag[d->level], d->msg);
        else
            fprintf(stderr, "%suioxld:%s %s: %s\n",
                    level_colour[d->level], COL_RESET,
                    level_tag[d->level], d->msg);
    }
    if (ctx->error_count)
        fprintf(stderr, "uioxld: %d error(s), %d warning(s)\n",
                ctx->error_count, ctx->warn_count);
}

int uld_diag_ok(const uld_diag_ctx_t *ctx)
{
    return ctx->error_count == 0;
}
