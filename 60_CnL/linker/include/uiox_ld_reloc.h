#ifndef UIOX_LD_RELOC_H
#define UIOX_LD_RELOC_H
/*
 * uiox_ld_reloc.h - UIOX linker relocation engine
 */
#include "uiox_ld_types.h"
#include "uiox_ld_symbol.h"
#include "uiox_ld_section.h"
#include "uiox_ld_diag.h"

/* -- Relocation types --------------------------------------- */
typedef enum uld_reloc_type {
    /* Generic */
    ULD_R_NONE       =  0,
    ULD_R_ABS8       =  1,   /* absolute  8-bit              */
    ULD_R_ABS16      =  2,   /* absolute 16-bit              */
    ULD_R_ABS32      =  3,   /* absolute 32-bit              */
    ULD_R_ABS64      =  4,   /* absolute 64-bit              */
    ULD_R_REL8       =  5,   /* PC-relative  8-bit           */
    ULD_R_REL16      =  6,   /* PC-relative 16-bit           */
    ULD_R_REL32      =  7,   /* PC-relative 32-bit           */
    ULD_R_REL64      =  8,   /* PC-relative 64-bit           */
    /* x86_64 specific */
    ULD_R_X86_PLT32  =  9,   /* PLT-relative 32-bit          */
    ULD_R_X86_GOT32  = 10,   /* GOT-relative 32-bit          */
    ULD_R_X86_GOTPC  = 11,   /* GOT PC-relative              */
    /* ARM32 specific */
    ULD_R_ARM_B26    = 12,   /* ARM B/BL 26-bit word offset  */
    ULD_R_ARM_MOVW   = 13,   /* ARM MOVW imm16 [15:0]        */
    ULD_R_ARM_MOVT   = 14,   /* ARM MOVT imm16 [31:16]       */
    ULD_R_ARM_THM_B  = 15,   /* Thumb BL offset              */
    /* AArch64 specific */
    ULD_R_A64_CALL26 = 16,   /* AArch64 BL imm26             */
    ULD_R_A64_JUMP26 = 17,   /* AArch64 B  imm26             */
    ULD_R_A64_ADR    = 18,   /* AArch64 ADR imm21            */
    ULD_R_A64_ADRP   = 19,   /* AArch64 ADRP imm21           */
    ULD_R_A64_LO12   = 20,   /* AArch64 add/ldr low 12 bits  */
    ULD_R_A64_ABS64  = 21,   /* AArch64 absolute 64-bit      */
} uld_reloc_type_t;

/* -- Relocation record -------------------------------------- */
typedef struct uld_reloc {
    uld_addr_t       offset;      /* VMA of location to patch   */
    uld_u32_t        sym_idx;     /* global symbol table index  */
    uld_reloc_type_t type;
    uld_s64_t        addend;      /* explicit addend (RELA)     */
    uld_u32_t        sect_idx;    /* output section to patch    */
    uld_u32_t        obj_idx;     /* originating object file    */
} uld_reloc_t;

/* -- Relocation table --------------------------------------- */
typedef struct uld_reloc_table {
    uld_reloc_t  entries[ULD_MAX_RELOCS];
    uld_u32_t    count;
} uld_reloc_table_t;

void uld_reloc_table_init (uld_reloc_table_t *rt);
int  uld_reloc_add        (uld_reloc_table_t *rt,
                            uld_addr_t offset, uld_u32_t sym_idx,
                            uld_reloc_type_t type, uld_s64_t addend,
                            uld_u32_t sect_idx, uld_u32_t obj_idx);
int  uld_reloc_apply      (uld_reloc_table_t *rt,
                            uld_sym_table_t   *syms,
                            uld_sect_table_t  *sects,
                            uld_diag_ctx_t    *diag);
const char *uld_reloc_type_str(uld_reloc_type_t t);

#endif /* UIOX_LD_RELOC_H */
