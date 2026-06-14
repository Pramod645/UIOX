#ifndef UIOX_BOOT_HANDOFF_H
#define UIOX_BOOT_HANDOFF_H
/*
 * uiox_boot_handoff.h  —  ELF64 loader + kernel handoff.
 */
#include "uiox_boot_types.h"
#include "uiox_boot_mem.h"
#include "uiox_boot_hw.h"

/* ── ELF64 types ─────────────────────────────────────────── */
#define ELF_PT_LOAD  1u

typedef struct __attribute__((packed)) {
    uboot_u8_t   e_ident[16];
    uboot_u16_t  e_type;
    uboot_u16_t  e_machine;
    uboot_u32_t  e_version;
    uboot_u64_t  e_entry;
    uboot_u64_t  e_phoff;
    uboot_u64_t  e_shoff;
    uboot_u32_t  e_flags;
    uboot_u16_t  e_ehsize;
    uboot_u16_t  e_phentsize;
    uboot_u16_t  e_phnum;
    uboot_u16_t  e_shentsize;
    uboot_u16_t  e_shnum;
    uboot_u16_t  e_shstrndx;
} uboot_elf64_ehdr_t;

typedef struct __attribute__((packed)) {
    uboot_u32_t  p_type;
    uboot_u32_t  p_flags;
    uboot_u64_t  p_offset;
    uboot_u64_t  p_vaddr;
    uboot_u64_t  p_paddr;
    uboot_u64_t  p_filesz;
    uboot_u64_t  p_memsz;
    uboot_u64_t  p_align;
} uboot_elf64_phdr_t;

/* ── Boot arguments (passed to kernel in a known phys addr) ─ */
typedef struct {
    uboot_u32_t       magic;
    uboot_u32_t       version;
    uboot_u32_t       arch;
    uboot_u32_t       checksum;
    uboot_mem_map_t   mem_map;
    uboot_u64_t       kernel_phys;
    uboot_u64_t       kernel_size;
    uboot_u64_t       kernel_entry;
    uboot_u64_t       dtb_phys;
    uboot_u64_t       initrd_phys;
    uboot_u64_t       initrd_size;
    uboot_uart_type_t uart_type;
    uboot_u64_t       uart_base;
    uboot_u32_t       uart_baud;
    char              cmdline[UIOX_BOOT_CMDLINE];
} uiox_boot_args_t;

/* ── API ─────────────────────────────────────────────────── */
int  uboot_elf64_load    (const void *img, uboot_size_t size,
                           uboot_u64_t *entry_out);
void uboot_args_init     (uiox_boot_args_t *args);
void uboot_args_checksum (uiox_boot_args_t *args);
void uboot_args_print    (const uiox_boot_args_t *args);
void uboot_jump_to_kernel(uboot_u64_t entry, uboot_u64_t args_phys);

#endif /* UIOX_BOOT_HANDOFF_H */
