/*
 * uiox_ld_archive.c - UIOX static archive (.a) reader
 */
#include "../include/uiox_ld_archive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uld_u8_t *read_file(const char *path, uld_u64_t *sz)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long s = ftell(f); fseek(f, 0, SEEK_SET);
    if (s <= 0) { fclose(f); return NULL; }
    uld_u8_t *b = (uld_u8_t *)malloc((size_t)s);
    if (!b) { fclose(f); return NULL; }
    fread(b, 1, (size_t)s, f);
    fclose(f);
    *sz = (uld_u64_t)s;
    return b;
}

static uld_u64_t ar_size_parse(const char *s)
{
    uld_u64_t v = 0;
    while (*s == ' ') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

int uld_archive_open(uld_archive_t *ar, const char *path,
                      uld_diag_ctx_t *diag)
{
    memset(ar, 0, sizeof(*ar));
    strncpy(ar->path, path, ULD_PATH_MAX - 1);

    ar->raw = read_file(path, &ar->raw_size);
    if (!ar->raw) {
        ULD_ERR(diag, path, 0, "cannot open archive: %s", path);
        return -1;
    }

    if (ar->raw_size < ULD_AR_MAGIC_LEN ||
        memcmp(ar->raw, ULD_AR_MAGIC, ULD_AR_MAGIC_LEN) != 0) {
        ULD_ERR(diag, path, 0, "not a valid ar archive: %s", path);
        return -1;
    }

    uld_u64_t pos = ULD_AR_MAGIC_LEN;

    /* scan members */
    while (pos + ULD_AR_HDR_SIZE <= ar->raw_size) {
        uld_ar_hdr_t *hdr = (uld_ar_hdr_t *)(ar->raw + pos);

        /* verify end magic */
        if (hdr->fmag[0] != '`' || hdr->fmag[1] != '\n') break;

        uld_u64_t member_size = ar_size_parse(hdr->size);
        uld_u64_t data_off    = pos + ULD_AR_HDR_SIZE;

        /* strip trailing spaces from name */
        char name[17] = {0};
        strncpy(name, hdr->name, 16);
        for (int i = 15; i >= 0 && name[i] == ' '; i--) name[i] = '\0';

        /* skip special members (symbol table, long name table) */
        if (name[0] != '/' && ar->member_count < 4096) {
            uld_ar_member_t *m =
                (uld_ar_member_t *)calloc(1, sizeof(*m));
            strncpy(m->name, name, ULD_NAME_MAX - 1);
            m->offset = data_off;
            m->size   = member_size;
            m->next   = NULL;
            if (!ar->members) ar->members = m;
            else {
                uld_ar_member_t *cur = ar->members;
                while (cur->next) cur = cur->next;
                cur->next = m;
            }
            ar->member_count++;
        }

        pos = data_off + member_size;
        if (member_size & 1) pos++; /* ar pads to even boundary */
    }

    return 0;
}

void uld_archive_free(uld_archive_t *ar)
{
    uld_ar_member_t *m = ar->members;
    while (m) {
        uld_ar_member_t *nx = m->next;
        free(m);
        m = nx;
    }
    free(ar->raw);
    if (ar->sym_names) {
        for (uld_u32_t i = 0; i < ar->sym_count; i++)
            free(ar->sym_names[i]);
        free(ar->sym_names);
    }
    free(ar->sym_offsets);
    memset(ar, 0, sizeof(*ar));
}

int uld_archive_extract_all(uld_archive_t *ar,
                              uld_object_t *objs,
                              uld_u32_t *obj_count,
                              uld_u32_t max_objs,
                              uld_diag_ctx_t *diag)
{
    uld_ar_member_t *m = ar->members;
    while (m && *obj_count < max_objs) {
        uld_object_t *obj = &objs[*obj_count];
        memset(obj, 0, sizeof(*obj));
        strncpy(obj->path, m->name, ULD_PATH_MAX - 1);
        obj->raw_size = m->size;
        obj->raw = (uld_u8_t *)malloc((size_t)m->size);
        if (!obj->raw) { m = m->next; continue; }
        memcpy(obj->raw, ar->raw + m->offset, (size_t)m->size);
        /* detect and parse format */
        if (m->size >= 4 &&
            obj->raw[0] == 0x7F && obj->raw[1] == 'E' &&
            obj->raw[2] == 'L'  && obj->raw[3] == 'F') {
            obj->format = ULD_OBJ_ELF;
            uld_object_read_elf(obj, diag);
        }
        obj->idx = *obj_count;
        (*obj_count)++;
        m = m->next;
    }
    return 0;
}

int uld_archive_extract_sym(uld_archive_t *ar,
                              const char *sym_name,
                              uld_object_t *out_obj,
                              uld_diag_ctx_t *diag)
{
    /* search symbol index */
    for (uld_u32_t i = 0; i < ar->sym_count; i++) {
        if (ar->sym_names[i] &&
            strcmp(ar->sym_names[i], sym_name) == 0) {
            uld_u64_t off = ar->sym_offsets[i];
            uld_ar_hdr_t *hdr = (uld_ar_hdr_t *)(ar->raw + off);
            uld_u64_t size = 0;
            char sbuf[11]; memcpy(sbuf, hdr->size, 10); sbuf[10]=0;
            size = (uld_u64_t)atoll(sbuf);
            out_obj->raw = (uld_u8_t *)malloc((size_t)size);
            if (!out_obj->raw) return -1;
            memcpy(out_obj->raw, ar->raw + off + ULD_AR_HDR_SIZE,
                   (size_t)size);
            out_obj->raw_size = size;
            out_obj->format = ULD_OBJ_ELF;
            return uld_object_read_elf(out_obj, diag);
        }
    }
    return -1;
}
