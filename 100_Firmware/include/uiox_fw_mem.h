/**
 * @file  uiox_fw_mem.h
 * @brief UIOX Firmware — Physical memory map and early MMU/MPU init.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_MEM_H
 #define UIOX_FW_MEM_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_MEM_MAX_REGIONS   32u
 
 typedef enum {
     UIOX_FW_MEM_RAM      = 0,
     UIOX_FW_MEM_MMIO     = 1,
     UIOX_FW_MEM_ROM      = 2,
     UIOX_FW_MEM_RESERVED = 3,
     UIOX_FW_MEM_FIRMWARE = 4,
 } uiox_fw_mem_type_t;
 
 typedef struct {
     uint64_t           base;
     uint64_t           size;
     uiox_fw_mem_type_t type;
     bool               cacheable;
     bool               executable;
     char               name[16];
 } uiox_fw_mem_region_t;
 
 typedef struct {
     uiox_fw_mem_region_t regions[UIOX_FW_MEM_MAX_REGIONS];
     uint32_t             count;
     uint64_t             total_ram;
 } uiox_fw_mem_map_t;
 
 /* Platform memory map constants (match linker scripts from 80_linker/) */
 
 /* ARM64 QEMU virt */
 #define UIOX_MEM_ARM64_RAM_BASE     0x40000000ULL
 #define UIOX_MEM_ARM64_RAM_SIZE     0x04000000ULL   /* 64 MB default      */
 #define UIOX_MEM_ARM64_GIC_DIST     0x08000000ULL
 #define UIOX_MEM_ARM64_GIC_CPU      0x08010000ULL
 #define UIOX_MEM_ARM64_UART0        0x09000000ULL
 #define UIOX_MEM_ARM64_TIMER        0x09010000ULL
 #define UIOX_MEM_ARM64_GPIO         0x09030000ULL
 #define UIOX_MEM_ARM64_PCIE         0x10000000ULL
 
 /* ARM32 QEMU versatilepb */
 #define UIOX_MEM_ARM32_RAM_BASE     0x00100000ULL
 #define UIOX_MEM_ARM32_RAM_SIZE     0x00F00000ULL   /* 15 MB              */
 #define UIOX_MEM_ARM32_UART0        0x101F1000ULL
 #define UIOX_MEM_ARM32_TIMER0       0x101E2000ULL
 #define UIOX_MEM_ARM32_VIC          0x10140000ULL
 #define UIOX_MEM_ARM32_GPIO         0x101E4000ULL
 
 /* x86_64 QEMU q35 */
 #define UIOX_MEM_X86_RAM_BASE       0x00100000ULL
 #define UIOX_MEM_X86_RAM_SIZE       0x03F00000ULL   /* 63 MB              */
 #define UIOX_MEM_X86_LAPIC          0xFEE00000ULL
 #define UIOX_MEM_X86_IOAPIC         0xFEC00000ULL
 #define UIOX_MEM_X86_BIOS           0xFFFF0000ULL
 
 /* API */
 uiox_fw_err_t uiox_fw_mem_init       (uiox_fw_mem_map_t *map,
                                         uint64_t dtb_pa);
 void          uiox_fw_mem_print      (const uiox_fw_mem_map_t *map);
 uiox_fw_err_t uiox_fw_mem_mmu_early  (void);  /**< Enable I/D caches     */
 
 /* memset / memcpy (no libc) */
 void *uiox_fw_memset  (void *dst, int c, size_t n);
 void *uiox_fw_memcpy  (void *dst, const void *src, size_t n);
 int   uiox_fw_memcmp  (const void *a, const void *b, size_t n);
 size_t uiox_fw_strlen (const char *s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_MEM_H */
 