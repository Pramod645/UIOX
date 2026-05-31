/*
 * uiox_ld_symbol.c - UIOX linker global symbol table
 */
#include "../include/uiox_ld_symbol.h"
#include "../include/uiox_ld_diag.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uld_u32_t sym_hash(const char *name)
{
    uld_u32_t h = 5381;
    while (*name) h = ((h << 5) + h) ^ (unsigned char)*name++;
    return h % ULD_SYM_HASH_SIZE;
}

void uld_sym_table_init(uld_sym_table_t *st)
{
    memset(st, 0, sizeof(*st));
}

void uld_sym_table_free(uld_sym_table_t *st)
{
    for (uld_u32_t i = 0; i < st->count; i++)
        free(st->list[i]);
    memset(st, 0, sizeof(*st));
}

uld_symbol_t *uld_sym_lookup(uld_sym_table_t *st, const char *name)
{
    uld_u32_t h = sym_hash(name);
    for (uld_symbol_t *s = st->hash[h]; s; s = s->hash_next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

uld_symbol_t *uld_sym_insert(uld_sym_table_t *st, const char *name,
                               uld_sym_bind_t bind, uld_sym_type_t type)
{
    if (st->count >= ULD_MAX_SYMBOLS) return NULL;
    uld_symbol_t *s = (uld_symbol_t *)calloc(1, sizeof(*s));
    if (!s) return NULL;
    strncpy(s->name, name, ULD_NAME_MAX - 1);
    s->bind = bind;
    s->type = type;
    uld_u32_t h  = sym_hash(name);
    s->hash_next = st->hash[h];
    st->hash[h]  = s;
    st->list[st->count++] = s;
    return s;
}

int uld_sym_define(uld_sym_table_t *st, const char *name,
                    uld_addr_t value, uld_u64_t size,
                    uld_u32_t sect_idx, uld_u32_t obj_idx,
                    uld_sym_bind_t bind, uld_sym_type_t type,
                    uld_diag_ctx_t *diag)
{
    uld_symbol_t *s = uld_sym_lookup(st, name);
    if (s) {
        if (s->defined && bind != ULD_BIND_WEAK &&
            s->bind != ULD_BIND_WEAK) {
            ULD_ERR(diag, name, 0, "duplicate symbol: %s", name);
            return -1;
        }
        /* weak: prefer strong definition */
        if (s->bind == ULD_BIND_WEAK && bind == ULD_BIND_GLOBAL) {
            s->value    = value;
            s->size     = size;
            s->sect_idx = sect_idx;
            s->obj_idx  = obj_idx;
            s->bind     = bind;
            s->defined  = ULD_TRUE;
        }
        return 0;
    }
    s = uld_sym_insert(st, name, bind, type);
    if (!s) return -1;
    s->value    = value;
    s->size     = size;
    s->sect_idx = sect_idx;
    s->obj_idx  = obj_idx;
    s->defined  = ULD_TRUE;
    return 0;
}

int uld_sym_check_undef(uld_sym_table_t *st, uld_diag_ctx_t *diag)
{
    int errors = 0;
    for (uld_u32_t i = 0; i < st->count; i++) {
        uld_symbol_t *s = st->list[i];
        if (!s->defined && s->referenced &&
             s->bind != ULD_BIND_WEAK) {
            ULD_ERR(diag, "<linker>", 0,
                    "undefined symbol: %s", s->name);
            errors++;
        }
    }
    return errors;
}

void uld_sym_define_abs(uld_sym_table_t *st, const char *name,
                         uld_addr_t value)
{
    uld_symbol_t *s = uld_sym_lookup(st, name);
    if (!s) s = uld_sym_insert(st, name, ULD_BIND_GLOBAL, ULD_SYM_NOTYPE);
    if (!s) return;
    s->value   = value;
    s->defined = ULD_TRUE;
    s->resolved= ULD_TRUE;
}

void uld_sym_print(const uld_sym_table_t *st)
{
    printf("Symbol table (%u symbols):\n", st->count);
    for (uld_u32_t i = 0; i < st->count; i++) {
        const uld_symbol_t *s = st->list[i];
        printf("  %c %016llx  %-6s  %s\n",
               s->defined ? (s->bind == ULD_BIND_GLOBAL ? 'G' : 'L') : 'U',
               (unsigned long long)s->value,
               s->bind == ULD_BIND_GLOBAL ? "global"
             : s->bind == ULD_BIND_WEAK   ? "weak" : "local",
               s->name);
    }
}
