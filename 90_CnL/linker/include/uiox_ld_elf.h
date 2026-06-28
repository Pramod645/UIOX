#ifndef UIOX_LD_ELF_H
#define UIOX_LD_ELF_H
/*
 * uiox_ld_elf.h - UIOX ELF64 / ELF32 output writer
 */
#include "uiox_ld_types.h"
#include "uiox_ld_section.h"
#include "uiox_ld_symbol.h"
#include "uiox_ld_diag.h"

/* ELF magic */
#define ELF_MAG0   0x7F
#define ELF_MAG1   'E'
#define ELF_MAG2   'L'
#define ELF_MAG3   'F'

/* ELF classes */
#define ELFCLASS32  1
#define ELFCLASS64  2

/* ELF data encoding */
#define ELFDATA2LSB 1  /* little-endian */
#define ELFDATA2MSB 2  /* big-endian    */

/* ELF types */
#define ET_EXEC  2
#define ET_REL   1
#define ET_DYN   3

/* ELF machines */
#define EM_386     3
#define EM_ARM     40
#define EM_X86_64  62
#define EM_AARCH64 183

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_NOTE    4
#define PT_PHDR    6

/* Program header flags */
#define PF_X  1   /* execute */
#define PF_W  2   /* write   */
#define PF_R  4   /* read    */

/* Section header types */
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_NOBITS   8
#define SHT_REL      9

/* Section header flags */
#define SHF_WRITE      0x1
#define SHF_ALLOC      0x2
#define SHF_EXECINSTR  0x4

/* -- ELF64 structures --------------------------------------- */
typedef struct {
    uld_u8_t   e_ident[16];
    uld_u16_t  e_type;
    uld_u16_t  e_machine;
    uld_u32_t  e_version;
    uld_u64_t  e_entry;
    uld_u64_t  e_phoff;
    uld_u64_t  e_shoff;
    uld_u32_t  e_flags;
    uld_u16_t  e_ehsize;
    uld_u16_t  e_phentsize;
    uld_u16_t  e_phnum;
    uld_u16_t  e_shentsize;
    uld_u16_t  e_shnum;
    uld_u16_t  e_shstrndx;
} __attribute__((packed)) uld_elf64_ehdr_t;

typedef struct {
    uld_u32_t  p_type;
    uld_u32_t  p_flags;
    uld_u64_t  p_offset;
    uld_u64_t  p_vaddr;
    uld_u64_t  p_paddr;
    uld_u64_t  p_filesz;
    uld_u64_t  p_memsz;
    uld_u64_t  p_align;
} __attribute__((packed)) uld_elf64_phdr_t;

typedef struct {
    uld_u32_t  sh_name;
    uld_u32_t  sh_type;
    uld_u64_t  sh_flags;
    uld_u64_t  sh_addr;
    uld_u64_t  sh_offset;
    uld_u64_t  sh_size;
    uld_u32_t  sh_link;
    uld_u32_t  sh_info;
    uld_u64_t  sh_addralign;
    uld_u64_t  sh_entsize;
} __attribute__((packed)) uld_elf64_shdr_t;

typedef struct {
    uld_u32_t  st_name;
    uld_u8_t   st_info;
    uld_u8_t   st_other;
    uld_u16_t  st_shndx;
    uld_u64_t  st_value;
    uld_u64_t  st_size;
} __attribute__((packed)) uld_elf64_sym_t;

/* -- ELF32 structures --------------------------------------- */
typedef struct {
    uld_u8_t   e_ident[16];
    uld_u16_t  e_type;
    uld_u16_t  e_machine;
    uld_u32_t  e_version;
    uld_u32_t  e_entry;
    uld_u32_t  e_phoff;
    uld_u32_t  e_shoff;
    uld_u32_t  e_flags;
    uld_u16_t  e_ehsize;
    uld_u16_t  e_phentsize;
    uld_u16_t  e_phnum;
    uld_u16_t  e_shentsize;
    uld_u16_t  e_shnum;
    uld_u16_t  e_shstrndx;
} __attribute__((packed)) uld_elf32_ehdr_t;

typedef struct {
    uld_u32_t  p_type;
    uld_u32_t  p_offset;
    uld_u32_t  p_vaddr;
    uld_u32_t  p_paddr;
    uld_u32_t  p_filesz;
    uld_u32_t  p_memsz;
    uld_u32_t  p_flags;
    uld_u32_t  p_align;
} __attribute__((packed)) uld_elf32_phdr_t;

typedef struct {
    uld_u32_t  sh_name;
    uld_u32_t  sh_type;
    uld_u32_t  sh_flags;
    uld_u32_t  sh_addr;
    uld_u32_t  sh_offset;
    uld_u32_t  sh_size;
    uld_u32_t  sh_link;
    uld_u32_t  sh_info;
    uld_u32_t  sh_addralign;
    uld_u32_t  sh_entsize;
} __attribute__((packed)) uld_elf32_shdr_t;

typedef struct {
    uld_u32_t  st_name;
    uld_u32_t  st_value;
    uld_u32_t  st_size;
    uld_u8_t   st_info;
    uld_u8_t   st_other;
    uld_u16_t  st_shndx;
} __attribute__((packed)) uld_elf32_sym_t;

/* -- ELF output context ------------------------------------- */
typedef struct uld_elf_ctx {
    uld_arch_t          arch;
    uld_addr_t          entry_addr;
    uld_sect_table_t   *sects;
    uld_sym_table_t    *syms;
    const char         *output_path;
    uld_diag_ctx_t     *diag;
} uld_elf_ctx_t;

int uld_elf64_write (uld_elf_ctx_t *ctx);
int uld_elf32_write (uld_elf_ctx_t *ctx);
int uld_flat_write  (uld_elf_ctx_t *ctx);
int uld_ihex_write  (uld_elf_ctx_t *ctx);
int uld_srec_write  (uld_elf_ctx_t *ctx);

#endif /* UIOX_LD_ELF_H */
