#ifndef UIOX_LD_OBJECT_H
#define UIOX_LD_OBJECT_H
/*
 * uiox_ld_object.h - UIOX linker object file reader
 * Reads both UIOX native .uobj files and ELF relocatable objects.
 */
#include "uiox_ld_types.h"
#include "uiox_ld_section.h"
#include "uiox_ld_symbol.h"
#include "uiox_ld_reloc.h"
#include "uiox_ld_diag.h"

#define ULD_OBJ_MAGIC_UOBJ  0x554F424Au  /* "UOBJ"              */
#define ULD_OBJ_MAGIC_ELF   0x464C457Fu  /* "\x7FELF"           */

typedef enum uld_obj_format {
    ULD_OBJ_UOBJ = 0,
    ULD_OBJ_ELF  = 1,
} uld_obj_format_t;

/* -- Per-object section reference --------------------------- */
typedef struct uld_obj_sect {
    char              name[ULD_NAME_MAX];
    uld_sect_type_t   type;
    uld_u32_t         flags;
    uld_u32_t         align;
    uld_u8_t         *data;
    uld_u32_t         size;
    uld_u32_t         out_sect_idx; /* index in global sect table */
    uld_u64_t         out_off;      /* offset within output sect  */
} uld_obj_sect_t;

/* -- Per-object local symbol -------------------------------- */
typedef struct uld_obj_sym {
    char            name[ULD_NAME_MAX];
    uld_u32_t       sect_idx;   /* local section index           */
    uld_u64_t       value;      /* offset within section         */
    uld_u64_t       size;
    uld_sym_bind_t  bind;
    uld_sym_type_t  type;
    uld_u32_t       global_idx; /* index in global symbol table  */
} uld_obj_sym_t;

/* -- Per-object relocation ---------------------------------- */
typedef struct uld_obj_reloc {
    uld_u64_t         offset;
    uld_u32_t         sym_idx;   /* local symbol index            */
    uld_reloc_type_t  type;
    uld_s64_t         addend;
    uld_u32_t         sect_idx;  /* local section being patched   */
} uld_obj_reloc_t;

/* -- Object file record ------------------------------------- */
typedef struct uld_object {
    char              path[ULD_PATH_MAX];
    uld_obj_format_t  format;
    uld_arch_t        arch;

    uld_obj_sect_t   *sects;
    uld_u32_t         sect_count;

    uld_obj_sym_t    *syms;
    uld_u32_t         sym_count;

    uld_obj_reloc_t  *relocs;
    uld_u32_t         reloc_count;

    uld_u8_t         *raw;       /* full file buffer              */
    uld_u64_t         raw_size;

    uld_u32_t         idx;       /* index in linker object table  */
} uld_object_t;

int  uld_object_load   (uld_object_t *obj, const char *path,
                         uld_diag_ctx_t *diag);
void uld_object_free   (uld_object_t *obj);
void uld_object_print  (const uld_object_t *obj);

/* internal readers */
int  uld_object_read_uobj(uld_object_t *obj, uld_diag_ctx_t *diag);
int  uld_object_read_elf (uld_object_t *obj, uld_diag_ctx_t *diag);

#endif /* UIOX_LD_OBJECT_H */
