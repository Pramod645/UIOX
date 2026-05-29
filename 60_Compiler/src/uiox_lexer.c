/*
 * uiox_lexer.c - UIOX source tokeniser
 */
#include "../include/uiox_lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* -- Keyword table ------------------------------------------ */
typedef struct { const char *word; uiox_token_kind_t kind; } kw_entry_t;
static const kw_entry_t kw_table[] = {
    {"int",       TOK_KW_INT},
    {"uint",      TOK_KW_UINT},
    {"long",      TOK_KW_LONG},
    {"ulong",     TOK_KW_ULONG},
    {"short",     TOK_KW_SHORT},
    {"char",      TOK_KW_CHAR},
    {"float",     TOK_KW_FLOAT},
    {"double",    TOK_KW_DOUBLE},
    {"void",      TOK_KW_VOID},
    {"struct",    TOK_KW_STRUCT},
    {"union",     TOK_KW_UNION},
    {"enum",      TOK_KW_ENUM},
    {"typedef",   TOK_KW_TYPEDEF},
    {"const",     TOK_KW_CONST},
    {"static",    TOK_KW_STATIC},
    {"extern",    TOK_KW_EXTERN},
    {"inline",    TOK_KW_INLINE},
    {"volatile",  TOK_KW_VOLATILE},
    {"register",  TOK_KW_REGISTER},
    {"unsigned",  TOK_KW_UNSIGNED},   /* <-- added */
    {"signed",    TOK_KW_SIGNED},     /* <-- added */
    {"auto",      TOK_KW_AUTO},       /* <-- added */
    {"restrict",  TOK_KW_RESTRICT},   /* <-- added */
    {"if",        TOK_KW_IF},
    {"else",      TOK_KW_ELSE},
    {"while",     TOK_KW_WHILE},
    {"for",       TOK_KW_FOR},
    {"do",        TOK_KW_DO},
    {"switch",    TOK_KW_SWITCH},
    {"case",      TOK_KW_CASE},
    {"default",   TOK_KW_DEFAULT},
    {"break",     TOK_KW_BREAK},
    {"continue",  TOK_KW_CONTINUE},
    {"return",    TOK_KW_RETURN},
    {"goto",      TOK_KW_GOTO},
    {"sizeof",    TOK_KW_SIZEOF},
    {"typeof",    TOK_KW_TYPEOF},
    {NULL,        TOK_UNKNOWN}
};

static uiox_token_kind_t kw_lookup(const char *s)
{
    for (int i = 0; kw_table[i].word; i++)
        if (strcmp(kw_table[i].word, s) == 0)
            return kw_table[i].kind;
    return TOK_IDENT;
}

void uiox_lexer_init(uiox_lexer_t *lex, const char *src,
                     int len, const char *filename,
                     uiox_diag_ctx_t *diag)
{
    lex->src      = src;
    lex->src_len  = len;
    lex->pos      = 0;
    lex->line     = 1;
    lex->col      = 1;
    lex->filename = filename;
    lex->diag     = diag;
    memset(&lex->cur,  0, sizeof(lex->cur));
    memset(&lex->next, 0, sizeof(lex->next));
    /* prime the lookahead */
    lex->cur  = uiox_lexer_next(lex);
    lex->next = uiox_lexer_next(lex);
}

static char lex_peek_c(uiox_lexer_t *lex, int ahead)
{
    int p = lex->pos + ahead;
    if (p >= lex->src_len) return '\0';
    return lex->src[p];
}

static char lex_adv(uiox_lexer_t *lex)
{
    char c = lex->src[lex->pos++];
    if (c == '\n') { lex->line++; lex->col = 1; }
    else            lex->col++;
    return c;
}

static void lex_skip_whitespace(uiox_lexer_t *lex)
{
    for (;;) {
        char c = lex_peek_c(lex, 0);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lex_adv(lex);
        } else if (c == '/' && lex_peek_c(lex, 1) == '/') {
            while (lex->pos < lex->src_len &&
                   lex->src[lex->pos] != '\n')
                lex->pos++;
        } else if (c == '/' && lex_peek_c(lex, 1) == '*') {
            lex->pos += 2; lex->col += 2;
            while (lex->pos + 1 < lex->src_len) {
                if (lex->src[lex->pos]   == '*' &&
                    lex->src[lex->pos+1] == '/') {
                    lex->pos += 2; lex->col += 2;
                    break;
                }
                lex_adv(lex);
            }
        } else break;
    }
}

uiox_token_t uiox_lexer_next(uiox_lexer_t *lex)
{
    uiox_token_t tok;
    memset(&tok, 0, sizeof(tok));

    lex_skip_whitespace(lex);

    tok.line = lex->line;
    tok.col  = lex->col;

    if (lex->pos >= lex->src_len) {
        tok.kind = TOK_EOF;
        return tok;
    }

    char c  = lex->src[lex->pos];
    char c2 = lex_peek_c(lex, 1);
    char c3 = lex_peek_c(lex, 2);

    /* -- Integer / float literal -------------------------- */
    if (isdigit(c) || (c == '.' && isdigit(c2))) {
        int i = 0; int is_float = 0;
        /* hex */
        if (c == '0' && (c2 == 'x' || c2 == 'X')) {
            tok.text[i++] = lex_adv(lex);
            tok.text[i++] = lex_adv(lex);
            while (isxdigit(lex_peek_c(lex, 0)))
                tok.text[i++] = lex_adv(lex);
            tok.val.ival = strtoll(tok.text, NULL, 16);
        } else {
            while (isdigit(lex_peek_c(lex, 0)))
                tok.text[i++] = lex_adv(lex);
            if (lex_peek_c(lex, 0) == '.') {
                is_float = 1;
                tok.text[i++] = lex_adv(lex);
                while (isdigit(lex_peek_c(lex, 0)))
                    tok.text[i++] = lex_adv(lex);
                tok.val.fval = atof(tok.text);
            } else {
                tok.val.ival = atoll(tok.text);
            }
        }
        tok.text[i] = '\0';
        tok.kind = is_float ? TOK_FLOAT_LIT : TOK_INT_LIT;
        return tok;
    }

    /* -- String literal ----------------------------------- */
    if (c == '"') {
        lex_adv(lex);
        int i = 0;
        while (lex->pos < lex->src_len && lex->src[lex->pos] != '"') {
            char ch = lex_adv(lex);
            if (ch == '\\') {
                char esc = lex_adv(lex);
                switch (esc) {
                    case 'n': tok.text[i++] = '\n'; break;
                    case 't': tok.text[i++] = '\t'; break;
                    case 'r': tok.text[i++] = '\r'; break;
                    case '0': tok.text[i++] = '\0'; break;
                    default:  tok.text[i++] = esc;  break;
                }
            } else {
                tok.text[i++] = ch;
            }
        }
        if (lex->pos < lex->src_len) lex_adv(lex); /* consume " */
        tok.text[i] = '\0';
        tok.kind = TOK_STR_LIT;
        return tok;
    }

    /* -- Char literal ------------------------------------- */
    if (c == '\'') {
        lex_adv(lex);
        char ch = lex_adv(lex);
        if (ch == '\\') {
            char esc = lex_adv(lex);
            switch (esc) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case '0': ch = '\0'; break;
                default:  ch = esc;  break;
            }
        }
        if (lex->pos < lex->src_len) lex_adv(lex); /* consume ' */
        tok.val.ival = ch;
        tok.text[0]  = ch; tok.text[1] = '\0';
        tok.kind = TOK_CHAR_LIT;
        return tok;
    }

    /* -- Identifier / keyword ----------------------------- */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (isalnum(lex_peek_c(lex, 0)) || lex_peek_c(lex, 0) == '_')
            tok.text[i++] = lex_adv(lex);
        tok.text[i] = '\0';
        tok.kind = kw_lookup(tok.text);
        return tok;
    }

    /* -- Multi-char operators ----------------------------- */
    lex_adv(lex);
    tok.text[0] = c; tok.text[1] = '\0';

#define MATCH2(a,b,k) if(c==(a)&&c2==(b)){lex_adv(lex);tok.text[1]=(b);tok.text[2]='\0';tok.kind=(k);return tok;}
#define MATCH3(a,b,d,k) if(c==(a)&&c2==(b)&&c3==(d)){lex_adv(lex);lex_adv(lex);tok.kind=(k);return tok;}

    MATCH3('<','<','=', TOK_LSHIFT_EQ)
    MATCH3('>','>','=', TOK_RSHIFT_EQ)
    MATCH3('.','.','.', TOK_ELLIPSIS)
    MATCH2('<','<', TOK_LSHIFT)
    MATCH2('>','>', TOK_RSHIFT)
    MATCH2('-','>', TOK_ARROW)
    MATCH2('+','+', TOK_PLUS_PLUS)
    MATCH2('-','-', TOK_MINUS_MINUS)
    MATCH2('&','&', TOK_AMP_AMP)
    MATCH2('|','|', TOK_PIPE_PIPE)
    MATCH2('=','=', TOK_EQ_EQ)
    MATCH2('!','=', TOK_BANG_EQ)
    MATCH2('<','=', TOK_LT_EQ)
    MATCH2('>','=', TOK_GT_EQ)
    MATCH2('+','=', TOK_PLUS_EQ)
    MATCH2('-','=', TOK_MINUS_EQ)
    MATCH2('*','=', TOK_STAR_EQ)
    MATCH2('/','=', TOK_SLASH_EQ)
    MATCH2('%','=', TOK_PERCENT_EQ)
    MATCH2('&','=', TOK_AMP_EQ)
    MATCH2('|','=', TOK_PIPE_EQ)
    MATCH2('^','=', TOK_CARET_EQ)

    /* Single-char tokens */
    switch (c) {
        case '(': tok.kind = TOK_LPAREN;    break;
        case ')': tok.kind = TOK_RPAREN;    break;
        case '{': tok.kind = TOK_LBRACE;    break;
        case '}': tok.kind = TOK_RBRACE;    break;
        case '[': tok.kind = TOK_LBRACKET;  break;
        case ']': tok.kind = TOK_RBRACKET;  break;
        case ';': tok.kind = TOK_SEMICOLON; break;
        case ':': tok.kind = TOK_COLON;     break;
        case ',': tok.kind = TOK_COMMA;     break;
        case '.': tok.kind = TOK_DOT;       break;
        case '+': tok.kind = TOK_PLUS;      break;
        case '-': tok.kind = TOK_MINUS;     break;
        case '*': tok.kind = TOK_STAR;      break;
        case '/': tok.kind = TOK_SLASH;     break;
        case '%': tok.kind = TOK_PERCENT;   break;
        case '&': tok.kind = TOK_AMP;       break;
        case '|': tok.kind = TOK_PIPE;      break;
        case '^': tok.kind = TOK_CARET;     break;
        case '~': tok.kind = TOK_TILDE;     break;
        case '!': tok.kind = TOK_BANG;      break;
        case '<': tok.kind = TOK_LT;        break;
        case '>': tok.kind = TOK_GT;        break;
        case '=': tok.kind = TOK_EQ;        break;
        case '?': tok.kind = TOK_QUESTION;  break;
        case '#': tok.kind = TOK_HASH;      break;
        default:  tok.kind = TOK_UNKNOWN;   break;
    }
    return tok;
}

uiox_token_t uiox_lexer_peek(uiox_lexer_t *lex)    { return lex->cur;  }
uiox_token_t uiox_lexer_consume(uiox_lexer_t *lex)
{
    uiox_token_t t = lex->cur;
    lex->cur  = lex->next;
    lex->next = uiox_lexer_next(lex);
    return t;
}

int uiox_lexer_expect(uiox_lexer_t *lex, uiox_token_kind_t k)
{
    if (lex->cur.kind != k) {
        UIOX_ERR(lex->diag, lex->filename,
                 lex->cur.line, lex->cur.col,
                 "expected '%s', got '%s'",
                 uiox_token_kind_str(k),
                 uiox_token_kind_str(lex->cur.kind));
        return 0;
    }
    uiox_lexer_consume(lex);
    return 1;
}

const char *uiox_token_kind_str(uiox_token_kind_t k)
{
    switch (k) {
        case TOK_INT_LIT:   return "<int>";
        case TOK_FLOAT_LIT: return "<float>";
        case TOK_STR_LIT:   return "<string>";
        case TOK_CHAR_LIT:  return "<char>";
        case TOK_IDENT:     return "<ident>";
        case TOK_KW_INT:    return "int";
        case TOK_KW_VOID:   return "void";
        case TOK_KW_RETURN: return "return";
        case TOK_KW_IF:     return "if";
        case TOK_KW_ELSE:   return "else";
        case TOK_KW_WHILE:  return "while";
        case TOK_KW_FOR:    return "for";
        case TOK_KW_STRUCT: return "struct";
        case TOK_LPAREN:    return "(";
        case TOK_RPAREN:    return ")";
        case TOK_LBRACE:    return "{";
        case TOK_RBRACE:    return "}";
        case TOK_SEMICOLON: return ";";
        case TOK_COMMA:     return ",";
        case TOK_EQ:        return "=";
        case TOK_KW_UNSIGNED:  return "unsigned";
        case TOK_KW_SIGNED:    return "signed";
        case TOK_KW_AUTO:      return "auto";
        case TOK_KW_RESTRICT:  return "restrict";
        case TOK_EOF:       return "<EOF>";
        default:            return "<token>";
    }
}
