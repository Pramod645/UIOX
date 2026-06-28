#ifndef UIOX_LINKER_H
#define UIOX_LINKER_H
/*
 * uiox_linker.h - UIOX linker master include and driver API
 *
 * Pipeline:
 *   1. load_objects  — read all .uobj / ELF relocatable files
 *   2. load_archives — read .a archives, extract needed members
 *   3. merge_sects   — merge input sections into output sections
 *   4. collect_syms  — build global symbol table
 *   5. layout        — assign VMAs via linker script
 *   6. resolve_syms  — set final symbol values
 *   7. apply_relocs  — patch all relocation sites
 *   8. emit          — write ELF64/ELF32/flat/IHEX/SREC
 *   9. write_map     — generate map file
 */
#include "uiox_ld_types.h"
#include "uiox_ld_diag.h"
#include "uiox_ld_section.h"
#include "uiox_ld_symbol.h"
#include "uiox_ld_reloc.h"
#include "uiox_ld_object.h"
#include "uiox_ld_archive.h"
#include "uiox_ld_script.h"
#include "uiox_ld_map.h"
#include "uiox_ld_elf.h"

/* -- Linker options ----------------------------------------- */
typedef struct uld_options {
    /* inputs */
    const char   *obj_paths[ULD_MAX_OBJECTS];
    uld_u32_t     obj_count;
    const char   *ar_paths[ULD_MAX_ARCHIVES];
    uld_u32_t     ar_count;
    /* output */
    const char   *output_path;
    const char   *map_path;
    /* configuration */
    const char   *script_path;
    const char   *entry_sym;
    uld_arch_t    arch;
    uld_fmt_t     fmt;
    uld_endian_t  endian;
    /* flags */
    uld_bool_t    gc_sections;    /* remove unused sections       */
    uld_bool_t    strip_debug;    /* remove debug sections        */
    uld_bool_t    verbose;
    uld_bool_t    print_map;
    uld_bool_t    warn_undef;     /* warn on undefined (not error)*/
    uld_addr_t    text_base;      /* override .text base address  */
} uld_options_t;

/* -- Main linker context ------------------------------------ */
typedef struct uld_ctx {
    uld_options_t     opts;
    uld_diag_ctx_t    diag;
    /* loaded inputs */
    uld_object_t      objs[ULD_MAX_OBJECTS];
    uld_u32_t         obj_count;
    uld_archive_t     archives[ULD_MAX_ARCHIVES];
    uld_u32_t         ar_count;
    /* global tables */
    uld_sect_table_t  sects;
    uld_sym_table_t   syms;
    uld_reloc_table_t relocs;
    /* linker script */
    uld_script_t      script;
    uld_bool_t        script_loaded;
    /* resolved entry */
    uld_addr_t        entry_addr;
    char              entry_sym[ULD_NAME_MAX];
} uld_ctx_t;

/* -- Driver API --------------------------------------------- */
void uld_options_default (uld_options_t *opts);
int  uld_options_parse   (uld_options_t *opts, int argc, char **argv);
void uld_options_print   (const uld_options_t *opts);

int  uld_ctx_init        (uld_ctx_t *ctx, const uld_options_t *opts);
void uld_ctx_free        (uld_ctx_t *ctx);
int  uld_ctx_run         (uld_ctx_t *ctx);

/* -- Pipeline passes (called by uld_ctx_run) ---------------- */
int  uld_pass_load_objs   (uld_ctx_t *ctx);
int  uld_pass_load_archives(uld_ctx_t *ctx);
int  uld_pass_merge_sects (uld_ctx_t *ctx);
int  uld_pass_collect_syms(uld_ctx_t *ctx);
int  uld_pass_layout      (uld_ctx_t *ctx);
int  uld_pass_resolve_syms(uld_ctx_t *ctx);
int  uld_pass_apply_relocs(uld_ctx_t *ctx);
int  uld_pass_gc_sections (uld_ctx_t *ctx);
int  uld_pass_emit        (uld_ctx_t *ctx);
int  uld_pass_write_map   (uld_ctx_t *ctx);

#endif /* UIOX_LINKER_H */
