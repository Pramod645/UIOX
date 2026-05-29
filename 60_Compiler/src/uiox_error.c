/*
 * uiox_error.c - UIOX compiler diagnostic system
 */
#include "../include/uiox_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static const char *level_str[] = {
    "note", "warning", "error", "fatal"
};
static const char *level_colour[] = {
    "\033[36m", "\033[33m", "\033[31m", "\033[35m"
};
#define COL_RESET "\033[0m"

void uiox_diag_init(uiox_diag_ctx_t *ctx)
{
    ctx->head          = NULL;
    ctx->tail          = NULL;
    ctx->error_count   = 0;
    ctx->warning_count = 0;
}

void uiox_diag_emit(uiox_diag_ctx_t *ctx, uiox_diag_level_t level,
                    const char *file, int line, int col,
                    const char *fmt, ...)
{
    uiox_diag_t *d = (uiox_diag_t *)malloc(sizeof(uiox_diag_t));
    if (!d) return;

    d->level = level;
    d->file  = file;
    d->line  = line;
    d->col   = col;
    d->next  = NULL;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(d->msg, sizeof(d->msg), fmt, ap);
    va_end(ap);

    if (!ctx->head) ctx->head = d;
    else            ctx->tail->next = d;
    ctx->tail = d;

    if (level >= UIOX_ERROR)   ctx->error_count++;
    if (level == UIOX_WARNING) ctx->warning_count++;
}

void uiox_diag_print(const uiox_diag_ctx_t *ctx)
{
    for (uiox_diag_t *d = ctx->head; d; d = d->next) {
        fprintf(stderr, "%s%s:%d:%d: %s:%s %s\n",
                level_colour[d->level],
                d->file ? d->file : "<unknown>",
                d->line, d->col,
                level_str[d->level],
                COL_RESET,
                d->msg);
    }
    if (ctx->error_count > 0)
        fprintf(stderr, "%d error(s), %d warning(s)\n",
                ctx->error_count, ctx->warning_count);
}

void uiox_diag_free(uiox_diag_ctx_t *ctx)
{
    uiox_diag_t *d = ctx->head;
    while (d) {
        uiox_diag_t *next = d->next;
        free(d);
        d = next;
    }
    ctx->head = ctx->tail = NULL;
}

int uiox_diag_has_error(const uiox_diag_ctx_t *ctx)
{
    return ctx->error_count > 0;
}
