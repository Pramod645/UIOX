#ifndef UIOX_AST_H
#define UIOX_AST_H
/*
 * uiox_ast.h - UIOX Abstract Syntax Tree node definitions
 */
#include "uiox_token.h"

typedef enum uiox_ast_kind {
    /* -- Declarations ------------------------------------- */
    AST_TRANSLATION_UNIT,
    AST_FUNC_DECL,
    AST_VAR_DECL,
    AST_PARAM_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,

    /* -- Statements --------------------------------------- */
    AST_COMPOUND_STMT,
    AST_EXPR_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_DO_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,

    /* -- Expressions -------------------------------------- */
    AST_ASSIGN_EXPR,
    AST_BINOP_EXPR,
    AST_UNOP_EXPR,
    AST_TERNARY_EXPR,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_MEMBER_EXPR,
    AST_ARROW_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_INT_LIT,
    AST_FLOAT_LIT,
    AST_STR_LIT,
    AST_CHAR_LIT,
    AST_IDENT_EXPR,

    /* -- Types -------------------------------------------- */
    AST_TYPE_BASE,
    AST_TYPE_PTR,
    AST_TYPE_ARRAY,
    AST_TYPE_FUNC,
    AST_TYPE_STRUCT_REF,
} uiox_ast_kind_t;

/* Forward declaration */
typedef struct uiox_ast_node uiox_ast_node_t;

/* -- Generic list of child nodes -------------------------- */
typedef struct uiox_ast_list {
    uiox_ast_node_t  **items;
    int                count;
    int                cap;
} uiox_ast_list_t;

/* -- AST node --------------------------------------------- */
struct uiox_ast_node {
    uiox_ast_kind_t kind;
    int             line;
    int             col;

    union {
        /* AST_TRANSLATION_UNIT */
        struct { uiox_ast_list_t decls; } unit;

        /* AST_FUNC_DECL */
        struct {
            char             name[128];
            uiox_ast_node_t *ret_type;
            uiox_ast_list_t  params;
            uiox_ast_node_t *body;
            int              is_static;
            int              is_inline;
        } func;

        /* AST_VAR_DECL */
        struct {
            char             name[128];
            uiox_ast_node_t *type;
            uiox_ast_node_t *init;
            int              is_static;
            int              is_extern;
            int              is_const;
        } var;

        /* AST_PARAM_DECL */
        struct {
            char             name[128];
            uiox_ast_node_t *type;
        } param;

        /* AST_COMPOUND_STMT */
        struct { uiox_ast_list_t stmts; } compound;

        /* AST_IF_STMT */
        struct {
            uiox_ast_node_t *cond;
            uiox_ast_node_t *then_s;
            uiox_ast_node_t *else_s;
        } if_stmt;

        /* AST_WHILE_STMT / AST_DO_STMT */
        struct {
            uiox_ast_node_t *cond;
            uiox_ast_node_t *body;
        } loop;

        /* AST_FOR_STMT */
        struct {
            uiox_ast_node_t *init;
            uiox_ast_node_t *cond;
            uiox_ast_node_t *step;
            uiox_ast_node_t *body;
        } for_stmt;

        /* AST_RETURN_STMT */
        struct { uiox_ast_node_t *expr; } ret;

        /* AST_BINOP_EXPR / AST_ASSIGN_EXPR */
        struct {
            uiox_token_kind_t  op;
            uiox_ast_node_t   *left;
            uiox_ast_node_t   *right;
        } binop;

        /* AST_UNOP_EXPR */
        struct {
            uiox_token_kind_t  op;
            uiox_ast_node_t   *operand;
            int                postfix;
        } unop;

        /* AST_CALL_EXPR */
        struct {
            uiox_ast_node_t *callee;
            uiox_ast_list_t  args;
        } call;

        /* AST_INDEX_EXPR */
        struct {
            uiox_ast_node_t *base;
            uiox_ast_node_t *index;
        } index;

        /* AST_MEMBER_EXPR / AST_ARROW_EXPR */
        struct {
            uiox_ast_node_t *obj;
            char             member[128];
        } member;

        /* AST_CAST_EXPR */
        struct {
            uiox_ast_node_t *type;
            uiox_ast_node_t *expr;
        } cast;

        /* AST_SIZEOF_EXPR */
        struct {
            uiox_ast_node_t *type;
            uiox_ast_node_t *expr;
        } szof;

        /* AST_INT_LIT */
        struct { long long val; unsigned is_unsigned; } ival;

        /* AST_FLOAT_LIT */
        struct { double val; } fval;

        /* AST_STR_LIT */
        struct { char text[512]; int len; } sval;

        /* AST_IDENT_EXPR */
        struct { char name[128]; } ident;

        /* AST_TYPE_BASE */
        struct {
            uiox_token_kind_t base_kind;
            unsigned          is_const    : 1;
            unsigned          is_volatile : 1;
            unsigned          is_unsigned : 1;
            unsigned          size_bytes  : 8;
        } type_base;

        /* AST_TYPE_PTR */
        struct {
            uiox_ast_node_t *pointee;
            unsigned         is_const : 1;
        } type_ptr;

        /* AST_TYPE_ARRAY */
        struct {
            uiox_ast_node_t *elem_type;
            uiox_ast_node_t *size_expr;
        } type_arr;

        /* AST_STRUCT_DECL / AST_UNION_DECL */
        struct {
            char            name[128];
            uiox_ast_list_t fields;
        } rec;

        /* AST_GOTO_STMT / AST_LABEL_STMT */
        struct { char label[128]; } label;

        /* AST_TERNARY_EXPR */
        struct {
            uiox_ast_node_t *cond;
            uiox_ast_node_t *then_e;
            uiox_ast_node_t *else_e;
        } ternary;
    };
};

uiox_ast_node_t *uiox_ast_alloc  (uiox_ast_kind_t kind, int line, int col);
void             uiox_ast_free   (uiox_ast_node_t *node);
void             uiox_ast_print  (const uiox_ast_node_t *node, int indent);
void             uiox_ast_list_push(uiox_ast_list_t *list, uiox_ast_node_t *node);

#endif /* UIOX_AST_H */
