#ifndef UIOX_LD_SECTION_H
#define UIOX_LD_SECTION_H
/*
 * uiox_ld_section.h - UIOX linker section management
 *
 * The linker merges input sections from all object files into
 * a set of output sections, then assigns virtual addresses.
 */
#include "uiox_ld_types.h"

/* -- Section type ------------------------------------------- */
typedef enum uld_sect_type {
    ULD_ST_NULL    = 0,
    ULD_ST_TEXT    = 1,   /* executable code                   */
    ULD_ST_RODATA  = 2,   /* read-only data                    */
    ULD_ST_DATA    = 3,   /* initialised read-write data       */
    ULD_ST_BSS     = 4,   /* zero-initialised (no file bytes)  */
    ULD_ST_RELOC   = 5,   /* relocation records                */
    ULD_ST_SYMTAB  = 6,   /* symbol table                      */
    ULD_ST_STRTAB  = 7,   /* string table                      */
    ULD_ST_DEBUG   = 8,   /* DWARF / debug                     */
    ULD_ST_NOTE    = 9,   /* ELF notes                         */
    ULD_ST_CUSTOM  = 10,  /* user-defined                      */
} uld_sect_type_t;

/* -- Section flags ------------------------------------------ */
#define ULD_SF_ALLOC   (1u << 0)  /* loaded into memory          */
#define ULD_SF_EXEC    (1u << 1)  /* executable                  */
#define ULD_SF_WRITE   (1u << 2)  /* writable                    */
#define ULD_SF_MERGE   (1u << 3)  /* mergeable constants         */
#define ULD_SF_STRINGS (1u << 4)  /* null-terminated strings     */
#define ULD_SF_NOLOAD  (1u << 5)  /* not loaded (BSS)            */
#define ULD_SF_KEEP    (1u << 6)  /* never GC'd                  */

/* -- Input section (from one object file) ------------------- */
typedef struct uld_input_sect {
    char              name[ULD_NAME_MAX];
    uld_sect_type_t   type;
    uld_u32_t         flags;
    uld_u32_t         align;       /* alignment (power of 2)      */
    uld_u8_t         *data;        /* raw bytes (NULL for BSS)    */
    uld_u32_t         size;        /* byte size                   */
    uld_u32_t         obj_idx;     /* which object file           */
    uld_addr_t        output_off;  /* offset within output sect   */
    struct uld_input_sect *next;
} uld_input_sect_t;

/* -- Output section (merged from many input sections) ------- */
typedef struct uld_output_sect {
    char              name[ULD_NAME_MAX];
    uld_sect_type_t   type;
    uld_u32_t         flags;
    uld_u32_t         align;
    uld_addr_t        vaddr;       /* assigned virtual address    */
    uld_off_t         file_off;    /* offset in output file       */
    uld_u8_t         *data;        /* merged raw bytes            */
    uld_u64_t         size;        /* total merged byte size      */
    uld_u64_t         cap;         /* allocated capacity          */
    uld_input_sect_t *inputs;      /* linked list of inputs       */
    uld_input_sect_t *inputs_tail;
    uld_u32_t         input_count;
    uld_u32_t         idx;         /* index in output_sect table  */
    uld_bool_t        dead;        /* GC'd away                   */
} uld_output_sect_t;

/* -- Section table ------------------------------------------ */
typedef struct uld_sect_table {
    uld_output_sect_t  sects[ULD_MAX_SECTIONS];
    uld_u32_t          count;
} uld_sect_table_t;

void              uld_sect_table_init  (uld_sect_table_t *st);
void              uld_sect_table_free  (uld_sect_table_t *st);
uld_output_sect_t *uld_sect_find      (uld_sect_table_t *st,
                                        const char *name);
uld_output_sect_t *uld_sect_get_or_create(uld_sect_table_t *st,
                                           const char *name,
                                           uld_sect_type_t type,
                                           uld_u32_t flags,
                                           uld_u32_t align);
int               uld_sect_append     (uld_output_sect_t *out,
                                        uld_input_sect_t  *in);
void              uld_sect_patch32    (uld_output_sect_t *s,
                                        uld_u64_t off, uld_u32_t val);
void              uld_sect_patch64    (uld_output_sect_t *s,
                                        uld_u64_t off, uld_u64_t val);
void              uld_sect_print      (const uld_sect_table_t *st);

#endif /* UIOX_LD_SECTION_H */
