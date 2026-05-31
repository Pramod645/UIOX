/*
 * uiox_ld_map.c - UIOX linker map file generator
 */
#include "../include/uiox_ld_map.h"
#include <stdio.h>
#include <string.h>

int uld_map_write(const uld_map_ctx_t *ctx, const char *map_path)
{
    FILE *f = fopen(map_path, "w");
    if (!f) return -1;

    fprintf(f,
        "============================================================\n"
        " UIOX Linker Map File\n"
        " Output  : %s\n"
        " Entry   : %s  (0x%016llx)\n"
        " Arch    : %s\n"
        "============================================================\n\n",
        ctx->output_path,
        ctx->entry_sym,
        (unsigned long long)ctx->entry_addr,
        ctx->arch == ULD_ARCH_ARM64  ? "arm64"
      : ctx->arch == ULD_ARCH_ARM32  ? "arm32"
      : "x86_64");

    /* ── Memory sections ─────────────────────────────────── */
    fprintf(f, "Output Sections:\n");
    fprintf(f, "%-20s  %-18s  %-18s  %-10s  %s\n",
            "Name", "VMA", "File Off", "Size", "Align");
    fprintf(f, "%-20s  %-18s  %-18s  %-10s  %s\n",
            "----", "---", "--------", "----", "-----");

    for (uld_u32_t i = 0; i < ctx->sects->count; i++) {
        const uld_output_sect_t *s = &ctx->sects->sects[i];
        if (s->dead) continue;
        fprintf(f, "%-20s  0x%016llx  0x%016llx  0x%08llx  %u\n",
                s->name,
                (unsigned long long)s->vaddr,
                (unsigned long long)s->file_off,
                (unsigned long long)s->size,
                s->align);

        /* per-object contributions */
        for (uld_input_sect_t *in = s->inputs; in; in = in->next) {
            const char *obj_name = "<unknown>";
            if (in->obj_idx < ctx->obj_count)
                obj_name = ctx->objs[in->obj_idx].path;
            fprintf(f,
                "  %-18s  0x%016llx  0x%016llx  0x%08x  (from %s)\n",
                in->name,
                (unsigned long long)(s->vaddr + in->output_off),
                (unsigned long long)in->output_off,
                in->size,
                obj_name);
        }
    }

    /* ── Symbol table ────────────────────────────────────── */
    fprintf(f, "\nSymbol Table (%u symbols):\n", ctx->syms->count);
    fprintf(f, "%-40s  %-18s  %-8s  %s\n",
            "Name", "Value", "Size", "Bind");
    fprintf(f, "%-40s  %-18s  %-8s  %s\n",
            "----", "-----", "----", "----");

    for (uld_u32_t i = 0; i < ctx->syms->count; i++) {
        const uld_symbol_t *s = ctx->syms->list[i];
        if (!s) continue;
        fprintf(f, "%-40s  0x%016llx  0x%06llx  %s\n",
                s->name,
                (unsigned long long)s->value,
                (unsigned long long)s->size,
                s->bind == ULD_BIND_GLOBAL ? "global"
              : s->bind == ULD_BIND_WEAK   ? "weak"
              : "local");
    }

    fprintf(f, "\n============================================================\n");
    fclose(f);
    return 0;
}
