/*
 * uiox_parser.c - UIOX recursive-descent parser
 */
#include "../include/uiox_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* -- Helpers ------------------------------------------------ */
static uiox_token_t peek(uiox_parser_t *p)
{
    return uiox_lexer_peek(p->lex);
}

static uiox_token_t consume(uiox_parser_t *p)
{
    return uiox_lexer_consume(p->lex);
}

static int match(uiox_parser_t *p, uiox_token_kind_t k)
{
    if (peek(p).kind == k) { consume(p); return 1; }
    return 0;
}

static int expect(uiox_parser_t *p, uiox_token_kind_t k)
{
    return uiox_lexer_expect(p->lex, k);
}

static uiox_ast_node_t *node(uiox_ast_kind_t kind, uiox_parser_t *p)
{
    uiox_token_t t = peek(p);
    return uiox_ast_alloc(kind, t.line, t.col);
}

/* -- Init / free ------------------------------------------- */
void uiox_parser_init(uiox_parser_t *p, uiox_lexer_t *lex,
                       uiox_diag_ctx_t *diag)
{
    p->lex  = lex;
    p->diag = diag;
    uiox_symtab_init(&p->symtab);
}

void uiox_parser_free(uiox_parser_t *p)
{
    uiox_symtab_free(&p->symtab);
}

/* -- Type parsing ------------------------------------------ */
static int is_type_start(uiox_token_kind_t k)
{
    switch (k) {
        case TOK_KW_INT:
        case TOK_KW_UINT:
        case TOK_KW_LONG:
        case TOK_KW_ULONG:
        case TOK_KW_SHORT:
        case TOK_KW_CHAR:
        case TOK_KW_FLOAT:
        case TOK_KW_DOUBLE:
        case TOK_KW_VOID:
        case TOK_KW_STRUCT:
        case TOK_KW_UNION:
        case TOK_KW_CONST:
        case TOK_KW_UNSIGNED:    /* <-- was missing */
        case TOK_KW_SIGNED:      /* <-- was missing */
        case TOK_KW_AUTO:        /* <-- was missing */
            return 1;
        default:
            return 0;
    }
}


uiox_ast_node_t *uiox_parse_type(uiox_parser_t *p)
{
    uiox_ast_node_t *t = node(AST_TYPE_BASE, p);
    t->type_base.is_const    = 0;
    t->type_base.is_unsigned = 0;

    /* -- consume leading qualifiers: const / unsigned / signed -- */
    for (;;) {
        uiox_token_t q = peek(p);
        if (q.kind == TOK_KW_CONST) {
            consume(p);
            t->type_base.is_const = 1;
        } else if (q.kind == TOK_KW_UNSIGNED) {
            consume(p);
            t->type_base.is_unsigned = 1;
        } else if (q.kind == TOK_KW_SIGNED) {
            consume(p);
            t->type_base.is_unsigned = 0;
        } else if (q.kind == TOK_KW_VOLATILE ||
                   q.kind == TOK_KW_RESTRICT) {
            consume(p);   /* consume and ignore for now */
        } else {
            break;
        }
    }

    /* -- base type specifier ---------------------------------- */
    uiox_token_t tok = consume(p);

    switch (tok.kind) {
        case TOK_KW_VOID:
            t->type_base.base_kind  = TOK_KW_VOID;
            t->type_base.size_bytes = 0;
            break;
        case TOK_KW_CHAR:
            t->type_base.base_kind  = TOK_KW_CHAR;
            t->type_base.size_bytes = 1;
            break;
        case TOK_KW_SHORT:
            t->type_base.base_kind  = TOK_KW_SHORT;
            t->type_base.size_bytes = 2;
            break;
        case TOK_KW_INT:
        case TOK_KW_UINT:
            t->type_base.base_kind  = TOK_KW_INT;
            t->type_base.size_bytes = 4;
            break;
        case TOK_KW_LONG:
        case TOK_KW_ULONG:
            t->type_base.base_kind  = TOK_KW_LONG;
            t->type_base.size_bytes = 8;
            break;
        case TOK_KW_FLOAT:
            t->type_base.base_kind  = TOK_KW_FLOAT;
            t->type_base.size_bytes = 4;
            break;
        case TOK_KW_DOUBLE:
            t->type_base.base_kind  = TOK_KW_DOUBLE;
            t->type_base.size_bytes = 8;
            break;
        case TOK_KW_STRUCT:
        case TOK_KW_UNION: {
            /* struct/union type reference: struct Foo */
            t->kind = AST_TYPE_STRUCT_REF;
            if (peek(p).kind == TOK_IDENT) {
                uiox_token_t sname = consume(p);
                strncpy(t->rec.name, sname.text,
                        sizeof(t->rec.name) - 1);
            }
            break;
        }
        default:
            /* fallback: treat as int */
            t->type_base.base_kind  = TOK_KW_INT;
            t->type_base.size_bytes = 4;
            break;
    }

    /* -- pointer declarators ---------------------------------- */
    while (peek(p).kind == TOK_STAR) {
        consume(p);
        uiox_ast_node_t *ptr = uiox_ast_alloc(AST_TYPE_PTR,
                                               t->line, t->col);
        ptr->type_ptr.pointee  = t;
        ptr->type_ptr.is_const = 0;
        if (peek(p).kind == TOK_KW_CONST) {
            consume(p);
            ptr->type_ptr.is_const = 1;
        }
        t = ptr;
    }

    return t;
}


/* -- Primary expression ------------------------------------ */
uiox_ast_node_t *uiox_parse_primary(uiox_parser_t *p)
{
    uiox_token_t tok = peek(p);

    if (tok.kind == TOK_INT_LIT) {
        consume(p);
        uiox_ast_node_t *n = uiox_ast_alloc(AST_INT_LIT,
                                              tok.line, tok.col);
        n->ival.val = tok.val.ival;
        return n;
    }
    if (tok.kind == TOK_FLOAT_LIT) {
        consume(p);
        uiox_ast_node_t *n = uiox_ast_alloc(AST_FLOAT_LIT,
                                              tok.line, tok.col);
        n->fval.val = tok.val.fval;
        return n;
    }
    if (tok.kind == TOK_STR_LIT) {
        consume(p);
        uiox_ast_node_t *n = uiox_ast_alloc(AST_STR_LIT,
                                              tok.line, tok.col);
        strncpy(n->sval.text, tok.text, sizeof(n->sval.text) - 1);
        n->sval.len = (int)strlen(tok.text);
        return n;
    }
    if (tok.kind == TOK_CHAR_LIT) {
        consume(p);
        uiox_ast_node_t *n = uiox_ast_alloc(AST_INT_LIT,
                                              tok.line, tok.col);
        n->ival.val = tok.val.ival;
        return n;
    }
    if (tok.kind == TOK_IDENT) {
        consume(p);
        uiox_ast_node_t *n = uiox_ast_alloc(AST_IDENT_EXPR,
                                              tok.line, tok.col);
        strncpy(n->ident.name, tok.text, sizeof(n->ident.name) - 1);
        return n;
    }
    if (tok.kind == TOK_LPAREN) {
        consume(p);
        uiox_ast_node_t *e = uiox_parse_expr(p);
        expect(p, TOK_RPAREN);
        return e;
    }
    /* error recovery */
    UIOX_ERR(p->diag, "<input>", tok.line, tok.col,
             "unexpected token '%s' in expression", tok.text);
    consume(p);
    return uiox_ast_alloc(AST_INT_LIT, tok.line, tok.col);
}

/* -- Postfix expression ------------------------------------ */
uiox_ast_node_t *uiox_parse_postfix(uiox_parser_t *p)
{
    uiox_ast_node_t *e = uiox_parse_primary(p);
    for (;;) {
        uiox_token_t t = peek(p);
        if (t.kind == TOK_LPAREN) {
            /* function call */
            consume(p);
            uiox_ast_node_t *call = uiox_ast_alloc(AST_CALL_EXPR,
                                                     t.line, t.col);
            call->call.callee = e;
            if (peek(p).kind != TOK_RPAREN) {
                do {
                    uiox_ast_list_push(&call->call.args,
                                       uiox_parse_assign_expr(p));
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN);
            e = call;
        } else if (t.kind == TOK_LBRACKET) {
            consume(p);
            uiox_ast_node_t *idx = uiox_ast_alloc(AST_INDEX_EXPR,
                                                    t.line, t.col);
            idx->index.base  = e;
            idx->index.index = uiox_parse_expr(p);
            expect(p, TOK_RBRACKET);
            e = idx;
        } else if (t.kind == TOK_DOT) {
            consume(p);
            uiox_token_t mem = consume(p);
            uiox_ast_node_t *m = uiox_ast_alloc(AST_MEMBER_EXPR,
                                                  t.line, t.col);
            m->member.obj = e;
            strncpy(m->member.member, mem.text,
                    sizeof(m->member.member) - 1);
            e = m;
        } else if (t.kind == TOK_ARROW) {
            consume(p);
            uiox_token_t mem = consume(p);
            uiox_ast_node_t *m = uiox_ast_alloc(AST_ARROW_EXPR,
                                                  t.line, t.col);
            m->member.obj = e;
            strncpy(m->member.member, mem.text,
                    sizeof(m->member.member) - 1);
            e = m;
        } else if (t.kind == TOK_PLUS_PLUS) {
            consume(p);
            uiox_ast_node_t *u = uiox_ast_alloc(AST_UNOP_EXPR,
                                                  t.line, t.col);
            u->unop.op      = TOK_PLUS_PLUS;
            u->unop.operand = e;
            u->unop.postfix = 1;
            e = u;
        } else if (t.kind == TOK_MINUS_MINUS) {
            consume(p);
            uiox_ast_node_t *u = uiox_ast_alloc(AST_UNOP_EXPR,
                                                  t.line, t.col);
            u->unop.op      = TOK_MINUS_MINUS;
            u->unop.operand = e;
            u->unop.postfix = 1;
            e = u;
        } else {
            break;
        }
    }
    return e;
}

/* -- Unary expression -------------------------------------- */
uiox_ast_node_t *uiox_parse_unary(uiox_parser_t *p)
{
    uiox_token_t t = peek(p);
    switch (t.kind) {
        case TOK_BANG: case TOK_TILDE: case TOK_MINUS:
        case TOK_STAR: case TOK_AMP:  case TOK_PLUS: {
            consume(p);
            uiox_ast_node_t *u = uiox_ast_alloc(AST_UNOP_EXPR,
                                                  t.line, t.col);
            u->unop.op      = t.kind;
            u->unop.operand = uiox_parse_unary(p);
            u->unop.postfix = 0;
            return u;
        }
        case TOK_PLUS_PLUS: case TOK_MINUS_MINUS: {
            consume(p);
            uiox_ast_node_t *u = uiox_ast_alloc(AST_UNOP_EXPR,
                                                  t.line, t.col);
            u->unop.op      = t.kind;
            u->unop.operand = uiox_parse_unary(p);
            u->unop.postfix = 0;
            return u;
        }
        case TOK_KW_SIZEOF: {
            consume(p);
            uiox_ast_node_t *s = uiox_ast_alloc(AST_SIZEOF_EXPR,
                                                  t.line, t.col);
            expect(p, TOK_LPAREN);
            if (is_type_start(peek(p).kind))
                s->szof.type = uiox_parse_type(p);
            else
                s->szof.expr = uiox_parse_unary(p);
            expect(p, TOK_RPAREN);
            return s;
        }
        default:
            return uiox_parse_postfix(p);
    }
}

/* -- Binary expression helpers (Pratt/precedence climb) ---- */
static int binop_prec(uiox_token_kind_t k)
{
    switch (k) {
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 13;
        case TOK_PLUS: case TOK_MINUS:                   return 12;
        case TOK_LSHIFT: case TOK_RSHIFT:                return 11;
        case TOK_LT: case TOK_GT:
        case TOK_LT_EQ: case TOK_GT_EQ:                 return 10;
        case TOK_EQ_EQ: case TOK_BANG_EQ:               return 9;
        case TOK_AMP:                                    return 8;
        case TOK_CARET:                                  return 7;
        case TOK_PIPE:                                   return 6;
        case TOK_AMP_AMP:                                return 5;
        case TOK_PIPE_PIPE:                              return 4;
        default:                                         return -1;
    }
}

static uiox_ast_node_t *parse_binop(uiox_parser_t *p,
                                     uiox_ast_node_t *lhs,
                                     int min_prec)
{
    for (;;) {
        uiox_token_t t = peek(p);
        int prec = binop_prec(t.kind);
        if (prec < min_prec) break;
        consume(p);
        uiox_ast_node_t *rhs = uiox_parse_unary(p);
        uiox_token_t nt = peek(p);
        int nprec = binop_prec(nt.kind);
        while (nprec > prec) {
            rhs   = parse_binop(p, rhs, prec + 1);
            nt    = peek(p);
            nprec = binop_prec(nt.kind);
        }
        uiox_ast_node_t *bin = uiox_ast_alloc(AST_BINOP_EXPR,
                                               t.line, t.col);
        bin->binop.op    = t.kind;
        bin->binop.left  = lhs;
        bin->binop.right = rhs;
        lhs = bin;
    }
    return lhs;
}

uiox_ast_node_t *uiox_parse_multiplicative(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 13); }

uiox_ast_node_t *uiox_parse_additive(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 12); }

uiox_ast_node_t *uiox_parse_shift(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 11); }

uiox_ast_node_t *uiox_parse_relational(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 10); }

uiox_ast_node_t *uiox_parse_equality(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 9); }

uiox_ast_node_t *uiox_parse_bitand(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 8); }

uiox_ast_node_t *uiox_parse_bitxor(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 7); }

uiox_ast_node_t *uiox_parse_bitor(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 6); }

uiox_ast_node_t *uiox_parse_logical_and(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 5); }

uiox_ast_node_t *uiox_parse_logical_or(uiox_parser_t *p)
{ return parse_binop(p, uiox_parse_unary(p), 4); }

uiox_ast_node_t *uiox_parse_ternary_expr(uiox_parser_t *p)
{
    uiox_ast_node_t *cond = parse_binop(p, uiox_parse_unary(p), 4);
    if (peek(p).kind == TOK_QUESTION) {
        uiox_token_t qt = consume(p);
        uiox_ast_node_t *tn = uiox_ast_alloc(AST_TERNARY_EXPR,
                                               qt.line, qt.col);
        tn->ternary.cond   = cond;
        tn->ternary.then_e = uiox_parse_expr(p);
        expect(p, TOK_COLON);
        tn->ternary.else_e = uiox_parse_ternary_expr(p);
        return tn;
    }
    return cond;
}

static int is_assign_op(uiox_token_kind_t k)
{
    switch (k) {
        case TOK_EQ: case TOK_PLUS_EQ: case TOK_MINUS_EQ:
        case TOK_STAR_EQ: case TOK_SLASH_EQ: case TOK_PERCENT_EQ:
        case TOK_AMP_EQ: case TOK_PIPE_EQ: case TOK_CARET_EQ:
        case TOK_LSHIFT_EQ: case TOK_RSHIFT_EQ:
            return 1;
        default: return 0;
    }
}

uiox_ast_node_t *uiox_parse_assign_expr(uiox_parser_t *p)
{
    uiox_ast_node_t *lhs = uiox_parse_ternary_expr(p);
    uiox_token_t t = peek(p);
    if (is_assign_op(t.kind)) {
        consume(p);
        uiox_ast_node_t *asgn = uiox_ast_alloc(AST_ASSIGN_EXPR,
                                                 t.line, t.col);
        asgn->binop.op    = t.kind;
        asgn->binop.left  = lhs;
        asgn->binop.right = uiox_parse_assign_expr(p);
        return asgn;
    }
    return lhs;
}

uiox_ast_node_t *uiox_parse_expr(uiox_parser_t *p)
{
    return uiox_parse_assign_expr(p);
}

/* -- Statement parsing ------------------------------------- */
uiox_ast_node_t *uiox_parse_return_stmt(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume 'return' */
    uiox_ast_node_t *r = uiox_ast_alloc(AST_RETURN_STMT, t.line, t.col);
    if (peek(p).kind != TOK_SEMICOLON)
        r->ret.expr = uiox_parse_expr(p);
    expect(p, TOK_SEMICOLON);
    return r;
}

uiox_ast_node_t *uiox_parse_if_stmt(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume 'if' */
    uiox_ast_node_t *s = uiox_ast_alloc(AST_IF_STMT, t.line, t.col);
    expect(p, TOK_LPAREN);
    s->if_stmt.cond   = uiox_parse_expr(p);
    expect(p, TOK_RPAREN);
    s->if_stmt.then_s = uiox_parse_stmt(p);
    if (peek(p).kind == TOK_KW_ELSE) {
        consume(p);
        s->if_stmt.else_s = uiox_parse_stmt(p);
    }
    return s;
}

uiox_ast_node_t *uiox_parse_while_stmt(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume 'while' */
    uiox_ast_node_t *s = uiox_ast_alloc(AST_WHILE_STMT, t.line, t.col);
    expect(p, TOK_LPAREN);
    s->loop.cond = uiox_parse_expr(p);
    expect(p, TOK_RPAREN);
    s->loop.body = uiox_parse_stmt(p);
    return s;
}

uiox_ast_node_t *uiox_parse_for_stmt(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume 'for' */
    uiox_ast_node_t *s = uiox_ast_alloc(AST_FOR_STMT, t.line, t.col);
    expect(p, TOK_LPAREN);
    if (peek(p).kind != TOK_SEMICOLON)
        s->for_stmt.init = uiox_parse_expr(p);
    expect(p, TOK_SEMICOLON);
    if (peek(p).kind != TOK_SEMICOLON)
        s->for_stmt.cond = uiox_parse_expr(p);
    expect(p, TOK_SEMICOLON);
    if (peek(p).kind != TOK_RPAREN)
        s->for_stmt.step = uiox_parse_expr(p);
    expect(p, TOK_RPAREN);
    s->for_stmt.body = uiox_parse_stmt(p);
    return s;
}

uiox_ast_node_t *uiox_parse_do_stmt(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume 'do' */
    uiox_ast_node_t *s = uiox_ast_alloc(AST_DO_STMT, t.line, t.col);
    s->loop.body = uiox_parse_stmt(p);
    if (peek(p).kind == TOK_KW_WHILE) consume(p);
    expect(p, TOK_LPAREN);
    s->loop.cond = uiox_parse_expr(p);
    expect(p, TOK_RPAREN);
    expect(p, TOK_SEMICOLON);
    return s;
}

uiox_ast_node_t *uiox_parse_compound(uiox_parser_t *p)
{
    uiox_token_t t = consume(p); /* consume '{' */
    uiox_ast_node_t *c = uiox_ast_alloc(AST_COMPOUND_STMT,
                                          t.line, t.col);
    while (peek(p).kind != TOK_RBRACE &&
           peek(p).kind != TOK_EOF) {
        uiox_ast_list_push(&c->compound.stmts, uiox_parse_stmt(p));
    }
    expect(p, TOK_RBRACE);
    return c;
}

uiox_ast_node_t *uiox_parse_stmt(uiox_parser_t *p)
{
    uiox_token_t t = peek(p);
    switch (t.kind) {
        case TOK_LBRACE:      return uiox_parse_compound(p);
        case TOK_KW_IF:       return uiox_parse_if_stmt(p);
        case TOK_KW_WHILE:    return uiox_parse_while_stmt(p);
        case TOK_KW_FOR:      return uiox_parse_for_stmt(p);
        case TOK_KW_DO:       return uiox_parse_do_stmt(p);
        case TOK_KW_RETURN:   return uiox_parse_return_stmt(p);
        case TOK_KW_BREAK: {
            consume(p);
            expect(p, TOK_SEMICOLON);
            return uiox_ast_alloc(AST_BREAK_STMT, t.line, t.col);
        }
        case TOK_KW_CONTINUE: {
            consume(p);
            expect(p, TOK_SEMICOLON);
            return uiox_ast_alloc(AST_CONTINUE_STMT, t.line, t.col);
        }
        default:
            if (is_type_start(t.kind))
                return uiox_parse_decl(p);
            else {
                uiox_ast_node_t *e = uiox_parse_expr(p);
                expect(p, TOK_SEMICOLON);
                return e;
            }
    }
}

/* -- Declaration parsing ----------------------------------- */
uiox_ast_node_t *uiox_parse_var_decl(uiox_parser_t *p,
                                       uiox_ast_node_t *type,
                                       const char *name)
{
    uiox_token_t t = peek(p);
    uiox_ast_node_t *v = uiox_ast_alloc(AST_VAR_DECL, t.line, t.col);
    v->var.type = type;
    strncpy(v->var.name, name, sizeof(v->var.name) - 1);
    if (match(p, TOK_EQ))
        v->var.init = uiox_parse_assign_expr(p);
    expect(p, TOK_SEMICOLON);
    return v;
}

uiox_ast_node_t *uiox_parse_func_decl(uiox_parser_t *p,
                                        uiox_ast_node_t *ret_type,
                                        const char *name)
{
    uiox_token_t t = peek(p);
    uiox_ast_node_t *fn = uiox_ast_alloc(AST_FUNC_DECL, t.line, t.col);
    fn->func.ret_type = ret_type;
    strncpy(fn->func.name, name, sizeof(fn->func.name) - 1);
    /* parse parameter list */
    while (peek(p).kind != TOK_RPAREN &&
           peek(p).kind != TOK_EOF) {
        if (peek(p).kind == TOK_ELLIPSIS) { consume(p); break; }
        uiox_ast_node_t *pty = uiox_parse_type(p);
        uiox_ast_node_t *par = uiox_ast_alloc(AST_PARAM_DECL,
                                               t.line, t.col);
        par->param.type = pty;
        if (peek(p).kind == TOK_IDENT) {
            uiox_token_t pn = consume(p);
            strncpy(par->param.name, pn.text,
                    sizeof(par->param.name) - 1);
        }
        uiox_ast_list_push(&fn->func.params, par);
        if (!match(p, TOK_COMMA)) break;
    }
    expect(p, TOK_RPAREN);
    if (peek(p).kind == TOK_LBRACE)
        fn->func.body = uiox_parse_compound(p);
    else
        expect(p, TOK_SEMICOLON);
    return fn;
}

uiox_ast_node_t *uiox_parse_decl(uiox_parser_t *p)
{
    uiox_ast_node_t *type = uiox_parse_type(p);
    uiox_token_t name_tok = consume(p);
    char name[128] = {0};
    strncpy(name, name_tok.text, sizeof(name) - 1);
    if (match(p, TOK_LPAREN))
        return uiox_parse_func_decl(p, type, name);
    else
        return uiox_parse_var_decl(p, type, name);
}

/* -- Translation unit -------------------------------------- */
uiox_ast_node_t *uiox_parse_translation_unit(uiox_parser_t *p)
{
    uiox_token_t t = peek(p);
    uiox_ast_node_t *tu = uiox_ast_alloc(AST_TRANSLATION_UNIT,
                                           t.line, t.col);
    while (peek(p).kind != TOK_EOF) {
        uiox_ast_node_t *d = uiox_parse_decl(p);
        if (d) uiox_ast_list_push(&tu->unit.decls, d);
    }
    return tu;
}
