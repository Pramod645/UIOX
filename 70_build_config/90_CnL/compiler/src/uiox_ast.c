/*
 * uiox_ast.c - UIOX AST node allocation and printing
 */
#include "../include/uiox_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uiox_ast_node_t *uiox_ast_alloc(uiox_ast_kind_t kind, int line, int col)
{
    uiox_ast_node_t *n = (uiox_ast_node_t *)calloc(1, sizeof(*n));
    if (!n) return NULL;
    n->kind = kind;
    n->line = line;
    n->col  = col;
    return n;
}

void uiox_ast_list_push(uiox_ast_list_t *list, uiox_ast_node_t *node)
{
    if (list->count >= list->cap) {
        int newcap = list->cap ? list->cap * 2 : 8;
        list->items = (uiox_ast_node_t **)realloc(
                          list->items,
                          (unsigned)newcap * sizeof(uiox_ast_node_t *));
        list->cap = newcap;
    }
    list->items[list->count++] = node;
}

void uiox_ast_free(uiox_ast_node_t *node)
{
    if (!node) return;
    switch (node->kind) {
        case AST_TRANSLATION_UNIT:
            for (int i = 0; i < node->unit.decls.count; i++)
                uiox_ast_free(node->unit.decls.items[i]);
            free(node->unit.decls.items);
            break;
        case AST_FUNC_DECL:
            uiox_ast_free(node->func.ret_type);
            for (int i = 0; i < node->func.params.count; i++)
                uiox_ast_free(node->func.params.items[i]);
            free(node->func.params.items);
            uiox_ast_free(node->func.body);
            break;
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->compound.stmts.count; i++)
                uiox_ast_free(node->compound.stmts.items[i]);
            free(node->compound.stmts.items);
            break;
        case AST_BINOP_EXPR:
        case AST_ASSIGN_EXPR:
            uiox_ast_free(node->binop.left);
            uiox_ast_free(node->binop.right);
            break;
        case AST_UNOP_EXPR:
            uiox_ast_free(node->unop.operand);
            break;
        case AST_CALL_EXPR:
            uiox_ast_free(node->call.callee);
            for (int i = 0; i < node->call.args.count; i++)
                uiox_ast_free(node->call.args.items[i]);
            free(node->call.args.items);
            break;
        case AST_IF_STMT:
            uiox_ast_free(node->if_stmt.cond);
            uiox_ast_free(node->if_stmt.then_s);
            uiox_ast_free(node->if_stmt.else_s);
            break;
        case AST_RETURN_STMT:
            uiox_ast_free(node->ret.expr);
            break;
        case AST_VAR_DECL:
            uiox_ast_free(node->var.type);
            uiox_ast_free(node->var.init);
            break;
        default:
            break;
    }
    free(node);
}

static void indent_print(int depth)
{
    for (int i = 0; i < depth; i++) printf("  ");
}

void uiox_ast_print(const uiox_ast_node_t *n, int d)
{
    if (!n) return;
    indent_print(d);
    switch (n->kind) {
        case AST_TRANSLATION_UNIT:
            printf("[TranslationUnit]\n");
            for (int i = 0; i < n->unit.decls.count; i++)
                uiox_ast_print(n->unit.decls.items[i], d+1);
            break;
        case AST_FUNC_DECL:
            printf("[FuncDecl] %s\n", n->func.name);
            uiox_ast_print(n->func.body, d+1);
            break;
        case AST_VAR_DECL:
            printf("[VarDecl] %s\n", n->var.name);
            if (n->var.init) uiox_ast_print(n->var.init, d+1);
            break;
            case AST_COMPOUND_STMT:
            printf("[CompoundStmt]\n");
            for (int i = 0; i < n->compound.stmts.count; i++)
                uiox_ast_print(n->compound.stmts.items[i], d+1);
            break;
        case AST_IF_STMT:
            printf("[IfStmt]\n");
            indent_print(d+1); printf("cond:\n");
            uiox_ast_print(n->if_stmt.cond,   d+2);
            indent_print(d+1); printf("then:\n");
            uiox_ast_print(n->if_stmt.then_s, d+2);
            if (n->if_stmt.else_s) {
                indent_print(d+1); printf("else:\n");
                uiox_ast_print(n->if_stmt.else_s, d+2);
            }
            break;
        case AST_WHILE_STMT:
            printf("[WhileStmt]\n");
            uiox_ast_print(n->loop.cond, d+1);
            uiox_ast_print(n->loop.body, d+1);
            break;
        case AST_FOR_STMT:
            printf("[ForStmt]\n");
            uiox_ast_print(n->for_stmt.init, d+1);
            uiox_ast_print(n->for_stmt.cond, d+1);
            uiox_ast_print(n->for_stmt.step, d+1);
            uiox_ast_print(n->for_stmt.body, d+1);
            break;
        case AST_RETURN_STMT:
            printf("[ReturnStmt]\n");
            uiox_ast_print(n->ret.expr, d+1);
            break;
        case AST_BINOP_EXPR:
        case AST_ASSIGN_EXPR:
            printf("[BinOp op=%d]\n", n->binop.op);
            uiox_ast_print(n->binop.left,  d+1);
            uiox_ast_print(n->binop.right, d+1);
            break;
        case AST_UNOP_EXPR:
            printf("[UnOp op=%d postfix=%d]\n",
                   n->unop.op, n->unop.postfix);
            uiox_ast_print(n->unop.operand, d+1);
            break;
        case AST_CALL_EXPR:
            printf("[CallExpr]\n");
            uiox_ast_print(n->call.callee, d+1);
            for (int i = 0; i < n->call.args.count; i++)
                uiox_ast_print(n->call.args.items[i], d+1);
            break;
        case AST_INT_LIT:
            printf("[IntLit %lld]\n", n->ival.val);
            break;
        case AST_FLOAT_LIT:
            printf("[FloatLit %f]\n", n->fval.val);
            break;
        case AST_STR_LIT:
            printf("[StrLit \"%s\"]\n", n->sval.text);
            break;
        case AST_IDENT_EXPR:
            printf("[Ident %s]\n", n->ident.name);
            break;
        case AST_TYPE_BASE:
            printf("[TypeBase kind=%d size=%d]\n",
                   n->type_base.base_kind,
                   n->type_base.size_bytes);
            break;
        case AST_TYPE_PTR:
            printf("[TypePtr]\n");
            uiox_ast_print(n->type_ptr.pointee, d+1);
            break;
        case AST_STRUCT_DECL:
            printf("[StructDecl %s]\n", n->rec.name);
            for (int i = 0; i < n->rec.fields.count; i++)
                uiox_ast_print(n->rec.fields.items[i], d+1);
            break;
        default:
            printf("[ASTNode kind=%d]\n", n->kind);
            break;
    }
}
