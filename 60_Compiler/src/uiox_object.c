/*
 * uiox_object.c - UIOX object file read/write
 */
#include "../include/uiox_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void uiox_object_init(uiox_object_t *obj, const char *srcfile,
                       unsigned int arch)
{
    memset(obj, 0, sizeof(*obj));
    obj->hdr.magic   = UIOX_OBJ_MAGIC;
    obj->hdr.version = UIOX_OBJ_VERSION;
    obj->hdr.arch    = arch;
    strncpy(obj->hdr.source_file, srcfile,
            sizeof(obj->hdr.source_file) - 1);
}

void uiox_object_free(uiox_object_t *obj)
{
    for (unsigned int i = 0; i < obj->hdr.sect_count; i++)
        uiox_sect_free(obj->sects[i]);
    memset(obj, 0, sizeof(*obj));
}

int uiox_object_add_sym(uiox_object_t *obj, const char *name,
                         unsigned int sect, unsigned long long val,
                         unsigned long long size,
                         uiox_sym_bind_t bind, uiox_sym_type_t type)
{
    if (obj->hdr.sym_count >= UIOX_OBJ_MAX_SYMS) return -1;
    uiox_obj_sym_t *s = &obj->syms[obj->hdr.sym_count++];
    strncpy(s->name, name, UIOX_SYM_NAME_MAX - 1);
    s->sect_idx = sect;
    s->value    = val;
    s->size     = size;
    s->bind     = bind;
    s->type     = type;
    return (int)(obj->hdr.sym_count - 1);
}

int uiox_object_add_reloc(uiox_object_t *obj,
                           unsigned long long offset,
                           unsigned int sym_idx,
                           uiox_reloc_type_t type,
                           long long addend,
                           unsigned int sect_idx)
{
    if (obj->hdr.reloc_count >= UIOX_OBJ_MAX_RELOCS) return -1;
    uiox_obj_reloc_t *r = &obj->relocs[obj->hdr.reloc_count++];
    r->offset   = offset;
    r->sym_idx  = sym_idx;
    r->type     = type;
    r->addend   = addend;
    r->sect_idx = sect_idx;
    return (int)(obj->hdr.reloc_count - 1);
}

int uiox_object_write(const uiox_object_t *obj, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(&obj->hdr, sizeof(obj->hdr), 1, f);
    for (unsigned int i = 0; i < obj->hdr.sect_count; i++) {
        uiox_section_t *s = obj->sects[i];
        fwrite(s->name,  UIOX_SECT_NAME_MAX, 1, f);
        fwrite(&s->type,  sizeof(s->type),  1, f);
        fwrite(&s->flags, sizeof(s->flags), 1, f);
        fwrite(&s->align, sizeof(s->align), 1, f);
        fwrite(&s->size,  sizeof(s->size),  1, f);
        if (s->type != SECT_BSS)
            fwrite(s->data, 1, s->size, f);
    }
    fwrite(obj->syms,   sizeof(uiox_obj_sym_t),
           obj->hdr.sym_count, f);
    fwrite(obj->relocs, sizeof(uiox_obj_reloc_t),
           obj->hdr.reloc_count, f);
    fclose(f);
    return 0;
}

void uiox_object_print(const uiox_object_t *obj)
{
    printf("UIOX Object: %s  arch=%u  sects=%u  syms=%u  relocs=%u\n",
           obj->hdr.source_file, obj->hdr.arch,
           obj->hdr.sect_count, obj->hdr.sym_count,
           obj->hdr.reloc_count);
    for (unsigned int i = 0; i < obj->hdr.sym_count; i++) {
        const uiox_obj_sym_t *s = &obj->syms[i];
        printf("  SYM %-32s  val=0x%llx  sz=%llu  bind=%d  type=%d\n",
               s->name, s->value, s->size, s->bind, s->type);
    }
}
