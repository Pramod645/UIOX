/*
 * uiox_ld_elf.c - UIOX ELF64 / ELF32 / flat / IHEX / SREC output
 */
#include "../include/uiox_ld_elf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── ELF64 writer ───────────────────────────────────────────── */
int uld_elf64_write(uld_elf_ctx_t *ctx)
{
    FILE *f = fopen(ctx->output_path, "wb");
    if (!f) {
        ULD_ERR(ctx->diag, ctx->output_path, 0,
                "cannot create output file: %s", ctx->output_path);
        return -1;
    }

    uld_u32_t nsects    = ctx->sects->count;
    uld_u16_t e_machine;
    switch (ctx->arch) {
        case ULD_ARCH_ARM64:  e_machine = EM_AARCH64; break;
        case ULD_ARCH_ARM32:  e_machine = EM_ARM;     break;
        default:              e_machine = EM_X86_64;  break;
    }

    /* ELF header */
    uld_elf64_ehdr_t ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0]  = ELF_MAG0;
    ehdr.e_ident[1]  = ELF_MAG1;
    ehdr.e_ident[2]  = ELF_MAG2;
    ehdr.e_ident[3]  = ELF_MAG3;
    ehdr.e_ident[4]  = ELFCLASS64;
    ehdr.e_ident[5]  = ELFDATA2LSB;
    ehdr.e_ident[6]  = 1; /* EV_CURRENT */
    ehdr.e_type      = ET_EXEC;
    ehdr.e_machine   = e_machine;
    ehdr.e_version   = 1;
    ehdr.e_entry     = ctx->entry_addr;
    ehdr.e_phoff     = sizeof(uld_elf64_ehdr_t);
    ehdr.e_ehsize    = sizeof(uld_elf64_ehdr_t);
    ehdr.e_phentsize = sizeof(uld_elf64_phdr_t);
    ehdr.e_phnum     = (uld_u16_t)nsects;
    ehdr.e_shentsize = sizeof(uld_elf64_shdr_t);
    ehdr.e_shnum     = 0;
    ehdr.e_shstrndx  = 0;

    /* calculate file offsets for section data */
    uld_u64_t data_start = sizeof(uld_elf64_ehdr_t) +
                           (uld_u64_t)nsects * sizeof(uld_elf64_phdr_t);
    uld_u64_t cur_off = data_start;

    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        /* align file offset */
        uld_u32_t al = s->align ? s->align : 1;
        uld_u64_t rem = cur_off % al;
        if (rem) cur_off += al - rem;
        s->file_off = cur_off;
        if (s->type != ULD_ST_BSS) cur_off += s->size;
    }

    fwrite(&ehdr, sizeof(ehdr), 1, f);

    /* program headers */
    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        uld_elf64_phdr_t ph;
        memset(&ph, 0, sizeof(ph));
        ph.p_type   = PT_LOAD;
        ph.p_flags  = (s->flags & ULD_SF_EXEC)  ? (PF_R | PF_X)
                    : (s->flags & ULD_SF_WRITE) ? (PF_R | PF_W)
                    : PF_R;
        ph.p_offset = s->file_off;
        ph.p_vaddr  = s->vaddr;
        ph.p_paddr  = s->vaddr;
        ph.p_filesz = (s->type == ULD_ST_BSS) ? 0 : s->size;
        ph.p_memsz  = s->size;
        ph.p_align  = s->align ? s->align : 0x1000;
        fwrite(&ph, sizeof(ph), 1, f);
    }

    /* section data */
    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        if (s->type == ULD_ST_BSS || !s->data || !s->size) continue;
        /* seek to file_off */
        fseek(f, (long)s->file_off, SEEK_SET);
        fwrite(s->data, 1, (size_t)s->size, f);
    }

    fclose(f);
    if (ctx->diag->verbose)
        printf("[uioxld] ELF64 written: %s  entry=0x%llx  segments=%u\n",
               ctx->output_path,
               (unsigned long long)ctx->entry_addr, nsects);
    return 0;
}

/* ── ELF32 writer ───────────────────────────────────────────── */
int uld_elf32_write(uld_elf_ctx_t *ctx)
{
    FILE *f = fopen(ctx->output_path, "wb");
    if (!f) return -1;

    uld_u32_t nsects = ctx->sects->count;
    uld_u16_t e_machine = (ctx->arch == ULD_ARCH_ARM32) ? EM_ARM : EM_386;

    uld_elf32_ehdr_t ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0] = ELF_MAG0; ehdr.e_ident[1] = ELF_MAG1;
    ehdr.e_ident[2] = ELF_MAG2; ehdr.e_ident[3] = ELF_MAG3;
    ehdr.e_ident[4] = ELFCLASS32;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = 1;
    ehdr.e_type      = ET_EXEC;
    ehdr.e_machine   = e_machine;
    ehdr.e_version   = 1;
    ehdr.e_entry     = (uld_u32_t)ctx->entry_addr;
    ehdr.e_phoff     = sizeof(uld_elf32_ehdr_t);
    ehdr.e_ehsize    = sizeof(uld_elf32_ehdr_t);
    ehdr.e_phentsize = sizeof(uld_elf32_phdr_t);
    ehdr.e_phnum     = (uld_u16_t)nsects;
    fwrite(&ehdr, sizeof(ehdr), 1, f);

    uld_u32_t cur_off = sizeof(uld_elf32_ehdr_t) +
                        nsects * sizeof(uld_elf32_phdr_t);
    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        uld_u32_t al  = s->align ? s->align : 1;
        uld_u32_t rem = cur_off % al;
        if (rem) cur_off += al - rem;
        s->file_off = cur_off;
        if (s->type != ULD_ST_BSS) cur_off += (uld_u32_t)s->size;
    }

    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        uld_elf32_phdr_t ph;
        memset(&ph, 0, sizeof(ph));
        ph.p_type   = PT_LOAD;
        ph.p_offset = (uld_u32_t)s->file_off;
        ph.p_vaddr  = (uld_u32_t)s->vaddr;
        ph.p_paddr  = (uld_u32_t)s->vaddr;
        ph.p_filesz = (s->type == ULD_ST_BSS) ? 0 : (uld_u32_t)s->size;
        ph.p_memsz  = (uld_u32_t)s->size;
        ph.p_flags  = (s->flags & ULD_SF_EXEC) ? (PF_R|PF_X) : (PF_R|PF_W);
        ph.p_align  = s->align ? s->align : 0x1000;
        fwrite(&ph, sizeof(ph), 1, f);
    }

    for (uld_u32_t i = 0; i < nsects; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        if (s->type == ULD_ST_BSS || !s->data || !s->size) continue;
        fseek(f, (long)s->file_off, SEEK_SET);
        fwrite(s->data, 1, (size_t)s->size, f);
    }
    fclose(f);
    return 0;
}

/* ── Flat binary writer ─────────────────────────────────────── */
int uld_flat_write(uld_elf_ctx_t *ctx)
{
    FILE *f = fopen(ctx->output_path, "wb");
    if (!f) return -1;
    for (uld_u32_t i = 0; i < ctx->sects->count; i++) {
        uld_output_sect_t *s = &ctx->sects->sects[i];
        if (s->type == ULD_ST_BSS || !s->data || !s->size) continue;
        fwrite(s->data, 1, (size_t)s->size, f);
    }
    fclose(f);
    return 0;
}

/* ── Intel HEX writer ───────────────────────────────────────── */
int uld_ihex_write(uld_elf_ctx_t *ctx)
{
    FILE *f = fopen(ctx->output_path, "w");
    if (!f) return -1;
    for (uld_u32_t si = 0; si < ctx->sects->count; si++) {
        uld_output_sect_t *s = &ctx->sects->sects[si];
        if (s->type == ULD_ST_BSS || !s->data || !s->size) continue;

        /* Extended Linear Address Record (04) for upper 32 bits */
        uld_u16_t seg = (uld_u16_t)(s->vaddr >> 16);
        uld_u8_t cs   = (uld_u8_t)(2 + 0 + 4 + (seg>>8) + (seg&0xFF));
        fprintf(f, ":02000004%04X%02X\n", seg, (uld_u8_t)(~cs+1));

        uld_u32_t addr = (uld_u32_t)(s->vaddr & 0xFFFF);
        for (uld_u64_t i = 0; i < s->size; ) {
            uld_u32_t chunk = (uld_u32_t)(s->size - i);
            if (chunk > 16) chunk = 16;
            uld_u8_t sum = 0;
            fprintf(f, ":%02X%04X00", chunk, addr & 0xFFFF);
            sum += (uld_u8_t)chunk;
            sum += (uld_u8_t)((addr >> 8) & 0xFF);
            sum += (uld_u8_t)(addr & 0xFF);
            for (uld_u32_t j = 0; j < chunk; j++) {
                fprintf(f, "%02X", s->data[i+j]);
                sum += s->data[i+j];
            }
            fprintf(f, "%02X\n", (uld_u8_t)(~sum + 1));
            i    += chunk;
            addr += chunk;
        }
    }
    fprintf(f, ":00000001FF\n");
    fclose(f);
    return 0;
}

/* ── Motorola SREC writer ───────────────────────────────────── */
int uld_srec_write(uld_elf_ctx_t *ctx)
{
    FILE *f = fopen(ctx->output_path, "w");
    if (!f) return -1;
    fprintf(f, "S0030000FC\n"); /* header record */
    for (uld_u32_t si = 0; si < ctx->sects->count; si++) {
        uld_output_sect_t *s = &ctx->sects->sects[si];
        if (s->type == ULD_ST_BSS || !s->data || !s->size) continue;
        uld_u64_t addr = s->vaddr;
        for (uld_u64_t i = 0; i < s->size; ) {
            uld_u32_t chunk = (uld_u32_t)(s->size - i);
            if (chunk > 16) chunk = 16;
            /* S3: 32-bit address */
            uld_u8_t byte_count = (uld_u8_t)(chunk + 5); /* addr4+data+sum */
            uld_u8_t sum = byte_count;
            sum += (uld_u8_t)((addr >> 24) & 0xFF);
            sum += (uld_u8_t)((addr >> 16) & 0xFF);
            sum += (uld_u8_t)((addr >>  8) & 0xFF);
            sum += (uld_u8_t)(addr & 0xFF);
            fprintf(f, "S3%02X%08llX",
                byte_count,
                (unsigned long long)addr);
        for (uld_u32_t j = 0; j < chunk; j++) {
            fprintf(f, "%02X", s->data[i+j]);
            sum += s->data[i+j];
        }
        fprintf(f, "%02X\n", (uld_u8_t)(~sum));
        i    += chunk;
        addr += chunk;
    }
}
/* S7: end record with entry address */
uld_u8_t esum = 5;
esum += (uld_u8_t)((ctx->entry_addr >> 24) & 0xFF);
esum += (uld_u8_t)((ctx->entry_addr >> 16) & 0xFF);
esum += (uld_u8_t)((ctx->entry_addr >>  8) & 0xFF);
esum += (uld_u8_t)(ctx->entry_addr & 0xFF);
fprintf(f, "S705%08llX%02X\n",
        (unsigned long long)ctx->entry_addr,
        (uld_u8_t)(~esum));
fclose(f);
return 0;
}
