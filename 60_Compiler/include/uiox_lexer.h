#ifndef UIOX_LEXER_H
#define UIOX_LEXER_H
/*
 * uiox_lexer.h - UIOX source tokeniser
 */
#include "uiox_token.h"
#include "uiox_error.h"

#define UIOX_LEX_BUF_MAX  (1024 * 1024)   /* 1 MB source limit */

typedef struct uiox_lexer {
    const char      *src;       /* source text (null-terminated)  */
    int              src_len;   /* total source length            */
    int              pos;       /* current byte position          */
    int              line;      /* current line number (1-based)  */
    int              col;       /* current column (1-based)       */
    const char      *filename;
    uiox_diag_ctx_t *diag;
    uiox_token_t     cur;       /* current token (peeked)         */
    uiox_token_t     next;      /* one token lookahead            */
} uiox_lexer_t;

void         uiox_lexer_init    (uiox_lexer_t *lex, const char *src,
                                  int len, const char *filename,
                                  uiox_diag_ctx_t *diag);
uiox_token_t uiox_lexer_next    (uiox_lexer_t *lex);
uiox_token_t uiox_lexer_peek    (uiox_lexer_t *lex);
uiox_token_t uiox_lexer_consume (uiox_lexer_t *lex);
int          uiox_lexer_expect  (uiox_lexer_t *lex, uiox_token_kind_t k);

#endif /* UIOX_LEXER_H */
