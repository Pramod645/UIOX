#ifndef UIOX_BOOT_MEM_H
#define UIOX_BOOT_MEM_H
/*
 * uiox_boot_mem.h - Memory discovery and bump allocator.
 */
#include "uiox_boot_types.h"

/* -- Memory region types ------------------------------------ */
typedef enum {
    UBOOT_MEM_USABLE   = 0,
    UBOOT_MEM_RESERVED = 1,
    UBOOT_MEM_FIRMWARE = 2,
    UBOOT_MEM_MMIO     = 3,
    UBOOT_MEM_BOOTLOADER = 4,
} uboot_mem_type_t;

typedef struct {
    uboot_u64_t     base;
    uboot_u64_t     size;
    uboot_mem_type_t type;
} uboot_mem_region_t;

typedef struct {
    uboot_mem_region_t regions[UBOOT_MEM_REGIONS_MAX];
    uboot_u32_t        count;
    uboot_u64_t        total_usable;
} uboot_mem_map_t;

#define UBOOT_MEM_REGIONS_MAX  UIOX_BOOT_MEM_REGIONS

/* -- Memory probe APIs (one per arch) ----------------------- */
int uboot_mem_probe_arm64(uboot_mem_map_t *map, uboot_u64_t dtb_phys);
int uboot_mem_probe_arm32(uboot_mem_map_t *map, uboot_u32_t *atags);
int uboot_mem_probe_x86  (uboot_mem_map_t *map, uboot_u32_t e820_ptr);

/* -- Bump allocator ----------------------------------------- */
void        uboot_heap_init  (uboot_addr_t base, uboot_size_t size);
void       *uboot_alloc      (uboot_size_t bytes, uboot_size_t align);
void        uboot_heap_reset (void);
uboot_size_t uboot_heap_used (void);

/* -- Utilities ---------------------------------------------- */
void uboot_mem_print(const uboot_mem_map_t *map);
void uboot_memset   (void *dst, uboot_u8_t val, uboot_size_t n);
void uboot_memcpy   (void *dst, const void *src, uboot_size_t n);
int  uboot_memcmp   (const void *a, const void *b, uboot_size_t n);

#endif /* UIOX_BOOT_MEM_H */
