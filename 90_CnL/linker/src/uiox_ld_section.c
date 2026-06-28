/*
 * uiox_ld_section.c - UIOX linker section management
 */
#include "../include/uiox_ld_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void uld_sect_table_init(uld_sect_table_t *st)
{
    memset(st, 0, sizeof(*st));
}

void uld_sect_table_free(uld_sect_table_t *st)
{
    for (uld_u32_t i = 0; i < st->count; i++) {
        uld_output_sect_t *s = &st->sects[i];
        free(s->data);
        /* free input section list */
        uld_input_sect_t *in = s->inputs;
        while (in) {
            uld_input_sect_t *nx = in->next;
            free(in->data);
            free(in);
            in = nx;
        }
    }
    memset(st, 0, sizeof(*st));
}

uld_output_sect_t *uld_sect_find(uld_sect_table_t *st, const char *name)
{
    for (uld_u32_t i = 0; i < st->count; i++)
        if (strcmp(st->sects[i].name, name) == 0)
            return &st->sects[i];
    return NULL;
}

uld_output_sect_t *uld_sect_get_or_create(uld_sect_table_t *st,
                                            const char *name,
                                            uld_sect_type_t type,
                                            uld_u32_t flags,
                                            uld_u32_t align)
{
    uld_output_sect_t *s = uld_sect_find(st, name);
    if (s) return s;
    if (st->count >= ULD_MAX_SECTIONS) return NULL;
    s = &st->sects[st->count];
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, ULD_NAME_MAX - 1);
    s->type  = type;
    s->flags = flags;
    s->align = align ? align : 1;
    s->idx   = st->count;
    s->cap   = 4096;
    s->data  = (uld_u8_t *)calloc(1, s->cap);
    st->count++;
    return s;
}

int uld_sect_append(uld_output_sect_t *out, uld_input_sect_t *in)
{
    /* align current size */
    uld_u32_t al  = in->align ? in->align : 1;
    uld_u64_t rem = out->size % al;
    uld_u64_t pad = rem ? (al - rem) : 0;
    uld_u64_t new_size = out->size + pad + in->size;

    /* grow buffer if needed */
    if (new_size > out->cap) {
        uld_u64_t nc = out->cap ? out->cap * 2 : 4096;
        while (nc < new_size) nc *= 2;
        uld_u8_t *nd = (uld_u8_t *)realloc(out->data, (size_t)nc);
        if (!nd) return -1;
        out->data = nd;
        out->cap  = nc;
    }

    /* zero padding */
    if (pad) memset(out->data + out->size, 0, (size_t)pad);
    in->output_off = out->size + pad;

    /* copy data (BSS has no bytes) */
    if (in->data && in->size)
        memcpy(out->data + in->output_off, in->data, in->size);
    else if (in->size)
        memset(out->data + in->output_off, 0, in->size);

    out->size = new_size;

    /* update alignment */
    if (al > out->align) out->align = al;

    /* link into list */
    in->next = NULL;
    if (!out->inputs) out->inputs = in;
    else              out->inputs_tail->next = in;
    out->inputs_tail = in;
    out->input_count++;
    return 0;
}

void uld_sect_patch32(uld_output_sect_t *s, uld_u64_t off, uld_u32_t val)
{
    if (off + 4 > s->size || !s->data) return;
    s->data[off+0] = (uld_u8_t)(val);
    s->data[off+1] = (uld_u8_t)(val >>  8);
    s->data[off+2] = (uld_u8_t)(val >> 16);
    s->data[off+3] = (uld_u8_t)(val >> 24);
}

void uld_sect_patch64(uld_output_sect_t *s, uld_u64_t off, uld_u64_t val)
{
    if (off + 8 > s->size || !s->data) return;
    for (int i = 0; i < 8; i++)
        s->data[off+i] = (uld_u8_t)(val >> (i * 8));
}

void uld_sect_print(const uld_sect_table_t *st)
{
    printf("Output sections (%u):\n", st->count);
    for (uld_u32_t i = 0; i < st->count; i++) {
        const uld_output_sect_t *s = &st->sects[i];
        printf("  [%2u] %-16s  vaddr=0x%016llx  size=0x%08llx"
               "  align=%u  inputs=%u\n",
               i, s->name,
               (unsigned long long)s->vaddr,
               (unsigned long long)s->size,
               s->align, s->input_count);
    }
}
