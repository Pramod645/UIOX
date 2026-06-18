/**
 * @file  uiox_boot_mem.h
 * @brief UIOX Bootloader — memory map probe and bump allocator.
 *
 * Probes the platform memory map from DTB (ARM) or E820 (x86), builds a
 * region table, and exposes a simple bump allocator for boot-time objects.
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_MEM_H
 #define UIOX_BOOT_MEM_H
 
 #include "uiox_boot_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Memory region types
  * ====================================================================== */
 
 typedef enum {
     UIOX_MEM_USABLE     = 0,
     UIOX_MEM_RESERVED   = 1,
     UIOX_MEM_FIRMWARE   = 2,
     UIOX_MEM_MMIO       = 3,
     UIOX_MEM_ACPI       = 4,
     UIOX_MEM_BAD        = 5,
 } uiox_mem_type_t;
 
 #define UIOX_MEM_MAX_REGIONS    32u
 
 typedef struct {
     uint64_t       base;
     uint64_t       size;
     uiox_mem_type_t type;
 } uiox_mem_region_t;
 
 typedef struct {
     uiox_mem_region_t regions[UIOX_MEM_MAX_REGIONS];
     uint32_t          count;
     uint64_t          total_usable;  /**< Bytes of usable RAM             */
 } uiox_mem_map_t;
 
 /* =========================================================================
  * Bump allocator (boot-time only, never freed)
  * ====================================================================== */
 
 typedef struct {
     uintptr_t base;   /**< Start of allocation arena                      */
     uintptr_t top;    /**< Current allocation pointer                     */
     uintptr_t limit;  /**< Hard limit (end of arena)                      */
 } uiox_bump_alloc_t;
 
 /* =========================================================================
  * API
  * ====================================================================== */
 
 /** Probe memory from DTB FDT blob at @dtb_pa (0 = use ATAG/E820). */
 uiox_boot_err_t uiox_boot_mem_probe(uint64_t dtb_pa, uiox_mem_map_t *map);
 
 /** Print the memory map to the boot console. */
 void uiox_boot_mem_print(const uiox_mem_map_t *map);
 
 /** Initialise the bump allocator from the first usable region. */
 uiox_boot_err_t uiox_boot_mem_alloc_init(uiox_bump_alloc_t *a,
                                           const uiox_mem_map_t *map,
                                           uintptr_t reserved_start,
                                           size_t    reserved_size);
 
 /** Allocate @size bytes aligned to @align from the bump allocator. */
 void *uiox_boot_mem_alloc(uiox_bump_alloc_t *a, size_t size, size_t align);
 
 /** Minimal memset / memcpy for boot use (no libc). */
 void *uiox_boot_memset (void *dst, int c, size_t n);
 void *uiox_boot_memcpy (void *dst, const void *src, size_t n);
 int   uiox_boot_memcmp (const void *a, const void *b, size_t n);
 
 /** Minimal strlen. */
 size_t uiox_boot_strlen(const char *s);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BOOT_MEM_H */
 