/*
 * uiox_ld_object.c - UIOX linker object file reader
 */
#include "../include/uiox_ld_object.h"
#include "../include/uiox_ld_elf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Read entire file into buffer ──────────────────────────── */
static uld_u8_t *read_file(const char *path, uld_u64_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    uld_u8_t *buf = (uld_u8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    fclose(f);
    *out_size = (uld_u64_t)sz;
    return buf;
}

/* ── Little-endian read helpers ────────────────────────────── */
static uld_u16_t rd16(const uld_u8_t *p)
{ return (uld_u16_t)(p[0] | (p[1]<<8)); }

static uld_u32_t rd32(const uld_u8_t *p)
{ return (uld_u32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|((uld_u32_t)p[3]<<24)); }

static uld_u64_t rd64(const uld_u8_t *p)
{
    return (uld_u64_t)p[0] | ((uld_u64_t)p[1]<<8)
         | ((uld_u64_t)p[2]<<16) | ((uld_u64_t)p[3]<<24)
         | ((uld_u64_t)p[4]<<32) | ((uld_u64_t)p[5]<<40)
         | ((uld_u64_t)p[6]<<48) | ((uld_u64_t)p[7]<<56);
}

/* ── ELF64 relocatable object reader ───────────────────────── */
int uld_object_read_elf(uld_object_t *obj, uld_diag_ctx_t *diag)
{
    uld_u8_t *raw  = obj->raw;
    uld_u64_t rsz  = obj->raw_size;

    if (rsz < 64) {
        ULD_ERR(diag, obj->path, 0, "ELF file too small");
        return -1;
    }

    /* ELF class */
    uld_u8_t elfclass = raw[4];

    if (elfclass == 2) {
        /* ── ELF64 ── */
        uld_u64_t shoff     = rd64(raw + 40);
        uld_u16_t shentsize = rd16(raw + 58);
        uld_u16_t shnum     = rd16(raw + 60);
        uld_u16_t shstrndx  = rd16(raw + 62);

        if (shoff + (uld_u64_t)shnum * shentsize > rsz) {
            ULD_ERR(diag, obj->path, 0, "ELF64 section table out of range");
            return -1;
        }

        /* allocate sections */
        obj->sects = (uld_obj_sect_t *)calloc(shnum, sizeof(uld_obj_sect_t));
        obj->sect_count = shnum;

        /* string table for section names */
        uld_u8_t *shstrtab = NULL;
        if (shstrndx < shnum) {
            uld_u8_t *sh = raw + shoff + (uld_u64_t)shstrndx * shentsize;
            uld_u64_t str_off = rd64(sh + 24);
            uld_u64_t str_sz  = rd64(sh + 32);
            if (str_off + str_sz <= rsz)
                shstrtab = raw + str_off;
        }

        /* read section headers */
        uld_u32_t sym_shidx  = 0;
        uld_u32_t str_shidx  = 0;
        for (uld_u16_t i = 0; i < shnum; i++) {
            uld_u8_t *sh   = raw + shoff + (uld_u64_t)i * shentsize;
            uld_u32_t name_off = rd32(sh + 0);
            uld_u32_t sh_type  = rd32(sh + 4);
            uld_u64_t sh_off   = rd64(sh + 24);
            uld_u64_t sh_size  = rd64(sh + 32);
            uld_u32_t sh_align = (uld_u32_t)rd64(sh + 48);

            uld_obj_sect_t *s = &obj->sects[i];
            if (shstrtab && (name_off < rsz))
                strncpy(s->name, (char*)(shstrtab + name_off),
                        ULD_NAME_MAX - 1);

            if (sh_type == SHT_SYMTAB) { sym_shidx = i; }
            if (sh_type == SHT_STRTAB && i != shstrndx)
                str_shidx = i;

            s->align = sh_align ? sh_align : 1;
            s->size  = (uld_u32_t)sh_size;
            if (sh_type != SHT_NOBITS && sh_off + sh_size <= rsz && sh_size) {
                s->data = (uld_u8_t *)malloc((size_t)sh_size);
                if (s->data)
                    memcpy(s->data, raw + sh_off, (size_t)sh_size);
            }
            /* map ELF type to internal type */
            if      (sh_type == SHT_NOBITS)   s->type = ULD_ST_BSS;
            else if (sh_type == SHT_SYMTAB)   s->type = ULD_ST_SYMTAB;
            else if (sh_type == SHT_STRTAB)   s->type = ULD_ST_STRTAB;
            else if (sh_type == SHT_RELA ||
                     sh_type == SHT_REL)       s->type = ULD_ST_RELOC;
            else if (sh_type == SHT_PROGBITS) {
                uld_u64_t sh_flags = rd64(sh + 8);
                if (sh_flags & SHF_EXECINSTR)  s->type = ULD_ST_TEXT;
                else if (sh_flags & SHF_WRITE)  s->type = ULD_ST_DATA;
                else                            s->type = ULD_ST_RODATA;
            }
        }

        /* read symbol table */
        if (sym_shidx) {
            uld_u8_t *sym_sh  = raw + shoff + (uld_u64_t)sym_shidx * shentsize;
            uld_u64_t sym_off = rd64(sym_sh + 24);
            uld_u64_t sym_sz  = rd64(sym_sh + 32);
            uld_u32_t sym_cnt = (uld_u32_t)(sym_sz / 24);
            obj->syms = (uld_obj_sym_t *)calloc(sym_cnt, sizeof(uld_obj_sym_t));
            obj->sym_count = sym_cnt;

            /* strtab for symbol names */
            uld_u8_t *strtab = NULL;
            if (str_shidx && str_shidx < shnum) {
                uld_u8_t *str_sh = raw + shoff +
                                    (uld_u64_t)str_shidx * shentsize;
                uld_u64_t soff = rd64(str_sh + 24);
                if (soff < rsz) strtab = raw + soff;
            }

            for (uld_u32_t i = 0; i < sym_cnt; i++) {
                uld_u8_t *se = raw + sym_off + (uld_u64_t)i * 24;
                uld_u32_t noff = rd32(se + 0);
                uld_u8_t  info = se[4];
                uld_u16_t shndx= rd16(se + 6);
                uld_u64_t val  = rd64(se + 8);
                uld_u64_t sz   = rd64(se + 16);
                uld_obj_sym_t *sym = &obj->syms[i];
                if (strtab) strncpy(sym->name, (char*)(strtab + noff),
                                    ULD_NAME_MAX - 1);
                sym->value    = val;
                sym->size     = sz;
                sym->sect_idx = shndx;
                sym->bind = (info >> 4) == 1 ? ULD_BIND_GLOBAL
                           : (info >> 4) == 2 ? ULD_BIND_WEAK
                           : ULD_BIND_LOCAL;
                sym->type = (uld_sym_type_t)(info & 0xF);
            }
        }
    } else {
        /* ELF32 — simplified: same structure, 32-bit fields */
        ULD_WARN(diag, obj->path, 0,
                 "ELF32 object reading is simplified");
    }
    return 0;
}

/* ── UIOX native .uobj reader ──────────────────────────────── */
int uld_object_read_uobj(uld_object_t *obj, uld_diag_ctx_t *diag)
{
    /* UIOX object format mirrors the internal structures exactly */
    (void)diag;
    /* For now treat as raw binary — real impl would parse header */
    return 0;
}

int uld_object_load(uld_object_t *obj, const char *path,
                     uld_diag_ctx_t *diag)
{
    memset(obj, 0, sizeof(*obj));
    strncpy(obj->path, path, ULD_PATH_MAX - 1);

    obj->raw = read_file(path, &obj->raw_size);
    if (!obj->raw) {
        ULD_ERR(diag, path, 0, "cannot open object file: %s", path);
        return -1;
    }

    /* detect format by magic */
    if (obj->raw_size >= 4 &&
        obj->raw[0] == 0x7F && obj->raw[1] == 'E' &&
        obj->raw[2] == 'L'  && obj->raw[3] == 'F') {
        obj->format = ULD_OBJ_ELF;
        return uld_object_read_elf(obj, diag);
    } else if (obj->raw_size >= 4 && rd32(obj->raw) == ULD_OBJ_MAGIC_UOBJ) {
        obj->format = ULD_OBJ_UOBJ;
        return uld_object_read_uobj(obj, diag);
    }

    ULD_ERR(diag, path, 0, "unknown object format: %s", path);
    return -1;
}

void uld_object_free(uld_object_t *obj)
{
    if (!obj) return;
    for (uld_u32_t i = 0; i < obj->sect_count; i++)
        free(obj->sects[i].data);
    free(obj->sects);
    free(obj->syms);
    free(obj->relocs);
    free(obj->raw);
    memset(obj, 0, sizeof(*obj));
}

void uld_object_print(const uld_object_t *obj)
{
    printf("Object: %s  sects=%u  syms=%u  relocs=%u\n",
           obj->path, obj->sect_count,
           obj->sym_count, obj->reloc_count);
}
