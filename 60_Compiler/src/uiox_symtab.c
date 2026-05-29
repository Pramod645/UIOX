/*
 * uiox_symtab.c - UIOX symbol table implementation
 */
#include "../include/uiox_symtab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static unsigned int sym_hash(const char *name)
{
    unsigned int h = 5381;
    while (*name) h = ((h << 5) + h) ^ (unsigned char)*name++;
    return h % UIOX_SYM_HASH_SIZE;
}

void uiox_symtab_init(uiox_symtab_t *st)
{
    st->depth   = 0;
    st->current = NULL;
    uiox_symtab_push(st);  /* global scope */
}

void uiox_symtab_free(uiox_symtab_t *st)
{
    while (st->current)
        uiox_symtab_pop(st);
}

void uiox_symtab_push(uiox_symtab_t *st)
{
    uiox_scope_t *sc = (uiox_scope_t *)calloc(1, sizeof(uiox_scope_t));
    sc->parent  = st->current;
    sc->depth   = st->depth++;
    st->current = sc;
}

void uiox_symtab_pop(uiox_symtab_t *st)
{
    if (!st->current) return;
    uiox_scope_t *sc = st->current;
    /* free all symbols in this scope */
    for (int i = 0; i < UIOX_SYM_HASH_SIZE; i++) {
        uiox_symbol_t *s = sc->table[i];
        while (s) {
            uiox_symbol_t *nx = s->next;
            free(s);
            s = nx;
        }
    }
    st->current = sc->parent;
    st->depth--;
    free(sc);
}

uiox_symbol_t *uiox_symtab_insert(uiox_symtab_t *st,
                                    const char *name,
                                    uiox_sym_kind_t kind)
{
    unsigned int h = sym_hash(name);
    uiox_scope_t *sc = st->current;
    uiox_symbol_t *s = (uiox_symbol_t *)calloc(1, sizeof(uiox_symbol_t));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->kind        = kind;
    s->scope_depth = sc->depth;
    s->next        = sc->table[h];
    sc->table[h]   = s;
    return s;
}

uiox_symbol_t *uiox_symtab_lookup(uiox_symtab_t *st, const char *name)
{
    unsigned int h = sym_hash(name);
    for (uiox_scope_t *sc = st->current; sc; sc = sc->parent) {
        for (uiox_symbol_t *s = sc->table[h]; s; s = s->next)
            if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

uiox_symbol_t *uiox_symtab_lookup_local(uiox_symtab_t *st, const char *name)
{
    unsigned int h = sym_hash(name);
    for (uiox_symbol_t *s = st->current->table[h]; s; s = s->next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}
