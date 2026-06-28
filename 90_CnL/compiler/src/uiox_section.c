/*
 * uiox_section.c - UIOX section management
 */
#include "../include/uiox_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uiox_section_t *uiox_sect_create(const char *name,
                                  uiox_sect_type_t type,
                                  unsigned int flags,
                                  unsigned int align)
{
    uiox_section_t *s = (uiox_section_t *)calloc(1, sizeof(*s));
    strncpy(s->name, name, UIOX_SECT_NAME_MAX - 1);
    s->type  = type;
    s->flags = flags;
    s->align = align ? align : 1;
    s->cap   = 4096;
    s->data  = (unsigned char *)malloc(s->cap);
    s->size  = 0;
    return s;
}

void uiox_sect_free(uiox_section_t *s)
{
    if (!s) return;
    free(s->data);
    free(s);
}

static int sect_grow(uiox_section_t *s, unsigned int need)
{
    if (s->size + need <= s->cap) return 1;
    unsigned int newcap = s->cap;
    while (newcap < s->size + need) newcap *= 2;
    unsigned char *nd = (unsigned char *)realloc(s->data, newcap);
    if (!nd) return 0;
    s->data = nd;
    s->cap  = newcap;
    return 1;
}

int uiox_sect_write(uiox_section_t *s, const void *data, unsigned int len)
{
    if (!sect_grow(s, len)) return 0;
    memcpy(s->data + s->size, data, len);
    s->size += len;
    return 1;
}

int uiox_sect_write8(uiox_section_t *s, unsigned char v)
{
    return uiox_sect_write(s, &v, 1);
}

int uiox_sect_write16(uiox_section_t *s, unsigned short v)
{
    unsigned char b[2] = { (unsigned char)(v), (unsigned char)(v >> 8) };
    return uiox_sect_write(s, b, 2);
}

int uiox_sect_write32(uiox_section_t *s, unsigned int v)
{
    unsigned char b[4] = {
        (unsigned char)(v),       (unsigned char)(v >> 8),
        (unsigned char)(v >> 16), (unsigned char)(v >> 24)
    };
    return uiox_sect_write(s, b, 4);
}

int uiox_sect_write64(uiox_section_t *s, unsigned long long v)
{
    unsigned char b[8];
    for (int i = 0; i < 8; i++) b[i] = (unsigned char)(v >> (i * 8));
    return uiox_sect_write(s, b, 8);
}

int uiox_sect_pad(uiox_section_t *s, unsigned int align)
{
    if (align < 2) return 1;
    unsigned int rem = s->size % align;
    if (!rem) return 1;
    unsigned int pad = align - rem;
    if (!sect_grow(s, pad)) return 0;
    memset(s->data + s->size, 0, pad);
    s->size += pad;
    return 1;
}

unsigned int uiox_sect_pos(const uiox_section_t *s) { return s->size; }

void uiox_sect_patch32(uiox_section_t *s, unsigned int off, unsigned int val)
{
    if (off + 4 > s->size) return;
    s->data[off+0] = (unsigned char)(val);
    s->data[off+1] = (unsigned char)(val >> 8);
    s->data[off+2] = (unsigned char)(val >> 16);
    s->data[off+3] = (unsigned char)(val >> 24);
}

void uiox_sect_patch64(uiox_section_t *s, unsigned int off,
                        unsigned long long val)
{
    if (off + 8 > s->size) return;
    for (int i = 0; i < 8; i++)
        s->data[off+i] = (unsigned char)(val >> (i * 8));
}
