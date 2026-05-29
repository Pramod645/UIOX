#ifndef UIOX_TOKEN_H
#define UIOX_TOKEN_H
/*
 * uiox_token.h - UIOX compiler token type definitions
 */

typedef enum uiox_token_kind {
    /* -- Literals ----------------------------------------- */
    TOK_INT_LIT,        /* 42, 0xFF, 0b1010                  */
    TOK_FLOAT_LIT,      /* 3.14                              */
    TOK_STR_LIT,        /* "hello"                           */
    TOK_CHAR_LIT,       /* 'A'                               */
    TOK_IDENT,          /* identifier                        */

    /* -- Keywords ----------------------------------------- */
    TOK_KW_INT,         /* int                               */
    TOK_KW_UINT,        /* uint                              */
    TOK_KW_LONG,        /* long                              */
    TOK_KW_ULONG,       /* ulong                             */
    TOK_KW_SHORT,       /* short                             */
    TOK_KW_CHAR,        /* char                              */
    TOK_KW_FLOAT,       /* float                             */
    TOK_KW_DOUBLE,      /* double                            */
    TOK_KW_VOID,        /* void                              */
    TOK_KW_STRUCT,      /* struct                            */
    TOK_KW_UNION,       /* union                             */
    TOK_KW_ENUM,        /* enum                              */
    TOK_KW_TYPEDEF,     /* typedef                           */
    TOK_KW_CONST,       /* const                             */
    TOK_KW_STATIC,      /* static                            */
    TOK_KW_EXTERN,      /* extern                            */
    TOK_KW_INLINE,      /* inline                            */
    TOK_KW_VOLATILE,    /* volatile                          */
    TOK_KW_REGISTER,    /* register                          */
    TOK_KW_UNSIGNED,    /* unsigned                          */
    TOK_KW_SIGNED,      /* signed                            */
    TOK_KW_AUTO,        /* auto                              */
    TOK_KW_RESTRICT,    /* restrict                          */
    TOK_KW_IF,          /* if                                */
    TOK_KW_ELSE,        /* else                              */
    TOK_KW_WHILE,       /* while                             */
    TOK_KW_FOR,         /* for                               */
    TOK_KW_DO,          /* do                                */
    TOK_KW_SWITCH,      /* switch                            */
    TOK_KW_CASE,        /* case                              */
    TOK_KW_DEFAULT,     /* default                           */
    TOK_KW_BREAK,       /* break                             */
    TOK_KW_CONTINUE,    /* continue                          */
    TOK_KW_RETURN,      /* return                            */
    TOK_KW_GOTO,        /* goto                              */
    TOK_KW_SIZEOF,      /* sizeof                            */
    TOK_KW_TYPEOF,      /* typeof (extension)               */

    /* -- Punctuation -------------------------------------- */
    TOK_LPAREN,         /* (                                 */
    TOK_RPAREN,         /* )                                 */
    TOK_LBRACE,         /* {                                 */
    TOK_RBRACE,         /* }                                 */
    TOK_LBRACKET,       /* [                                 */
    TOK_RBRACKET,       /* ]                                 */
    TOK_SEMICOLON,      /* ;                                 */
    TOK_COLON,          /* :                                 */
    TOK_COMMA,          /* ,                                 */
    TOK_DOT,            /* .                                 */
    TOK_ARROW,          /* ->                                */
    TOK_ELLIPSIS,       /* ...                               */

    /* -- Operators ---------------------------------------- */
    TOK_PLUS,           /* +                                 */
    TOK_MINUS,          /* -                                 */
    TOK_STAR,           /* *                                 */
    TOK_SLASH,          /* /                                 */
    TOK_PERCENT,        /* %                                 */
    TOK_AMP,            /* &                                 */
    TOK_PIPE,           /* |                                 */
    TOK_CARET,          /* ^                                 */
    TOK_TILDE,          /* ~                                 */
    TOK_BANG,           /* !                                 */
    TOK_LSHIFT,         /* <<                                */
    TOK_RSHIFT,         /* >>                                */
    TOK_AMP_AMP,        /* &&                                */
    TOK_PIPE_PIPE,      /* ||                                */
    TOK_EQ_EQ,          /* ==                                */
    TOK_BANG_EQ,        /* !=                                */
    TOK_LT,             /* <                                 */
    TOK_GT,             /* >                                 */
    TOK_LT_EQ,          /* <=                                */
    TOK_GT_EQ,          /* >=                                */
    TOK_EQ,             /* =                                 */
    TOK_PLUS_EQ,        /* +=                                */
    TOK_MINUS_EQ,       /* -=                                */
    TOK_STAR_EQ,        /* *=                                */
    TOK_SLASH_EQ,       /* /=                                */
    TOK_PERCENT_EQ,     /* %=                                */
    TOK_AMP_EQ,         /* &=                                */
    TOK_PIPE_EQ,        /* |=                                */
    TOK_CARET_EQ,       /* ^=                                */
    TOK_LSHIFT_EQ,      /* <<=                               */
    TOK_RSHIFT_EQ,      /* >>=                               */
    TOK_PLUS_PLUS,      /* ++                                */
    TOK_MINUS_MINUS,    /* --                                */
    TOK_QUESTION,       /* ?                                 */
    TOK_HASH,           /* # (preprocessor)                 */

    /* -- Special ------------------------------------------ */
    TOK_EOF,
    TOK_UNKNOWN,
} uiox_token_kind_t;

#define UIOX_TOKEN_TEXT_MAX 256

typedef struct uiox_token {
    uiox_token_kind_t kind;
    char              text[UIOX_TOKEN_TEXT_MAX];
    int               line;
    int               col;
    union {
        long long  ival;   /* integer literal value         */
        double     fval;   /* float literal value           */
    } val;
} uiox_token_t;

const char *uiox_token_kind_str(uiox_token_kind_t kind);

#endif /* UIOX_TOKEN_H */
