/*
 * uiox_ld_script.c - UIOX linker script parser
 */
#include "../include/uiox_ld_script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Default linker scripts ─────────────────────────────────── */
const char *uld_script_default_x86_64(void)
{
    return
        "ENTRY(_start)\n"
        "MEMORY {\n"
        "  KERN (rwx) : ORIGIN = 0x0000000000100000, LENGTH = 63M\n"
        "  STACK (rw) : ORIGIN = 0x0000000003F00000, LENGTH = 1M\n"
        "}\n"
        "SECTIONS {\n"
        "  .text   : { *(.text*)   *(.rodata*) } > KERN\n"
        "  .data   : { *(.data*)               } > KERN\n"
        "  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > KERN\n"
        "}\n";
}

const char *uld_script_default_arm64(void)
{
    return
        "ENTRY(_start)\n"
        "MEMORY {\n"
        "  DRAM (rwx) : ORIGIN = 0x0000000040000000, LENGTH = 64M\n"
        "  STACK (rw) : ORIGIN = 0x0000000043F00000, LENGTH = 1M\n"
        "}\n"
        "SECTIONS {\n"
        "  .vectors : { *(.vectors) } > DRAM\n"
        "  .text    : { *(.text*)  *(.rodata*) } > DRAM\n"
        "  .data    : { *(.data*)              } > DRAM\n"
        "  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > DRAM\n"
        "}\n";
}

const char *uld_script_default_arm32(void)
{
    return
        "ENTRY(_start)\n"
        "MEMORY {\n"
        "  ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 1M\n"
        "  RAM (rwx) : ORIGIN = 0x00100000, LENGTH = 15M\n"
        "}\n"
        "SECTIONS {\n"
        "  .vectors : { *(.vectors) } > ROM\n"
        "  .text    : { *(.text*)  *(.rodata*) } > RAM\n"
        "  .data    : { *(.data*)              } > RAM\n"
        "  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > RAM\n"
        "}\n";
}

/* ── Minimal script tokeniser ──────────────────────────────── */
typedef struct {
    const char *src;
    int         pos;
    int         len;
} sc_lex_t;

static void sc_skip(sc_lex_t *l)
{
    while (l->pos < l->len) {
        if (isspace(l->src[l->pos])) { l->pos++; continue; }
        /* C-style comment */
        if (l->pos+1 < l->len &&
            l->src[l->pos]=='/' && l->src[l->pos+1]=='*') {
            l->pos += 2;
            while (l->pos+1 < l->len) {
                if (l->src[l->pos]=='*' && l->src[l->pos+1]=='/') {
                    l->pos += 2; break;
                }
                l->pos++;
            }
            continue;
        }
        break;
    }
}

static int sc_token(sc_lex_t *l, char *out, int maxlen)
{
    sc_skip(l);
    if (l->pos >= l->len) return 0;
    char c = l->src[l->pos];
    /* single-char tokens */
    if (c=='{' || c=='}' || c=='(' || c==')' ||
        c==':' || c=='>' || c==',') {
        out[0] = c; out[1] = '\0';
        l->pos++; return 1;
    }
    /* word token */
    int i = 0;
    while (l->pos < l->len && i < maxlen - 1) {
        char ch = l->src[l->pos];
        if (isspace(ch) || ch=='{' || ch=='}' ||
            ch=='(' || ch==')' || ch==':' ||
            ch=='>' || ch==',') break;
        out[i++] = ch;
        l->pos++;
    }
    out[i] = '\0';
    return i > 0;
}

static uld_addr_t parse_addr(const char *s)
{
    if (s[0]=='0' && (s[1]=='x' || s[1]=='X'))
        return (uld_addr_t)strtoull(s+2, NULL, 16);
    /* handle size suffixes K/M/G */
    uld_u64_t v = strtoull(s, NULL, 10);
    int last = (int)strlen(s) - 1;
    if (last >= 0) {
        if (s[last]=='K' || s[last]=='k') v *= 1024ULL;
        if (s[last]=='M' || s[last]=='m') v *= 1024ULL * 1024;
        if (s[last]=='G' || s[last]=='g') v *= 1024ULL * 1024 * 1024;
    }
    return (uld_addr_t)v;
}

int uld_script_parse_str(uld_script_t *sc, const char *text,
                           uld_diag_ctx_t *diag)
{
    memset(sc, 0, sizeof(*sc));
    strncpy(sc->entry_sym, "_start", ULD_NAME_MAX - 1);

    sc_lex_t l;
    l.src = text;
    l.pos = 0;
    l.len = (int)strlen(text);

    char tok[ULD_NAME_MAX];

    while (sc_token(&l, tok, sizeof(tok))) {
        /* ENTRY(sym) */
        if (strcmp(tok, "ENTRY") == 0) {
            sc_token(&l, tok, sizeof(tok)); /* ( */
            sc_token(&l, tok, sizeof(tok)); /* sym */
            strncpy(sc->entry_sym, tok, ULD_NAME_MAX - 1);
            sc_token(&l, tok, sizeof(tok)); /* ) */
            sc->has_entry = ULD_TRUE;
            continue;
        }

        /* MEMORY { ... } */
        if (strcmp(tok, "MEMORY") == 0) {
            sc_token(&l, tok, sizeof(tok)); /* { */
            while (sc_token(&l, tok, sizeof(tok)) &&
                   strcmp(tok, "}") != 0) {
                /* region_name (flags) : ORIGIN = addr, LENGTH = len */
                if (sc->region_count >= ULD_SCRIPT_MAX_REGIONS) break;
                uld_mem_region_t *r = &sc->regions[sc->region_count];
                strncpy(r->name, tok, ULD_NAME_MAX - 1);
                /* skip (flags) */
                sc_token(&l, tok, sizeof(tok)); /* ( */
                sc_token(&l, tok, sizeof(tok)); /* flags */
                sc_token(&l, tok, sizeof(tok)); /* ) */
                sc_token(&l, tok, sizeof(tok)); /* : */
                /* ORIGIN = value */
                sc_token(&l, tok, sizeof(tok)); /* ORIGIN */
                sc_token(&l, tok, sizeof(tok)); /* = */
                sc_token(&l, tok, sizeof(tok)); /* value */
                r->origin = r->current = parse_addr(tok);
                sc_token(&l, tok, sizeof(tok)); /* , */
                /* LENGTH = value */
                sc_token(&l, tok, sizeof(tok)); /* LENGTH */
                sc_token(&l, tok, sizeof(tok)); /* = */
                sc_token(&l, tok, sizeof(tok)); /* value */
                r->length = parse_addr(tok);
                sc->region_count++;
            }
            continue;
        }

        /* SECTIONS { ... } */
        if (strcmp(tok, "SECTIONS") == 0) {
            sc_token(&l, tok, sizeof(tok)); /* { */
            while (sc_token(&l, tok, sizeof(tok)) &&
                   strcmp(tok, "}") != 0) {
                if (sc->sect_cmd_count >= ULD_SCRIPT_MAX_SECTIONS) break;
                uld_sect_cmd_t *cmd =
                    &sc->sect_cmds[sc->sect_cmd_count];
                memset(cmd, 0, sizeof(*cmd));
                strncpy(cmd->out_name, tok, ULD_NAME_MAX - 1);

                /* optional (NOLOAD) */
                sc_token(&l, tok, sizeof(tok));
                if (strcmp(tok, "(") == 0) {
                    sc_token(&l, tok, sizeof(tok)); /* NOLOAD */
                    if (strcmp(tok, "NOLOAD") == 0)
                        cmd->noload = ULD_TRUE;
                    sc_token(&l, tok, sizeof(tok)); /* ) */
                    sc_token(&l, tok, sizeof(tok)); /* : */
                } else {
                    /* tok should be ':' */
                }

                /* { patterns } */
                sc_token(&l, tok, sizeof(tok)); /* { */
                while (sc_token(&l, tok, sizeof(tok)) &&
                       strcmp(tok, "}") != 0) {
                    if (cmd->pattern_count < 16)
                        strncpy(cmd->patterns[cmd->pattern_count++],
                                tok, ULD_NAME_MAX - 1);
                }

                /* optional > REGION */
                sc_skip(&l);
                if (l.pos < l.len && l.src[l.pos] == '>') {
                    sc_token(&l, tok, sizeof(tok)); /* > */
                    sc_token(&l, tok, sizeof(tok)); /* region name */
                    strncpy(cmd->region, tok, ULD_NAME_MAX - 1);
                }

                sc->sect_cmd_count++;
            }
            continue;
        }
    }

    (void)diag;
    return 0;
}

int uld_script_parse(uld_script_t *sc, const char *path,
                      uld_diag_ctx_t *diag)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ULD_ERR(diag, path, 0, "cannot open linker script: %s", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)(sz + 1));
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    int rc = uld_script_parse_str(sc, buf, diag);
    free(buf);
    return rc;
}

void uld_script_free(uld_script_t *sc)
{
    memset(sc, 0, sizeof(*sc));
}

void uld_script_print(const uld_script_t *sc)
{
    printf("Linker script: entry=%s\n", sc->entry_sym);
    for (uld_u32_t i = 0; i < sc->region_count; i++) {
        const uld_mem_region_t *r = &sc->regions[i];
        printf("  MEM  %-12s  origin=0x%llx  length=0x%llx\n",
               r->name,
               (unsigned long long)r->origin,
               (unsigned long long)r->length);
    }
    for (uld_u32_t i = 0; i < sc->sect_cmd_count; i++) {
        const uld_sect_cmd_t *c = &sc->sect_cmds[i];
        printf("  SECT %-16s  region=%-8s  noload=%d\n",
               c->out_name, c->region, c->noload);
    }
}
