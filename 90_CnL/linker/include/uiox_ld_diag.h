#ifndef UIOX_LD_DIAG_H
#define UIOX_LD_DIAG_H
/*
 * uiox_ld_diag.h - UIOX linker diagnostics
 */
#include "uiox_ld_types.h"

typedef enum uld_diag_level {
    ULD_NOTE    = 0,
    ULD_WARN    = 1,
    ULD_ERROR   = 2,
    ULD_FATAL   = 3,
} uld_diag_level_t;

typedef struct uld_diag {
    uld_diag_level_t  level;
    char              file[ULD_NAME_MAX];
    int               line;
    char              msg[512];
    struct uld_diag  *next;
} uld_diag_t;

typedef struct uld_diag_ctx {
    uld_diag_t  *head;
    uld_diag_t  *tail;
    int          error_count;
    int          warn_count;
    int          verbose;
} uld_diag_ctx_t;

void uld_diag_init    (uld_diag_ctx_t *ctx, int verbose);
void uld_diag_free    (uld_diag_ctx_t *ctx);
void uld_diag_emit    (uld_diag_ctx_t *ctx, uld_diag_level_t level,
                        const char *file, int line,
                        const char *fmt, ...);
void uld_diag_print   (const uld_diag_ctx_t *ctx);
int  uld_diag_ok      (const uld_diag_ctx_t *ctx);

#define ULD_NOTE(ctx,f,l,...)  uld_diag_emit(ctx,ULD_NOTE, f,l,__VA_ARGS__)
#define ULD_WARN(ctx,f,l,...)  uld_diag_emit(ctx,ULD_WARN, f,l,__VA_ARGS__)
#define ULD_ERR(ctx,f,l,...)   uld_diag_emit(ctx,ULD_ERROR,f,l,__VA_ARGS__)
#define ULD_FATAL(ctx,f,l,...) uld_diag_emit(ctx,ULD_FATAL,f,l,__VA_ARGS__)

#endif /* UIOX_LD_DIAG_H */
