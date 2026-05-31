#ifndef UIOX_LINKER_H
#define UIOX_LINKER_H
/*
 * uiox_linker.h - UIOX linker
 * Links one or more .uobj object files into a final ELF binary.
 */
#include "uiox_object.h"
#include "uiox_section.h"
#include "uiox_emit.h"
#include "uiox_error.h"

#define UIOX_LNK_MAX_OBJS    256
#define UIOX_LNK_MAX_SYMS    16384
#define UIOX_LNK_MAX_SECTS   64
#define UIOX_LNK_MAX_RELOCS  32768

typedef enum uiox_lnk_output {
    UIOX_LNK_ELF64,    /* 64-bit ELF                          */
    UIOX_LNK_ELF32,    /* 32-bit ELF                          */
    UIOX_LNK_FLAT,     /* flat binary image                   */
    UIOX_LNK_IHEX,     /* Intel HEX                           */
} uiox_lnk_output_t;

/* -- Global symbol (resolved across all objects) ------------ */
typedef struct uiox_lnk_sym {
    char               name[UIOX_SYM_NAME_MAX];
    unsigned long long value;      /* resolved virtual address   */
    unsigned long long size;
    uiox_sym_bind_t    bind;
    uiox_sym_type_t    type;
    int                obj_idx;    /* which object defines it    */
    int                sect_idx;   /* which merged section       */
    int                defined;
} uiox_lnk_sym_t;

/* -- Merged output section ---------------------------------- */
typedef struct uiox_lnk_sect {
    char               name[UIOX_SECT_NAME_MAX];
    uiox_sect_type_t   type;
    unsigned int       flags;
    unsigned long long vaddr;
    unsigned long long file_off;
    unsigned int       align;
    unsigned char     *data;
    unsigned int       size;
    unsigned int       cap;
} uiox_lnk_sect_t;

/* -- Pending relocation ------------------------------------- */
typedef struct uiox_lnk_reloc {
    unsigned long long offset;     /* VMA of patch location      */
    int                sym_idx;    /* global symbol table index  */
    uiox_reloc_type_t  type;
    long long          addend;
    int                sect_idx;   /* output section to patch    */
} uiox_lnk_reloc_t;

/* -- Linker script entry (simplified) ----------------------- */
typedef struct uiox_lnk_script_entry {
    char               sect_name[UIOX_SECT_NAME_MAX];
    unsigned long long load_addr;   /* physical load address     */
    unsigned long long virt_addr;   /* virtual address           */
} uiox_lnk_script_entry_t;

/* -- Linker context ----------------------------------------- */
typedef struct uiox_linker {
    /* input objects */
    uiox_object_t     *objs[UIOX_LNK_MAX_OBJS];
    int                obj_count;

    /* global symbol table */
    uiox_lnk_sym_t    syms[UIOX_LNK_MAX_SYMS];
    int                sym_count;

    /* merged output sections */
    uiox_lnk_sect_t   sects[UIOX_LNK_MAX_SECTS];
    int                sect_count;

    /* pending relocations */
    uiox_lnk_reloc_t  relocs[UIOX_LNK_MAX_RELOCS];
    int                reloc_count;

    /* configuration */
    uiox_lnk_output_t output_format;
    uiox_target_arch_t arch;
    unsigned long long entry_addr;
    char               entry_sym[UIOX_SYM_NAME_MAX];
    char               output_path[512];
    const char        *linker_script;

    /* diagnostics */
    uiox_diag_ctx_t   *diag;
} uiox_linker_t;

/* -- Linker API --------------------------------------------- */
void uiox_linker_init       (uiox_linker_t *lnk, uiox_diag_ctx_t *diag,
                              uiox_target_arch_t arch,
                              uiox_lnk_output_t fmt);
void uiox_linker_free       (uiox_linker_t *lnk);
int  uiox_linker_add_object (uiox_linker_t *lnk, uiox_object_t *obj);
int  uiox_linker_add_object_file(uiox_linker_t *lnk, const char *path);
int  uiox_linker_set_script (uiox_linker_t *lnk, const char *script_path);
int  uiox_linker_set_entry  (uiox_linker_t *lnk, const char *sym);
int  uiox_linker_set_output (uiox_linker_t *lnk, const char *path);
int  uiox_linker_run        (uiox_linker_t *lnk);

/* -- Internal linker passes --------------------------------- */
int  uiox_lnk_pass_collect  (uiox_linker_t *lnk);
int  uiox_lnk_pass_merge    (uiox_linker_t *lnk);
int  uiox_lnk_pass_layout   (uiox_linker_t *lnk);
int  uiox_lnk_pass_resolve  (uiox_linker_t *lnk);
int  uiox_lnk_pass_relocate (uiox_linker_t *lnk);
int  uiox_lnk_pass_emit     (uiox_linker_t *lnk);

/* -- ELF emission ------------------------------------------- */
int  uiox_lnk_emit_elf64    (uiox_linker_t *lnk);
int  uiox_lnk_emit_elf32    (uiox_linker_t *lnk);
int  uiox_lnk_emit_flat     (uiox_linker_t *lnk);
int  uiox_lnk_emit_ihex     (uiox_linker_t *lnk);

#endif /* UIOX_LINKER_H */
