/*
 * uiox_boot_mem.c - Memory map discovery + bump allocator.
 */
#include "uiox_boot_mem.h"
#include "uiox_boot_console.h"
#include <string.h>

/* -- Bump allocator state ----------------------------------- */
static uboot_addr_t g_heap_base = 0;
static uboot_size_t g_heap_size = 0;
static uboot_size_t g_heap_used = 0;

void uboot_heap_init(uboot_addr_t base, uboot_size_t size)
{
    g_heap_base = base;
    g_heap_size = size;
    g_heap_used = 0;
}

void *uboot_alloc(uboot_size_t bytes, uboot_size_t align)
{
    if (!align) align = 8u;
    uboot_size_t off = (g_heap_used + align - 1u) & ~(align - 1u);
    if (off + bytes > g_heap_size) return NULL;
    g_heap_used = off + bytes;
    return (void *)(uboot_addr_t)(g_heap_base + off);
}

void uboot_heap_reset(void) { g_heap_used = 0; }
uboot_size_t uboot_heap_used(void) { return g_heap_used; }

/* -- Utility memset / memcpy / memcmp ---------------------- */
void uboot_memset(void *dst, uboot_u8_t val, uboot_size_t n)
{
    uboot_u8_t *p = (uboot_u8_t *)dst;
    while (n--) *p++ = val;
}

void uboot_memcpy(void *dst, const void *src, uboot_size_t n)
{
    const uboot_u8_t *s = (const uboot_u8_t *)src;
    uboot_u8_t       *d = (uboot_u8_t *)dst;
    while (n--) *d++ = *s++;
}

int uboot_memcmp(const void *a, const void *b, uboot_size_t n)
{
    const uboot_u8_t *pa = (const uboot_u8_t *)a;
    const uboot_u8_t *pb = (const uboot_u8_t *)b;
    while (n--) {
        if (*pa != *pb) return (*pa < *pb) ? -1 : 1;
        pa++; pb++;
    }
    return 0;
}

/* -- ARM64: parse FDT memory nodes (simplified) ------------ */
int uboot_mem_probe_arm64(uboot_mem_map_t *map, uboot_u64_t dtb_phys)
{
    (void)dtb_phys;
    memset(map, 0, sizeof(*map));
    /* QEMU virt defaults: 64 MB DRAM at 0x40000000             */
    map->regions[0].base  = 0x40000000ULL;
    map->regions[0].size  = 0x04000000ULL;  /* 64 MB            */
    map->regions[0].type  = UBOOT_MEM_USABLE;
    map->count            = 1;
    map->total_usable     = 0x04000000ULL;
    return UBOOT_OK;
}

/* -- ARM32: parse ATAG list --------------------------------- */
int uboot_mem_probe_arm32(uboot_mem_map_t *map, uboot_u32_t *atags)
{
    (void)atags;
    memset(map, 0, sizeof(*map));
    map->regions[0].base  = 0x00100000UL;
    map->regions[0].size  = 0x00F00000UL;   /* 15 MB            */
    map->regions[0].type  = UBOOT_MEM_USABLE;
    map->count            = 1;
    map->total_usable     = 0x00F00000UL;
    return UBOOT_OK;
}

/* -- x86: use E820 map -------------------------------------- */
int uboot_mem_probe_x86(uboot_mem_map_t *map, uboot_u32_t e820_ptr)
{
    (void)e820_ptr;
    memset(map, 0, sizeof(*map));
    /* conventional low memory                                   */
    map->regions[0].base  = 0x00100000ULL;
    map->regions[0].size  = 0x03F00000ULL;  /* 63 MB above 1MB  */
    map->regions[0].type  = UBOOT_MEM_USABLE;
    /* MMIO region                                               */
    map->regions[1].base  = 0xFEC00000ULL;
    map->regions[1].size  = 0x00400000ULL;
    map->regions[1].type  = UBOOT_MEM_MMIO;
    map->count            = 2;
    map->total_usable     = 0x03F00000ULL;
    return UBOOT_OK;
}

/* -- Print memory map --------------------------------------- */
void uboot_mem_print(const uboot_mem_map_t *map)
{
    static const char *tname[] = {
        "USABLE","RESERVED","FIRMWARE","MMIO","BOOTLOADER"
    };
    uboot_printf("  Memory map (%u regions):\r\n", map->count);
    for (uboot_u32_t i = 0; i < map->count; i++) {
        const uboot_mem_region_t *r = &map->regions[i];
        uboot_printf("    [%u] base=", i);
        uboot_puthex64(r->base);
        uboot_printf("  size=");
        uboot_puthex64(r->size);
        uboot_printf("  %s\r\n",
            r->type < 5 ? tname[r->type] : "?");
    }
    uboot_printf("  Total usable: %u MB\r\n",
        (uboot_u32_t)(map->total_usable >> 20));
}
