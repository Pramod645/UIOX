#ifndef UIOX_ERROR_H
#define UIOX_ERROR_H
/*
 * uiox_error.h - UIOX compiler diagnostic system
 */

typedef enum uiox_diag_level {
    UIOX_NOTE    = 0,
    UIOX_WARNING = 1,
    UIOX_ERROR   = 2,
    UIOX_FATAL   = 3,
} uiox_diag_level_t;

typedef struct uiox_diag {
    uiox_diag_level_t level;
    const char       *file;
    int               line;
    int               col;
    char              msg[512];
    struct uiox_diag *next;
} uiox_diag_t;

typedef struct uiox_diag_ctx {
    uiox_diag_t *head;
    uiox_diag_t *tail;
    int          error_count;
    int          warning_count;
} uiox_diag_ctx_t;

void uiox_diag_init   (uiox_diag_ctx_t *ctx);
void uiox_diag_emit   (uiox_diag_ctx_t *ctx, uiox_diag_level_t level,
                        const char *file, int line, int col,
                        const char *fmt, ...);
void uiox_diag_print  (const uiox_diag_ctx_t *ctx);
void uiox_diag_free   (uiox_diag_ctx_t *ctx);
int  uiox_diag_has_error(const uiox_diag_ctx_t *ctx);

#define UIOX_NOTE(ctx, f, l, c, ...)    \
    uiox_diag_emit(ctx, UIOX_NOTE,    f, l, c, __VA_ARGS__)
#define UIOX_WARN(ctx, f, l, c, ...)    \
    uiox_diag_emit(ctx, UIOX_WARNING, f, l, c, __VA_ARGS__)
#define UIOX_ERR(ctx, f, l, c, ...)     \
    uiox_diag_emit(ctx, UIOX_ERROR,   f, l, c, __VA_ARGS__)
#define UIOX_FATAL(ctx, f, l, c, ...)   \
    uiox_diag_emit(ctx, UIOX_FATAL,   f, l, c, __VA_ARGS__)

#endif /* UIOX_ERROR_H */
