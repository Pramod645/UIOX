#ifndef UIOX_PARSER_H
#define UIOX_PARSER_H
/*
 * uiox_parser.h - UIOX recursive-descent parser
 */
#include "uiox_lexer.h"
#include "uiox_ast.h"
#include "uiox_symtab.h"
#include "uiox_error.h"

typedef struct uiox_parser {
    uiox_lexer_t    *lex;
    uiox_symtab_t    symtab;
    uiox_diag_ctx_t *diag;
} uiox_parser_t;

void             uiox_parser_init        (uiox_parser_t *p,
                                           uiox_lexer_t *lex,
                                           uiox_diag_ctx_t *diag);
void             uiox_parser_free        (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_translation_unit(uiox_parser_t *p);

/* -- Declaration parsers ------------------------------------ */
uiox_ast_node_t *uiox_parse_decl         (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_func_decl    (uiox_parser_t *p,
                                           uiox_ast_node_t *ret_type,
                                           const char *name);
uiox_ast_node_t *uiox_parse_var_decl     (uiox_parser_t *p,
                                           uiox_ast_node_t *type,
                                           const char *name);
uiox_ast_node_t *uiox_parse_struct_decl  (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_typedef_decl (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_type         (uiox_parser_t *p);

/* -- Statement parsers -------------------------------------- */
uiox_ast_node_t *uiox_parse_stmt         (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_compound     (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_if_stmt      (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_while_stmt   (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_for_stmt     (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_do_stmt      (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_return_stmt  (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_switch_stmt  (uiox_parser_t *p);

/* -- Expression parsers ------------------------------------- */
uiox_ast_node_t *uiox_parse_expr         (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_assign_expr  (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_ternary_expr (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_logical_or   (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_logical_and  (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_bitor        (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_bitxor       (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_bitand       (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_equality     (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_relational   (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_shift        (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_additive     (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_multiplicative(uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_unary        (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_postfix      (uiox_parser_t *p);
uiox_ast_node_t *uiox_parse_primary      (uiox_parser_t *p);

#endif /* UIOX_PARSER_H */
