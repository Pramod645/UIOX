/*
 * uiox_linker.c - UIOX linker implementation
 */
#include "../include/uiox_linker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void uiox_linker_init(uiox_linker_t *lnk, uiox_diag_ctx_t *diag,
                       uiox_target_arch_t arch, uiox_lnk_output_t fmt)
{
    memset(lnk, 0, sizeof(*lnk));
    lnk->diag          = diag;
    lnk->arch          = arch;
    lnk->output_format = fmt;
    strncpy(lnk->entry_sym, "_start", sizeof(lnk->entry_sym) - 1);
}

void uiox_linker_free(uiox_linker_t *lnk)
{
    for (int i = 0; i < lnk->sect_count; i++)
        free(lnk->sects[i].data);
}

int uiox_linker_add_object(uiox_linker_t *lnk, uiox_object_t *obj)
{
    if (lnk->obj_count >= UIOX_LNK_MAX_OBJS) return -1;
    lnk->objs[lnk->obj_count++] = obj;
    return 0;
}

int uiox_linker_set_entry(uiox_linker_t *lnk, const char *sym)
{
    strncpy(lnk->entry_sym, sym, sizeof(lnk->entry_sym) - 1);
    return 0;
}

int uiox_linker_set_output(uiox_linker_t *lnk, const char *path)
{
    strncpy(lnk->output_path, path, sizeof(lnk->output_path) - 1);
    return 0;
}

/* -- Pass 1: collect all global symbols --------------------- */
int uiox_lnk_pass_collect(uiox_linker_t *lnk)
{
    for (int oi = 0; oi < lnk->obj_count; oi++) {
        uiox_object_t *obj = lnk->objs[oi];
        for (unsigned int si = 0; si < obj->hdr.sym_count; si++) {
            uiox_obj_sym_t *s = &obj->syms[si];
            if (s->bind == UIOX_BIND_LOCAL) continue;
            /* check for duplicate */
            int found = 0;
            for (int gi = 0; gi < lnk->sym_count; gi++) {
                if (strcmp(lnk->syms[gi].name, s->name) == 0) {
                    if (lnk->syms[gi].defined && s->type != UIOX_SYM_NOTYPE) {
                        UIOX_ERR(lnk->diag, obj->hdr.source_file,
                                 0, 0,
                                 "duplicate symbol: %s", s->name);
                    }
                    found = 1; break;
                }
            }
            if (!found && lnk->sym_count < UIOX_LNK_MAX_SYMS) {
                uiox_lnk_sym_t *gs = &lnk->syms[lnk->sym_count++];
                strncpy(gs->name, s->name, UIOX_SYM_NAME_MAX - 1);
                gs->bind    = s->bind;
                gs->type    = s->type;
                gs->size    = s->size;
                gs->obj_idx = oi;
                gs->defined = (s->sect_idx != 0);
            }
        }
    }
    return uiox_diag_has_error(lnk->diag) ? -1 : 0;
}

/* -- Pass 2: merge sections from all objects ---------------- */
int uiox_lnk_pass_merge(uiox_linker_t *lnk)
{
    const char *sect_order[] = {
        ".text", ".rodata", ".data", ".bss", NULL
    };
    for (int si = 0; sect_order[si]; si++) {
        uiox_lnk_sect_t *out = &lnk->sects[lnk->sect_count++];
        strncpy(out->name, sect_order[si], UIOX_SECT_NAME_MAX - 1);
        out->cap  = 65536;
        out->data = (unsigned char *)malloc(out->cap);
        out->size = 0;
        for (int oi = 0; oi < lnk->obj_count; oi++) {
            uiox_object_t *obj = lnk->objs[oi];
            for (unsigned int i = 0; i < obj->hdr.sect_count; i++) {
                uiox_section_t *src = obj->sects[i];
                if (!src || strcmp(src->name, sect_order[si]) != 0) continue;
                if (out->size + src->size > out->cap) {
                    out->cap  = (out->size + src->size) * 2;
                    out->data = (unsigned char *)realloc(out->data, out->cap);
                }
                if (src->type != SECT_BSS)
                    memcpy(out->data + out->size, src->data, src->size);
                else
                    memset(out->data + out->size, 0, src->size);
                out->size += src->size;
            }
        }
    }
    return 0;
}

/* -- Pass 3: assign virtual addresses ----------------------- */
int uiox_lnk_pass_layout(uiox_linker_t *lnk)
{
    unsigned long long vaddr = 0x00100000ULL; /* default load addr */
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        /* align to 4 KB */
        vaddr = (vaddr + 0xFFF) & ~0xFFFULL;
        s->vaddr = vaddr;
        vaddr   += s->size;
    }
    return 0;
}

/* -- Pass 4: resolve symbol addresses ----------------------- */
int uiox_lnk_pass_resolve(uiox_linker_t *lnk)
{
    for (int gi = 0; gi < lnk->sym_count; gi++) {
        uiox_lnk_sym_t *gs = &lnk->syms[gi];
        if (!gs->defined) {
            UIOX_ERR(lnk->diag, "<linker>", 0, 0,
                     "undefined symbol: %s", gs->name);
            continue;
        }
        uiox_object_t  *obj = lnk->objs[gs->obj_idx];
        uiox_obj_sym_t *s   = NULL;
        for (unsigned int si = 0; si < obj->hdr.sym_count; si++) {
            if (strcmp(obj->syms[si].name, gs->name) == 0) {
                s = &obj->syms[si]; break;
            }
        }
        if (!s) continue;
        if (gs->sect_idx < (unsigned int)lnk->sect_count)
            gs->value = lnk->sects[gs->sect_idx].vaddr + s->value;
        /* resolve entry point */
        if (strcmp(gs->name, lnk->entry_sym) == 0)
            lnk->entry_addr = gs->value;
    }
    return uiox_diag_has_error(lnk->diag) ? -1 : 0;
}

/* -- Pass 5: apply relocations ------------------------------ */
int uiox_lnk_pass_relocate(uiox_linker_t *lnk)
{
    for (int oi = 0; oi < lnk->obj_count; oi++) {
        uiox_object_t *obj = lnk->objs[oi];
        for (unsigned int ri = 0; ri < obj->hdr.reloc_count; ri++) {
            uiox_obj_reloc_t *r = &obj->relocs[ri];

            /* find the symbol value */
            unsigned long long sym_val = 0;
            if (r->sym_idx < obj->hdr.sym_count) {
                const char *sname = obj->syms[r->sym_idx].name;
                for (int gi = 0; gi < lnk->sym_count; gi++) {
                    if (strcmp(lnk->syms[gi].name, sname) == 0) {
                        sym_val = lnk->syms[gi].value;
                        break;
                    }
                }
            }

            /* find the output section to patch */
            if (r->sect_idx >= (unsigned int)lnk->sect_count) continue;
            uiox_lnk_sect_t *sect = &lnk->sects[r->sect_idx];
            unsigned long long patch_vma = sect->vaddr + r->offset;
            unsigned int       off       = (unsigned int)r->offset;

            if (off + 8 > sect->size) continue;

            long long S = (long long)sym_val;
            long long A = r->addend;
            long long P = (long long)patch_vma;

            switch (r->type) {
                case UIOX_RELOC_ABS64: {
                    unsigned long long v = (unsigned long long)(S + A);
                    memcpy(sect->data + off, &v, 8);
                    break;
                }
                case UIOX_RELOC_ABS32: {
                    unsigned int v = (unsigned int)(S + A);
                    memcpy(sect->data + off, &v, 4);
                    break;
                }
                case UIOX_RELOC_REL32: {
                    int v = (int)(S + A - P);
                    memcpy(sect->data + off, &v, 4);
                    break;
                }
                case UIOX_RELOC_REL64: {
                    long long v = S + A - P;
                    memcpy(sect->data + off, &v, 8);
                    break;
                }
                case UIOX_RELOC_ARM_B26: {
                    /* ARM B/BL: encode 24-bit word offset in [23:0] */
                    long long off26 = (S + A - P) / 4;
                    unsigned int instr;
                    memcpy(&instr, sect->data + off, 4);
                    instr = (instr & 0xFF000000u) |
                            ((unsigned int)(off26) & 0x00FFFFFFu);
                    memcpy(sect->data + off, &instr, 4);
                    break;
                }
                case UIOX_RELOC_A64_CALL: {
                    /* AArch64 BL: encode 26-bit word offset in [25:0] */
                    long long off26 = (S + A - P) / 4;
                    unsigned int instr;
                    memcpy(&instr, sect->data + off, 4);
                    instr = (instr & 0xFC000000u) |
                            ((unsigned int)(off26) & 0x03FFFFFFu);
                    memcpy(sect->data + off, &instr, 4);
                    break;
                }
                default:
                    break;
            }
        }
    }
    return 0;
}

/* -- Pass 6: emit output file ------------------------------- */
int uiox_lnk_pass_emit(uiox_linker_t *lnk)
{
    switch (lnk->output_format) {
        case UIOX_LNK_ELF64: return uiox_lnk_emit_elf64(lnk);
        case UIOX_LNK_ELF32: return uiox_lnk_emit_elf32(lnk);
        case UIOX_LNK_FLAT:  return uiox_lnk_emit_flat(lnk);
        case UIOX_LNK_IHEX:  return uiox_lnk_emit_ihex(lnk);
        default:             return -1;
    }
}

/* -- ELF64 emission ----------------------------------------- */
int uiox_lnk_emit_elf64(uiox_linker_t *lnk)
{
    FILE *f = fopen(lnk->output_path, "wb");
    if (!f) {
        UIOX_ERR(lnk->diag, "<linker>", 0, 0,
                 "cannot open output: %s", lnk->output_path);
        return -1;
    }

    /* ELF64 header (64 bytes) */
    unsigned char ehdr[64];
    memset(ehdr, 0, sizeof(ehdr));
    /* e_ident */
    ehdr[0]  = 0x7F; ehdr[1] = 'E'; ehdr[2] = 'L'; ehdr[3] = 'F';
    ehdr[4]  = 2;    /* ELFCLASS64    */
    ehdr[5]  = 1;    /* ELFDATA2LSB   */
    ehdr[6]  = 1;    /* EV_CURRENT    */
    ehdr[7]  = 0;    /* ELFOSABI_NONE */
    /* e_type = ET_EXEC (2) */
    ehdr[16] = 2; ehdr[17] = 0;
    /* e_machine */
    unsigned short mach = (lnk->arch == UIOX_TARGET_ARM64) ? 183  /* EM_AARCH64 */
                        : (lnk->arch == UIOX_TARGET_ARM32) ? 40   /* EM_ARM     */
                        : 62;                                       /* EM_X86_64  */
    memcpy(ehdr + 18, &mach, 2);
    /* e_version = 1 */
    ehdr[20] = 1;
    /* e_entry (8 bytes at offset 24) */
    memcpy(ehdr + 24, &lnk->entry_addr, 8);
    /* e_phoff = 64 (program headers follow ELF header) */
    unsigned long long phoff = 64ULL;
    memcpy(ehdr + 32, &phoff, 8);
    /* e_shoff = 0 (no section headers for flat exec) */
    /* e_flags = 0 */
    /* e_ehsize = 64 */
    unsigned short ehsize = 64;
    memcpy(ehdr + 52, &ehsize, 2);
    /* e_phentsize = 56 */
    unsigned short phentsize = 56;
    memcpy(ehdr + 54, &phentsize, 2);
    /* e_phnum = sect_count */
    unsigned short phnum = (unsigned short)lnk->sect_count;
    memcpy(ehdr + 56, &phnum, 2);

    fwrite(ehdr, 1, 64, f);

    /* Calculate file offsets for each section */
    unsigned long long data_off =
        64ULL + (unsigned long long)lnk->sect_count * 56ULL;

    /* Program headers (56 bytes each) */
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        unsigned char phdr[56];
        memset(phdr, 0, sizeof(phdr));
        /* p_type = PT_LOAD (1) */
        unsigned int ptype = 1;
        memcpy(phdr + 0, &ptype, 4);
        /* p_flags: text=RX(5), data=RW(6), bss=RW(6) */
        unsigned int pflags = (strcmp(s->name, ".text") == 0) ? 5u : 6u;
        memcpy(phdr + 4, &pflags, 4);
        /* p_offset */
        memcpy(phdr + 8,  &data_off,  8);
        /* p_vaddr */
        memcpy(phdr + 16, &s->vaddr,  8);
        /* p_paddr = p_vaddr */
        memcpy(phdr + 24, &s->vaddr,  8);
        /* p_filesz */
        unsigned long long fsz = (strcmp(s->name, ".bss") == 0) ? 0 : s->size;
        memcpy(phdr + 32, &fsz,       8);
        /* p_memsz */
        unsigned long long msz = s->size;
        memcpy(phdr + 40, &msz,       8);
        /* p_align = 4096 */
        unsigned long long palign = 4096ULL;
        memcpy(phdr + 48, &palign,    8);

        fwrite(phdr, 1, 56, f);
        s->file_off = data_off;
        if (strcmp(s->name, ".bss") != 0) data_off += s->size;
    }

    /* Section data */
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        if (strcmp(s->name, ".bss") == 0) continue;
        fwrite(s->data, 1, s->size, f);
    }

    fclose(f);
    printf("[linker] ELF64 written: %s  entry=0x%llx  sects=%d\n",
           lnk->output_path, lnk->entry_addr, lnk->sect_count);
    return 0;
}

/* -- ELF32 emission ----------------------------------------- */
int uiox_lnk_emit_elf32(uiox_linker_t *lnk)
{
    FILE *f = fopen(lnk->output_path, "wb");
    if (!f) return -1;

    unsigned char ehdr[52];
    memset(ehdr, 0, sizeof(ehdr));
    ehdr[0]=0x7F; ehdr[1]='E'; ehdr[2]='L'; ehdr[3]='F';
    ehdr[4]=1; /* ELFCLASS32 */
    ehdr[5]=1; /* LSB        */
    ehdr[6]=1; /* EV_CURRENT */
    unsigned short mach = 40; /* EM_ARM */
    memcpy(ehdr+18, &mach, 2);
    ehdr[20]=1;
    unsigned int entry32 = (unsigned int)lnk->entry_addr;
    memcpy(ehdr+24, &entry32, 4);
    unsigned int phoff32 = 52;
    memcpy(ehdr+28, &phoff32, 4);
    unsigned short ehsz32 = 52, phent32 = 32;
    unsigned short phnum32 = (unsigned short)lnk->sect_count;
    memcpy(ehdr+40, &ehsz32,  2);
    memcpy(ehdr+42, &phent32, 2);
    memcpy(ehdr+44, &phnum32, 2);
    fwrite(ehdr, 1, 52, f);

    unsigned int data_off32 = 52u + (unsigned int)lnk->sect_count * 32u;
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        unsigned char phdr[32];
        memset(phdr, 0, 32);
        unsigned int ptype = 1; memcpy(phdr+0,  &ptype,    4);
        memcpy(phdr+4,  &data_off32,             4);
        unsigned int va32 = (unsigned int)s->vaddr;
        memcpy(phdr+8,  &va32, 4);
        memcpy(phdr+12, &va32, 4);
        memcpy(phdr+16, &s->size, 4);
        memcpy(phdr+20, &s->size, 4);
        unsigned int pflags = (strcmp(s->name,".text")==0)?5u:6u;
        memcpy(phdr+24, &pflags, 4);
        unsigned int palign = 4096;
        memcpy(phdr+28, &palign, 4);
        fwrite(phdr, 1, 32, f);
        s->file_off = data_off32;
        if (strcmp(s->name, ".bss") != 0) data_off32 += s->size;
    }
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        if (strcmp(s->name, ".bss") == 0) continue;
        fwrite(s->data, 1, s->size, f);
    }
    fclose(f);
    printf("[linker] ELF32 written: %s\n", lnk->output_path);
    return 0;
}

/* -- Flat binary emission ----------------------------------- */
int uiox_lnk_emit_flat(uiox_linker_t *lnk)
{
    FILE *f = fopen(lnk->output_path, "wb");
    if (!f) return -1;
    for (int i = 0; i < lnk->sect_count; i++) {
        uiox_lnk_sect_t *s = &lnk->sects[i];
        if (strcmp(s->name, ".bss") == 0) continue;
        fwrite(s->data, 1, s->size, f);
    }
    fclose(f);
    printf("[linker] Flat binary written: %s\n", lnk->output_path);
    return 0;
}

/* -- Intel HEX emission ------------------------------------- */
int uiox_lnk_emit_ihex(uiox_linker_t *lnk)
{
    FILE *f = fopen(lnk->output_path, "w");
    if (!f) return -1;
    for (int si = 0; si < lnk->sect_count; si++) {
        uiox_lnk_sect_t *s = &lnk->sects[si];
        if (strcmp(s->name, ".bss") == 0) continue;
        unsigned int addr = (unsigned int)s->vaddr;
        for (unsigned int i = 0; i < s->size; ) {
            unsigned int chunk = s->size - i;
            if (chunk > 16) chunk = 16;
            unsigned char sum = 0;
            fprintf(f, ":%02X%04X00", chunk, addr & 0xFFFF);
            sum += (unsigned char)chunk;
            sum += (unsigned char)((addr >> 8) & 0xFF);
            sum += (unsigned char)(addr & 0xFF);
            for (unsigned int j = 0; j < chunk; j++) {
                fprintf(f, "%02X", s->data[i+j]);
                sum += s->data[i+j];
            }
            fprintf(f, "%02X\n", (unsigned char)(~sum + 1));
            i    += chunk;
            addr += chunk;
        }
    }
    fprintf(f, ":00000001FF\n");
    fclose(f);
    printf("[linker] Intel HEX written: %s\n", lnk->output_path);
    return 0;
}

/* -- Top-level linker run ----------------------------------- */
int uiox_linker_run(uiox_linker_t *lnk)
{
    if (uiox_lnk_pass_collect (lnk) < 0) return -1;
    if (uiox_lnk_pass_merge   (lnk) < 0) return -1;
    if (uiox_lnk_pass_layout  (lnk) < 0) return -1;
    if (uiox_lnk_pass_resolve (lnk) < 0) return -1;
    if (uiox_lnk_pass_relocate(lnk) < 0) return -1;
    if (uiox_lnk_pass_emit    (lnk) < 0) return -1;
    return 0;
}
