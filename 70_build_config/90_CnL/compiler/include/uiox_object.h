#ifndef UIOX_OBJECT_H
#define UIOX_OBJECT_H
/*
 * uiox_object.h - UIOX object file format (.uobj)
 * Simple relocatable object file used internally by the
 * UIOX compiler and linker pipeline.
 */
#include "uiox_section.h"

#define UIOX_OBJ_MAGIC      0x554F424A   /* "UOBJ"                 */
#define UIOX_OBJ_VERSION    1
#define UIOX_SYM_NAME_MAX   128
#define UIOX_OBJ_MAX_SECTS  32
#define UIOX_OBJ_MAX_SYMS   4096
#define UIOX_OBJ_MAX_RELOCS 8192

/* -- Symbol binding ----------------------------------------- */
typedef enum uiox_sym_bind {
    UIOX_BIND_LOCAL  = 0,
    UIOX_BIND_GLOBAL = 1,
    UIOX_BIND_WEAK   = 2,
} uiox_sym_bind_t;

/* -- Symbol type -------------------------------------------- */
typedef enum uiox_sym_type {
    UIOX_SYM_NOTYPE  = 0,
    UIOX_SYM_OBJECT  = 1,   /* data variable                   */
    UIOX_SYM_FUNC    = 2,   /* function                        */
    UIOX_SYM_SECTION = 3,
    UIOX_SYM_FILE    = 4,
} uiox_sym_type_t;

/* -- Relocation types --------------------------------------- */
typedef enum uiox_reloc_type {
    UIOX_RELOC_ABS64     = 0,  /* absolute 64-bit               */
    UIOX_RELOC_ABS32     = 1,  /* absolute 32-bit               */
    UIOX_RELOC_REL32     = 2,  /* PC-relative 32-bit            */
    UIOX_RELOC_REL64     = 3,  /* PC-relative 64-bit            */
    UIOX_RELOC_GOT32     = 4,  /* GOT-relative 32-bit           */
    UIOX_RELOC_PLT32     = 5,  /* PLT-relative 32-bit           */
    UIOX_RELOC_ARM_B26   = 6,  /* ARM branch 26-bit offset      */
    UIOX_RELOC_ARM_MOVW  = 7,  /* ARM MOVW imm16                */
    UIOX_RELOC_ARM_MOVT  = 8,  /* ARM MOVT imm16                */
    UIOX_RELOC_A64_CALL  = 9,  /* AArch64 BL/BLR               */
    UIOX_RELOC_A64_ADR   = 10, /* AArch64 ADR/ADRP             */
} uiox_reloc_type_t;

/* -- Symbol table entry ------------------------------------- */
typedef struct uiox_obj_sym {
    char             name[UIOX_SYM_NAME_MAX];
    unsigned int     sect_idx;    /* section index (0=undef)    */
    unsigned long long value;     /* offset within section      */
    unsigned long long size;      /* size of symbol             */
    uiox_sym_bind_t  bind;
    uiox_sym_type_t  type;
} uiox_obj_sym_t;

/* -- Relocation entry --------------------------------------- */
typedef struct uiox_obj_reloc {
    unsigned long long offset;    /* offset in section          */
    unsigned int       sym_idx;   /* index into symbol table    */
    uiox_reloc_type_t  type;
    long long          addend;    /* explicit addend            */
    unsigned int       sect_idx;  /* which section needs patch  */
} uiox_obj_reloc_t;

/* -- Object file header ------------------------------------- */
typedef struct uiox_obj_header {
    unsigned int   magic;         /* UIOX_OBJ_MAGIC              */
    unsigned int   version;       /* UIOX_OBJ_VERSION            */
    unsigned int   arch;          /* 32=ARM32, 64=ARM64/x86_64  */
    unsigned int   sect_count;
    unsigned int   sym_count;
    unsigned int   reloc_count;
    char           source_file[256];
} uiox_obj_header_t;

/* -- Full in-memory object file ----------------------------- */
typedef struct uiox_object {
    uiox_obj_header_t  hdr;
    uiox_section_t    *sects[UIOX_OBJ_MAX_SECTS];
    uiox_obj_sym_t     syms[UIOX_OBJ_MAX_SYMS];
    uiox_obj_reloc_t   relocs[UIOX_OBJ_MAX_RELOCS];
} uiox_object_t;

void uiox_object_init   (uiox_object_t *obj, const char *srcfile,
                          unsigned int arch);
void uiox_object_free   (uiox_object_t *obj);
int  uiox_object_write  (const uiox_object_t *obj, const char *path);
int  uiox_object_read   (uiox_object_t *obj, const char *path);
void uiox_object_print  (const uiox_object_t *obj);

int  uiox_object_add_sym(uiox_object_t *obj, const char *name,
                          unsigned int sect, unsigned long long val,
                          unsigned long long size,
                          uiox_sym_bind_t bind, uiox_sym_type_t type);

int  uiox_object_add_reloc(uiox_object_t *obj,
                            unsigned long long offset,
                            unsigned int sym_idx,
                            uiox_reloc_type_t type,
                            long long addend,
                            unsigned int sect_idx);

#endif /* UIOX_OBJECT_H */
