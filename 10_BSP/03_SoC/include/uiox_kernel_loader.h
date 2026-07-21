/**
 * @file    uiox_kernel_loader.h
 * @brief   UIOX BSP — Dynamic kernel loader.
 *
 * Used when 10_BSP and the kernel are NOT compiled together.
 * The BSP locates the kernel image on storage, verifies it,
 * loads it into DRAM, and transfers control.
 *
 * Supported kernel image sources:
 *   1. Raw binary at a fixed flash / eMMC offset
 *   2. UIOX Image Format (UIF) — simple header + payload
 *   3. ELF64 / ELF32 executable
 *
 * Called from uiox_soc_main.c Stage 8 when
 * UIOX_DYNAMIC_KERNEL_LOAD is defined.
 */

 #ifndef UIOX_KERNEL_LOADER_H
 #define UIOX_KERNEL_LOADER_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * UIOX Image Format (UIF) header
  *
  * A minimal image header placed at the start of the kernel binary.
  * Inspired by U-Boot legacy image format but much simpler.
  * ====================================================================== */
 #define UIF_MAGIC           0x55494F58u   /* "UIOX"                      */
 #define UIF_VERSION         1u
 
 typedef enum {
     UIF_ARCH_ARM64   = 1,
     UIF_ARCH_ARM32   = 2,
     UIF_ARCH_X86_64  = 3,
     UIF_ARCH_RISCV64 = 4,
 } uif_arch_t;
 
 typedef enum {
     UIF_TYPE_KERNEL  = 1,   /* OS kernel                               */
     UIF_TYPE_INITRD  = 2,   /* Initial RAM disk                        */
     UIF_TYPE_DTB     = 3,   /* Device Tree Blob                        */
 } uif_type_t;
 
 typedef enum {
     UIF_COMP_NONE    = 0,   /* No compression                          */
     UIF_COMP_LZ4     = 1,   /* LZ4 (fast decompress on bare metal)     */
 } uif_comp_t;
 
 typedef struct __attribute__((packed)) {
     uiox_uint32_t  magic;           /* UIF_MAGIC                        */
     uiox_uint32_t  version;         /* UIF_VERSION                      */
     uiox_uint32_t  header_size;     /* sizeof(uif_header_t)             */
     uiox_uint32_t  payload_size;    /* compressed payload bytes         */
     uiox_uint32_t  load_addr_lo;    /* physical load address (lo 32)    */
     uiox_uint32_t  load_addr_hi;    /* physical load address (hi 32)    */
     uiox_uint32_t  entry_addr_lo;   /* entry point (lo 32)              */
     uiox_uint32_t  entry_addr_hi;   /* entry point (hi 32)              */
     uiox_uint32_t  data_size;       /* uncompressed size                */
     uiox_uint32_t  header_crc32;    /* CRC32 of header (this field = 0) */
     uiox_uint32_t  data_crc32;      /* CRC32 of uncompressed payload    */
     uiox_uint8_t   arch;            /* uif_arch_t                       */
     uiox_uint8_t   type;            /* uif_type_t                       */
     uiox_uint8_t   compression;     /* uif_comp_t                       */
     uiox_uint8_t   flags;           /* reserved, set to 0               */
     char           name[32];        /* human-readable image name        */
 } uif_header_t;
 
 /* =========================================================================
  * Kernel load descriptor
  *
  * Describes where to find and where to place the kernel image.
  * Populated by uiox_kernel_loader_init() from compile-time defaults
  * or a boot configuration block (BCB) in flash.
  * ====================================================================== */
 typedef struct {
     /* Source: storage location of the kernel image */
     uiox_uintptr_t  src_flash_base;  /* flash/eMMC physical offset      */
     uiox_size_t     src_max_size;    /* maximum bytes to read           */
 
     /* Destination: where to load in DRAM */
     uiox_uintptr_t  load_addr;       /* physical DRAM address           */
     uiox_uintptr_t  entry_addr;      /* kernel entry point              */
 
     /* DTB */
     uiox_uintptr_t  dtb_addr;        /* DTB physical address (0=none)   */
 
     /* Verification */
     uiox_bool_t     verify_crc;      /* verify CRC32 after load         */
     uiox_bool_t     verify_sig;      /* verify signature (secboot)      */
 
     /* Result — filled in by loader */
     uiox_size_t     loaded_bytes;
     uiox_bool_t     loaded;
 } uiox_kernel_desc_t;
 
 /* =========================================================================
  * Loader API
  * ====================================================================== */
 
 /**
  * Initialise the kernel descriptor with platform defaults.
  * Call once before uiox_kernel_load().
  */
 uiox_soc_err_t uiox_kernel_loader_init(uiox_kernel_desc_t *desc);
 
 /**
  * Read the kernel image from storage into DRAM.
  * Parses UIF header if present; falls back to raw binary load.
  * Sets desc->entry_addr and desc->dtb_addr.
  */
 uiox_soc_err_t uiox_kernel_load(uiox_kernel_desc_t *desc);
 
 /**
  * Verify the loaded kernel image (CRC32 + optional secboot signature).
  * Must be called after uiox_kernel_load() succeeds.
  */
 uiox_soc_err_t uiox_kernel_verify(uiox_kernel_desc_t *desc);
 
 /**
  * Transfer control to the kernel entry point.
  * This function never returns on success.
  *
  * @param desc      Populated and verified kernel descriptor.
  * @param dtb_pa    DTB physical address passed to kernel (0 if none).
  */
 void __attribute__((noreturn))
 uiox_kernel_jump(const uiox_kernel_desc_t *desc,
                   uiox_uint64_t             dtb_pa);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KERNEL_LOADER_H */
 