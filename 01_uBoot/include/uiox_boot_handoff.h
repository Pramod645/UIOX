/**
 * @file  uiox_boot_handoff.h
 * @brief UIOX Bootloader — ELF64 loader, boot-args struct, kernel jump.
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_HANDOFF_H
 #define UIOX_BOOT_HANDOFF_H
 
 #include "uiox_boot_types.h"
 #include "uiox_boot_mem.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * ELF64 minimal structures
  * ====================================================================== */
 
 #define ELF64_MAGIC             0x464C457Fu  /**< "\x7FELF"               */
 #define ELF_CLASS_64            2u
 #define ELF_DATA_2LSB           1u
 #define ELF_TYPE_EXEC           2u
 #define ELF_ARCH_AARCH64        0xB7u
 #define ELF_ARCH_X86_64         0x3Eu
 #define ELF_ARCH_ARM            0x28u
 #define PT_LOAD                 1u
 #define PF_X                    0x1u
 #define PF_W                    0x2u
 #define PF_R                    0x4u
 
 typedef struct __attribute__((packed)) {
     uint8_t  e_ident[16];
     uint16_t e_type;
     uint16_t e_machine;
     uint32_t e_version;
     uint64_t e_entry;
     uint64_t e_phoff;
     uint64_t e_shoff;
     uint32_t e_flags;
     uint16_t e_ehsize;
     uint16_t e_phentsize;
     uint16_t e_phnum;
     uint16_t e_shentsize;
     uint16_t e_shnum;
     uint16_t e_shstrndx;
 } uiox_elf64_ehdr_t;
 
 typedef struct __attribute__((packed)) {
     uint32_t p_type;
     uint32_t p_flags;
     uint64_t p_offset;
     uint64_t p_vaddr;
     uint64_t p_paddr;
     uint64_t p_filesz;
     uint64_t p_memsz;
     uint64_t p_align;
 } uiox_elf64_phdr_t;
 
 /* =========================================================================
  * Boot arguments structure (passed to the UIOX kernel)
  * Placed at a well-known physical address below the kernel load address.
  * ====================================================================== */
 
 #define UIOX_BOOT_ARGS_VERSION  1u
 
 typedef struct {
     uint32_t         magic;           /**< UIOX_BOOT_ARGS_MAGIC           */
     uint32_t         version;
     uint64_t         kernel_entry;    /**< Physical kernel entry point     */
     uint64_t         dtb_pa;          /**< Physical address of DTB / 0    */
     uint64_t         initrd_pa;       /**< Physical address of initrd / 0 */
     uint64_t         initrd_size;
     uint64_t         args_pa;         /**< Self physical address          */
     uiox_mem_map_t   mem_map;
     char             cmdline[UIOX_IMAGE_CMDLINE_MAX];
     uiox_arch_t      arch;
     uint8_t          _pad[28];        /**< Pad to 512 bytes               */
 } uiox_boot_args_t;
 
 /* =========================================================================
  * ELF loader API
  * ====================================================================== */
 
 /**
  * Parse an ELF64 image and load all PT_LOAD segments to their physical
  * addresses.  Returns the entry point physical address.
  */
 uiox_boot_err_t uiox_boot_elf64_load(const void *elf_buf, size_t elf_size,
                                        uint64_t *entry_pa);
 
 /**
  * Flat binary load: copy @size bytes from @src to @dest_pa.
  */
 uiox_boot_err_t uiox_boot_flat_load(const void *src, size_t size,
                                       uintptr_t dest_pa);
 
 /* =========================================================================
  * Handoff API
  * ====================================================================== */
 
 /**
  * Build the boot-args struct at @args_pa and transfer control to
  * @kernel_entry.  Never returns.
  */
 void uiox_boot_handoff(uint64_t kernel_entry,
                         uint64_t dtb_pa,
                         uint64_t args_pa,
                         const uiox_mem_map_t *mem_map,
                         const char *cmdline)
      __attribute__((noreturn));
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_HANDOFF_H */
 