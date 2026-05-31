/*
 * uiox_linker.c - UIOX linker pipeline implementation
 */
#include "../include/uiox_linker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Init / free ─────────────────────────────────────────── */
int uld_ctx_init(uld_ctx_t *ctx, const uld_options_t *opts)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->opts = *opts;
    uld_diag_init(&ctx->diag, opts->verbose);
    uld_sect_table_init(&ctx->sects);
    uld_sym_table_init(&ctx->syms);
    uld_reloc_table_init(&ctx->relocs);
    strncpy(ctx->entry_sym,
            opts->entry_sym ? opts->entry_sym : "_start",
            ULD_NAME_MAX - 1);
    return 0;
}

void uld_ctx_free(uld_ctx_t *ctx)
{
    for (uld_u32_t i = 0; i < ctx->obj_count; i++)
        uld_object_free(&ctx->objs[i]);
    for (uld_u32_t i = 0; i < ctx->ar_count; i++)
        uld_archive_free(&ctx->archives[i]);
    uld_sect_table_free(&ctx->sects);
    uld_sym_table_free(&ctx->syms);
    uld_script_free(&ctx->script);
    uld_diag_free(&ctx->diag);
}

/* ── Pass 1: load object files ──────────────────────────── */
int uld_pass_load_objs(uld_ctx_t *ctx)
{
    for (uld_u32_t i = 0; i < ctx->opts.obj_count; i++) {
        if (ctx->obj_count >= ULD_MAX_OBJECTS) {
            ULD_ERR(&ctx->diag, "<linker>", 0,
                    "too many object files (max %d)", ULD_MAX_OBJECTS);
            return -1;
        }
        uld_object_t *obj = &ctx->objs[ctx->obj_count];
        if (uld_object_load(obj, ctx->opts.obj_paths[i],
                             &ctx->diag) == 0) {
            obj->idx = ctx->obj_count;
            ctx->obj_count++;
            if (ctx->diag.verbose)
                printf("[pass1] loaded: %s  sects=%u syms=%u\n",
                       obj->path, obj->sect_count, obj->sym_count);
        }
    }
    return uld_diag_ok(&ctx->diag) ? 0 : -1;
}

/* ── Pass 2: load archives ──────────────────────────────── */
int uld_pass_load_archives(uld_ctx_t *ctx)
{
    for (uld_u32_t i = 0; i < ctx->opts.ar_count; i++) {
        if (ctx->ar_count >= ULD_MAX_ARCHIVES) break;
        uld_archive_t *ar = &ctx->archives[ctx->ar_count];
        if (uld_archive_open(ar, ctx->opts.ar_paths[i],
                              &ctx->diag) == 0) {
            ctx->ar_count++;
            /* extract all members */
            uld_archive_extract_all(ar,
                                     ctx->objs,
                                     &ctx->obj_count,
                                     ULD_MAX_OBJECTS,
                                     &ctx->diag);
            if (ctx->diag.verbose)
                printf("[pass2] archive: %s  members=%u\n",
                       ar->path, ar->member_count);
        }
    }
    return 0;
}

/* ── Pass 3: merge sections ─────────────────────────────── */
int uld_pass_merge_sects(uld_ctx_t *ctx)
{
    /* determine canonical section order from linker script */
    static const char *default_order[] = {
        ".vectors", ".text", ".rodata", ".data", ".bss", NULL
    };

    for (uld_u32_t oi = 0; oi < ctx->obj_count; oi++) {
        uld_object_t *obj = &ctx->objs[oi];
        for (uld_u32_t si = 0; si < obj->sect_count; si++) {
            uld_obj_sect_t *isrc = &obj->sects[si];
            if (isrc->type == ULD_ST_NULL   ||
                isrc->type == ULD_ST_SYMTAB ||
                isrc->type == ULD_ST_STRTAB ||
                isrc->type == ULD_ST_RELOC  ||
                isrc->name[0] == '\0') continue;

            /* skip debug sections if strip_debug */
            if (ctx->opts.strip_debug &&
                strncmp(isrc->name, ".debug", 6) == 0) continue;

            uld_u32_t flags = ULD_SF_ALLOC;
            if (isrc->type == ULD_ST_TEXT)
                flags |= ULD_SF_EXEC;
            else if (isrc->type == ULD_ST_DATA ||
                     isrc->type == ULD_ST_BSS)
                flags |= ULD_SF_WRITE;
            if (isrc->type == ULD_ST_BSS)
                flags |= ULD_SF_NOLOAD;

            uld_output_sect_t *out =
                uld_sect_get_or_create(&ctx->sects,
                                        isrc->name,
                                        isrc->type,
                                        flags,
                                        isrc->align);
            if (!out) continue;

            uld_input_sect_t *in =
                (uld_input_sect_t *)calloc(1, sizeof(*in));
            if (!in) continue;
            strncpy(in->name, isrc->name, ULD_NAME_MAX - 1);
            in->type    = isrc->type;
            in->flags   = flags;
            in->align   = isrc->align;
            in->size    = isrc->size;
            in->obj_idx = oi;
            if (isrc->data && isrc->size) {
                in->data = (uld_u8_t *)malloc(isrc->size);
                if (in->data)
                    memcpy(in->data, isrc->data, isrc->size);
            }

            isrc->out_sect_idx = out->idx;
            isrc->out_off      = out->size;
            uld_sect_append(out, in);
        }
    }

    (void)default_order;

    if (ctx->diag.verbose)
        uld_sect_print(&ctx->sects);
    return 0;
}

/* ── Pass 4: collect symbols ────────────────────────────── */
int uld_pass_collect_syms(uld_ctx_t *ctx)
{
    for (uld_u32_t oi = 0; oi < ctx->obj_count; oi++) {
        uld_object_t *obj = &ctx->objs[oi];
        for (uld_u32_t si = 0; si < obj->sym_count; si++) {
            uld_obj_sym_t *sym = &obj->syms[si];
            if (sym->name[0] == '\0') continue;
            if (sym->type == ULD_SYM_FILE ||
                sym->type == ULD_SYM_SECTION) continue;

            if (sym->bind == ULD_BIND_LOCAL) {
                /* local: insert with unique name */
                uld_symbol_t *gs =
                    uld_sym_insert(&ctx->syms, sym->name,
                                    ULD_BIND_LOCAL, sym->type);
                if (gs) {
                    gs->size    = sym->size;
                    gs->obj_idx = oi;
                }
                sym->global_idx = ctx->syms.count - 1;
                continue;
            }

            /* global / weak */
            uld_u64_t val = 0;
            uld_u32_t sect_out = 0;
            uld_bool_t defined = ULD_FALSE;

            if (sym->sect_idx < obj->sect_count) {
                uld_obj_sect_t *osect = &obj->sects[sym->sect_idx];
                if (osect->out_sect_idx < ctx->sects.count) {
                    sect_out = osect->out_sect_idx;
                    val      = osect->out_off + sym->value;
                    defined  = ULD_TRUE;
                }
            }

            if (defined) {
                uld_sym_define(&ctx->syms, sym->name,
                                val, sym->size,
                                sect_out, oi,
                                sym->bind, sym->type,
                                &ctx->diag);
            } else {
                uld_symbol_t *gs =
                    uld_sym_lookup(&ctx->syms, sym->name);
                if (!gs)
                    gs = uld_sym_insert(&ctx->syms, sym->name,
                                         sym->bind, sym->type);
                if (gs) gs->referenced = ULD_TRUE;
            }

            /* track local index -> global index */
            uld_symbol_t *gs = uld_sym_lookup(&ctx->syms, sym->name);
            if (gs) {
                for (uld_u32_t gi = 0; gi < ctx->syms.count; gi++) {
                    if (ctx->syms.list[gi] == gs) {
                        sym->global_idx = gi;
                        break;
                    }
                }
            }
        }
    }

    /* collect relocations */
    for (uld_u32_t oi = 0; oi < ctx->obj_count; oi++) {
        uld_object_t *obj = &ctx->objs[oi];
        for (uld_u32_t ri = 0; ri < obj->reloc_count; ri++) {
            uld_obj_reloc_t *r = &obj->relocs[ri];
            uld_u32_t gsym_idx = (r->sym_idx < obj->sym_count)
                                   ? obj->syms[r->sym_idx].global_idx
                                   : 0;
            uld_u32_t sect_out = (r->sect_idx < obj->sect_count)
                                   ? obj->sects[r->sect_idx].out_sect_idx
                                   : 0;
            uld_u64_t abs_off  = (r->sect_idx < obj->sect_count)
                                   ? obj->sects[r->sect_idx].out_off + r->offset
                                   : r->offset;
            /* convert to VMA after layout — store as section-relative for now */
            uld_reloc_add(&ctx->relocs,
                           abs_off, gsym_idx,
                           r->type, r->addend,
                           sect_out, oi);
            if (gsym_idx < ctx->syms.count && ctx->syms.list[gsym_idx])
                ctx->syms.list[gsym_idx]->referenced = ULD_TRUE;
        }
    }

    if (ctx->diag.verbose)
        printf("[pass4] symbols=%u  relocs=%u\n",
               ctx->syms.count, ctx->relocs.count);

    return uld_sym_check_undef(&ctx->syms, &ctx->diag) == 0 ? 0 : -1;
}

/* ── Pass 5: layout (assign VMAs) ───────────────────────── */
int uld_pass_layout(uld_ctx_t *ctx)
{
    /* Use linker script if available */
    if (ctx->script_loaded) {
        for (uld_u32_t ci = 0; ci < ctx->script.sect_cmd_count; ci++) {
            uld_sect_cmd_t *cmd = &ctx->script.sect_cmds[ci];
            uld_output_sect_t *out =
                uld_sect_find(&ctx->sects, cmd->out_name);
            if (!out) continue;

            /* find memory region */
            uld_mem_region_t *reg = NULL;
            for (uld_u32_t ri = 0; ri < ctx->script.region_count; ri++) {
                if (strcmp(ctx->script.regions[ri].name,
                           cmd->region) == 0) {
                    reg = &ctx->script.regions[ri];
                    break;
                }
            }

            if (reg) {
                /* align within region */
                uld_u32_t al = out->align ? out->align : 1;
                uld_u64_t rem = reg->current % al;
                if (rem) reg->current += al - rem;
                out->vaddr      = reg->current;
                out->dead       = cmd->noload ? ULD_TRUE : ULD_FALSE;
                if (!cmd->noload) reg->current += out->size;
            }
        }
    } else {
        /* default layout: pack sections sequentially */
        uld_addr_t cur = ctx->opts.text_base
                        ? ctx->opts.text_base
                        : 0x00100000ULL;

        static const char *order[] = {
            ".vectors",".text",".rodata",".data",".bss", NULL
        };

        /* first pass: ordered sections */
        for (int oi = 0; order[oi]; oi++) {
            uld_output_sect_t *s = uld_sect_find(&ctx->sects, order[oi]);
            if (!s || s->size == 0) continue;
            uld_u32_t al = s->align ? s->align : 1;
            uld_u64_t rem = cur % al;
            if (rem) cur += al - rem;
            s->vaddr = cur;
            cur += s->size;
        }

        /* second pass: any remaining sections */
        for (uld_u32_t i = 0; i < ctx->sects.count; i++) {
            uld_output_sect_t *s = &ctx->sects.sects[i];
            if (s->vaddr != 0 || s->size == 0) continue;
            uld_u32_t al = s->align ? s->align : 1;
            uld_u64_t rem = cur % al;
            if (rem) cur += al - rem;
            s->vaddr = cur;
            cur += s->size;
        }
    }

    if (ctx->diag.verbose) {
        printf("[pass5] section layout:\n");
        uld_sect_print(&ctx->sects);
    }
    return 0;
}

/* ── Pass 6: resolve symbol values ──────────────────────── */
int uld_pass_resolve_syms(uld_ctx_t *ctx)
{
    for (uld_u32_t i = 0; i < ctx->syms.count; i++) {
        uld_symbol_t *s = ctx->syms.list[i];
        if (!s || !s->defined || s->resolved) continue;
        if (s->sect_idx < ctx->sects.count) {
            s->value  += ctx->sects.sects[s->sect_idx].vaddr;
            s->resolved = ULD_TRUE;
        }
    }

    /* fix up section-relative reloc offsets to absolute VMAs */
    for (uld_u32_t i = 0; i < ctx->relocs.count; i++) {
        uld_reloc_t *r = &ctx->relocs.entries[i];
        if (r->sect_idx < ctx->sects.count)
            r->offset += ctx->sects.sects[r->sect_idx].vaddr;
    }

    /* resolve entry point */
    uld_symbol_t *entry = uld_sym_lookup(&ctx->syms, ctx->entry_sym);
    if (entry && entry->defined)
        ctx->entry_addr = entry->value;
    else if (ctx->sects.count > 0)
        ctx->entry_addr = ctx->sects.sects[0].vaddr;

    /* define linker-provided boundary symbols */
    for (uld_u32_t i = 0; i < ctx->sects.count; i++) {
        uld_output_sect_t *s = &ctx->sects.sects[i];
        char sym_start[ULD_NAME_MAX], sym_end[ULD_NAME_MAX];
        snprintf(sym_start, sizeof(sym_start), "__%s_start__", s->name + 1);
        snprintf(sym_end,   sizeof(sym_end),   "__%s_end__",   s->name + 1);
        uld_sym_define_abs(&ctx->syms, sym_start, s->vaddr);
        uld_sym_define_abs(&ctx->syms, sym_end,   s->vaddr + s->size);
    }
    uld_sym_define_abs(&ctx->syms, "__stack_top__",
                        ctx->script_loaded
                        ? ctx->script.regions[0].origin +
                          ctx->script.regions[0].length
                        : 0x04000000ULL);

    if (ctx->diag.verbose)
        printf("[pass6] entry=0x%llx  (%s)\n",
               (unsigned long long)ctx->entry_addr, ctx->entry_sym);
    return 0;
}

/* ── Pass 7: apply relocations ───────────────────────────── */
int uld_pass_apply_relocs(uld_ctx_t *ctx)
{
    int rc = uld_reloc_apply(&ctx->relocs,
                               &ctx->syms,
                               &ctx->sects,
                               &ctx->diag);
    if (ctx->diag.verbose)
        printf("[pass7] applied %u relocations  (%d errors)\n",
               ctx->relocs.count, rc);
    return rc > 0 ? -1 : 0;
}

/* ── Pass 8: GC unused sections ─────────────────────────── */
int uld_pass_gc_sections(uld_ctx_t *ctx)
{
    if (!ctx->opts.gc_sections) return 0;
    uld_u32_t removed = 0;
    for (uld_u32_t i = 0; i < ctx->sects.count; i++) {
        uld_output_sect_t *s = &ctx->sects.sects[i];
        if (s->input_count == 0 && s->size == 0) {
            s->dead = ULD_TRUE;
            removed++;
        }
    }
    if (ctx->diag.verbose)
        printf("[pass8] GC removed %u empty sections\n", removed);
    return 0;
}

/* ── Pass 9: emit output ─────────────────────────────────── */
int uld_pass_emit(uld_ctx_t *ctx)
{
    uld_elf_ctx_t ec;
    ec.arch        = ctx->opts.arch;
    ec.entry_addr  = ctx->entry_addr;
    ec.sects       = &ctx->sects;
    ec.syms        = &ctx->syms;
    ec.output_path = ctx->opts.output_path;
    ec.diag        = &ctx->diag;

    int rc = 0;
    switch (ctx->opts.fmt) {
        case ULD_FMT_ELF64: rc = uld_elf64_write(&ec); break;
        case ULD_FMT_ELF32: rc = uld_elf32_write(&ec); break;
        case ULD_FMT_FLAT:  rc = uld_flat_write(&ec);  break;
        case ULD_FMT_IHEX:  rc = uld_ihex_write(&ec);  break;
        case ULD_FMT_SREC:  rc = uld_srec_write(&ec);  break;
        default:
            ULD_ERR(&ctx->diag, "<linker>", 0,
                    "unknown output format");
            rc = -1; break;
    }
    return rc;
}

/* ── Pass 10: write map file ─────────────────────────────── */
int uld_pass_write_map(uld_ctx_t *ctx)
{
    if (!ctx->opts.map_path && !ctx->opts.print_map) return 0;

    const char *mp = ctx->opts.map_path
                   ? ctx->opts.map_path : "uiox.map";

    uld_map_ctx_t mc;
    mc.output_path = ctx->opts.output_path;
    mc.sects       = &ctx->sects;
    mc.syms        = &ctx->syms;
    mc.objs        = ctx->objs;
    mc.obj_count   = ctx->obj_count;
    mc.arch        = ctx->opts.arch;
    mc.entry_addr  = ctx->entry_addr;
    mc.entry_sym   = ctx->entry_sym;

    int rc = uld_map_write(&mc, mp);
    if (rc == 0 && ctx->diag.verbose)
        printf("[pass10] map: %s\n", mp);
    return rc;
}

/* ── Top-level run ───────────────────────────────────────── */
int uld_ctx_run(uld_ctx_t *ctx)
{
    /* load linker script */
    if (ctx->opts.script_path) {
        if (uld_script_parse(&ctx->script,
                              ctx->opts.script_path,
                              &ctx->diag) == 0)
            ctx->script_loaded = ULD_TRUE;
    } else {
        const char *def = NULL;
        switch (ctx->opts.arch) {
            case ULD_ARCH_ARM64: def = uld_script_default_arm64();  break;
            case ULD_ARCH_ARM32: def = uld_script_default_arm32();  break;
            default:             def = uld_script_default_x86_64(); break;
        }
        if (def && uld_script_parse_str(&ctx->script, def,
                                          &ctx->diag) == 0)
            ctx->script_loaded = ULD_TRUE;
    }

    if (uld_pass_load_objs   (ctx) < 0) goto done;
    if (uld_pass_load_archives(ctx) < 0) goto done;
    if (uld_pass_merge_sects (ctx) < 0) goto done;
    if (uld_pass_collect_syms(ctx) < 0) goto done;
    if (uld_pass_layout      (ctx) < 0) goto done;
    if (uld_pass_resolve_syms(ctx) < 0) goto done;
    if (uld_pass_apply_relocs(ctx) < 0) goto done;
    if (uld_pass_gc_sections (ctx) < 0) goto done;
    if (uld_pass_emit        (ctx) < 0) goto done;
    if (uld_pass_write_map   (ctx) < 0) goto done;

done:
    uld_diag_print(&ctx->diag);
    return uld_diag_ok(&ctx->diag) ? 0 : 1;
}
