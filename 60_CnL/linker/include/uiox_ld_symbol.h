#ifndef UIOX_LD_SYMBOL_H
#define UIOX_LD_SYMBOL_H
/*
 * uiox_ld_symbol.h - UIOX linker global symbol table
 */
#include "uiox_ld_types.h"
#include "uiox_ld_diag.h"

#define ULD_SYM_HASH_SIZE  4096

/* -- Symbol binding ----------------------------------------- */
typedef enum uld_sym_bind {
    ULD_BIND_LOCAL  = 0,
    ULD_BIND_GLOBAL = 1,
    ULD_BIND_WEAK   = 2,
} uld_sym_bind_t;

/* -- Symbol type -------------------------------------------- */
typedef enum uld_sym_type {
    ULD_SYM_NOTYPE  = 0,
    ULD_SYM_OBJECT  = 1,
    ULD_SYM_FUNC    = 2,
    ULD_SYM_SECTION = 3,
    ULD_SYM_FILE    = 4,
    ULD_SYM_COMMON  = 5,
} uld_sym_type_t;

/* -- Symbol visibility -------------------------------------- */
typedef enum uld_sym_vis {
    ULD_VIS_DEFAULT   = 0,
    ULD_VIS_HIDDEN    = 1,
    ULD_VIS_PROTECTED = 2,
} uld_sym_vis_t;

/* -- Symbol record ------------------------------------------ */
typedef struct uld_symbol {
    char             name[ULD_NAME_MAX];
    uld_addr_t       value;       /* resolved virtual address      */
    uld_u64_t        size;        /* symbol size in bytes          */
    uld_sym_bind_t   bind;
    uld_sym_type_t   type;
    uld_sym_vis_t    vis;
    uld_u32_t        sect_idx;    /* output section index          */
    uld_u32_t        obj_idx;     /* which object defines it       */
    uld_bool_t       defined;     /* 1 = has a definition          */
    uld_bool_t       resolved;    /* 1 = value is final            */
    uld_bool_t       referenced;  /* 1 = used by a relocation      */
    struct uld_symbol *hash_next; /* hash chain                    */
} uld_symbol_t;

/* -- Global symbol table ------------------------------------ */
typedef struct uld_sym_table {
    uld_symbol_t  *hash[ULD_SYM_HASH_SIZE];
    uld_symbol_t  *list[ULD_MAX_SYMBOLS];  /* ordered for output  */
    uld_u32_t      count;
    uld_u32_t      undef_count;
} uld_sym_table_t;

void          uld_sym_table_init   (uld_sym_table_t *st);
void          uld_sym_table_free   (uld_sym_table_t *st);
uld_symbol_t *uld_sym_lookup       (uld_sym_table_t *st,
                                     const char *name);
uld_symbol_t *uld_sym_insert       (uld_sym_table_t *st,
                                     const char *name,
                                     uld_sym_bind_t bind,
                                     uld_sym_type_t type);
int           uld_sym_define       (uld_sym_table_t *st,
                                     const char *name,
                                     uld_addr_t value,
                                     uld_u64_t size,
                                     uld_u32_t sect_idx,
                                     uld_u32_t obj_idx,
                                     uld_sym_bind_t bind,
                                     uld_sym_type_t type,
                                     uld_diag_ctx_t *diag);
int           uld_sym_check_undef  (uld_sym_table_t *st,
                                     uld_diag_ctx_t *diag);
void          uld_sym_print        (const uld_sym_table_t *st);

/* -- Absolute / linker-defined symbols ---------------------- */
void uld_sym_define_abs(uld_sym_table_t *st, const char *name,
                         uld_addr_t value);

#endif /* UIOX_LD_SYMBOL_H */
