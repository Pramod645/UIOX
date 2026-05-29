#ifndef UIOX_SYMTAB_H
#define UIOX_SYMTAB_H
/*
 * uiox_symtab.h - UIOX compiler symbol table
 */
#include "uiox_ast.h"

#define UIOX_SYM_HASH_SIZE  256

typedef enum uiox_sym_kind {
    SYM_VAR,
    SYM_FUNC,
    SYM_TYPEDEF,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_ENUM_CONST,
    SYM_LABEL,
} uiox_sym_kind_t;

typedef enum uiox_sym_storage {
    STORAGE_AUTO,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_REGISTER,
} uiox_sym_storage_t;

typedef struct uiox_symbol {
    char               name[128];
    uiox_sym_kind_t    kind;
    uiox_sym_storage_t storage;
    uiox_ast_node_t   *type;
    uiox_ast_node_t   *decl_node;
    int                scope_depth;
    int                offset;        /* stack offset (locals)    */
    int                is_defined;
    struct uiox_symbol *next;         /* hash chain               */
} uiox_symbol_t;

typedef struct uiox_scope {
    uiox_symbol_t    *table[UIOX_SYM_HASH_SIZE];
    struct uiox_scope *parent;
    int               depth;
} uiox_scope_t;

typedef struct uiox_symtab {
    uiox_scope_t *current;
    int           depth;
} uiox_symtab_t;

void          uiox_symtab_init   (uiox_symtab_t *st);
void          uiox_symtab_free   (uiox_symtab_t *st);
void          uiox_symtab_push   (uiox_symtab_t *st);
void          uiox_symtab_pop    (uiox_symtab_t *st);
uiox_symbol_t *uiox_symtab_insert(uiox_symtab_t *st, const char *name,
                                   uiox_sym_kind_t kind);
uiox_symbol_t *uiox_symtab_lookup(uiox_symtab_t *st, const char *name);
uiox_symbol_t *uiox_symtab_lookup_local(uiox_symtab_t *st, const char *name);

#endif /* UIOX_SYMTAB_H */
