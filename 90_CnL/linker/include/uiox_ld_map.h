#ifndef UIOX_LD_MAP_H
#define UIOX_LD_MAP_H
/*
 * uiox_ld_map.h - UIOX linker map file generator
 * Produces a human-readable .map file showing:
 *   - output sections with VMA, size, alignment
 *   - all symbols with their resolved addresses
 *   - per-object contribution to each section
 */
#include "uiox_ld_types.h"
#include "uiox_ld_section.h"
#include "uiox_ld_symbol.h"
#include "uiox_ld_object.h"

typedef struct uld_map_ctx {
    const char             *output_path;
    const uld_sect_table_t *sects;
    const uld_sym_table_t  *syms;
    const uld_object_t     *objs;
    uld_u32_t               obj_count;
    uld_arch_t              arch;
    uld_addr_t              entry_addr;
    const char             *entry_sym;
} uld_map_ctx_t;

int uld_map_write(const uld_map_ctx_t *ctx, const char *map_path);

#endif /* UIOX_LD_MAP_H */
