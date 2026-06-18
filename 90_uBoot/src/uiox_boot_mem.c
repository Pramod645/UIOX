/**
 * @file  uiox_boot_mem.c
 * @brief UIOX Bootloader — memory map probe (DTB/E820) + bump allocator.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * String / memory helpers (no libc)
  * ====================================================================== */
 
 void *uiox_boot_memset(void *dst, int c, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     while (n--) *d++ = (uint8_t)c;
     return dst;
 }
 
 void *uiox_boot_memcpy(void *dst, const void *src, size_t n)
 {
     uint8_t       *d = (uint8_t *)dst;
     const uint8_t *s = (const uint8_t *)src;
     while (n--) *d++ = *s++;
     return dst;
 }
 
 int uiox_boot_memcmp(const void *a, const void *b, size_t n)
 {
     const uint8_t *p = (const uint8_t *)a;
     const uint8_t *q = (const uint8_t *)b;
     while (n--) {
         if (*p != *q) return (int)*p - (int)*q;
         p++; q++;
     }
     return 0;
 }
 
 size_t uiox_boot_strlen(const char *s)
 {
     size_t n = 0u;
     while (*s++) n++;
     return n;
 }
 
 /* =========================================================================
  * FDT (Device Tree Blob) minimal parser
  * JEDEC reference: [github.com](https://github.com/devicetree-org/devicetree-specification)
  *
  * We only need /memory nodes (reg property).
  * ====================================================================== */
 
 #define FDT_BEGIN_NODE  0x00000001u
 #define FDT_END_NODE    0x00000002u
 #define FDT_PROP        0x00000003u
 #define FDT_NOP         0x00000004u
 #define FDT_END         0x00000009u
 
 static uint32_t fdt_be32(const uint8_t *p)
 {
     return ((uint32_t)p[0] << 24u) | ((uint32_t)p[1] << 16u)
          | ((uint32_t)p[2] <<  8u) |  (uint32_t)p[3];
 }
 
 static uint64_t fdt_be64(const uint8_t *p)
 {
     return ((uint64_t)fdt_be32(p) << 32u) | (uint64_t)fdt_be32(p + 4u);
 }
 
 /* Minimal FDT header */
 typedef struct {
     uint32_t magic;          /* 0xD00DFEED */
     uint32_t totalsize;
     uint32_t off_dt_struct;
     uint32_t off_dt_strings;
     uint32_t off_mem_rsvmap;
     uint32_t version;
     uint32_t last_comp_version;
     uint32_t boot_cpuid_phys;
     uint32_t size_dt_strings;
     uint32_t size_dt_struct;
 } uiox_fdt_hdr_t;
 
 static uiox_boot_err_t fdt_parse_memory(const uint8_t *fdt,
                                           uiox_mem_map_t *map)
 {
     const uiox_fdt_hdr_t *hdr = (const uiox_fdt_hdr_t *)fdt;
     if (fdt_be32((const uint8_t *)&hdr->magic) != UIOX_DTB_MAGIC)
         return UIOX_BOOT_ERR_BADMAGIC;
 
     uint32_t struct_off   = fdt_be32((const uint8_t *)&hdr->off_dt_struct);
     uint32_t strings_off  = fdt_be32((const uint8_t *)&hdr->off_dt_strings);
     const uint8_t *p      = fdt + struct_off;
     const char    *strtab = (const char *)(fdt + strings_off);
 
     bool in_memory_node = false;
     map->count          = 0u;
     map->total_usable   = 0u;
 
     while (1) {
         uint32_t token = fdt_be32(p); p += 4u;
         switch (token) {
         case FDT_BEGIN_NODE: {
             /* Node name follows as NUL-terminated string */
             const char *name = (const char *)p;
             /* Check if it starts with "memory" */
             in_memory_node =
                 (name[0]=='m' && name[1]=='e' && name[2]=='m' &&
                  name[3]=='o' && name[4]=='r' && name[5]=='y');
             /* Skip name (rounded to 4-byte boundary) */
             size_t nlen = uiox_boot_strlen(name) + 1u;
             p += UIOX_ALIGN_UP(nlen, 4u);
             break;
         }
         case FDT_END_NODE:
             in_memory_node = false;
             break;
         case FDT_PROP: {
             uint32_t len    = fdt_be32(p);     p += 4u;
             uint32_t nameoff= fdt_be32(p);     p += 4u;
             const uint8_t *val = p;
             p += UIOX_ALIGN_UP(len, 4u);
             if (in_memory_node) {
                 const char *pname = strtab + nameoff;
                 /* "reg" property: pairs of (base, size) each 8 bytes */
                 if (pname[0]=='r' && pname[1]=='e' && pname[2]=='g'
                     && pname[3]=='\0') {
                     uint32_t pairs = len / 16u;
                     for (uint32_t i = 0u;
                          i < pairs && map->count < UIOX_MEM_MAX_REGIONS;
                          i++) {
                         uiox_mem_region_t *r = &map->regions[map->count++];
                         r->base = fdt_be64(val + i * 16u);
                         r->size = fdt_be64(val + i * 16u + 8u);
                         r->type = UIOX_MEM_USABLE;
                         map->total_usable += r->size;
                     }
                 }
             }
             break;
         }
         case FDT_NOP:
             break;
         case FDT_END:
         default:
             goto done;
         }
     }
 done:
     return (map->count > 0u) ? UIOX_BOOT_OK : UIOX_BOOT_ERR_NOTFOUND;
 }
 
 /* =========================================================================
  * E820-style fallback (x86 Multiboot2 memory map tag)
  * ====================================================================== */
 
 static uiox_boot_err_t probe_fallback(uiox_mem_map_t *map)
 {
     /* Provide a single DRAM region: 64 MB starting at 0x40000000
      * This matches the QEMU virt defaults used in UIOX build config.
      * A real x86 driver would walk the Multiboot2 mmap tags. */
 #if defined(__aarch64__) || defined(__arm__)
     map->count = 1u;
     map->regions[0].base  = 0x40000000u;
     map->regions[0].size  = 64u * 1024u * 1024u;
     map->regions[0].type  = UIOX_MEM_USABLE;
     map->total_usable     = map->regions[0].size;
 #else
     /* x86_64: 128 MB at 1 MB */
     map->count = 2u;
     map->regions[0].base  = 0x00100000u;  /* 1 MB → 64 MB */
     map->regions[0].size  = 63u * 1024u * 1024u;
     map->regions[0].type  = UIOX_MEM_USABLE;
     map->regions[1].base  = 0x40000000u;  /* 1 GB → 64 MB */
     map->regions[1].size  = 64u * 1024u * 1024u;
     map->regions[1].type  = UIOX_MEM_USABLE;
     map->total_usable     = map->regions[0].size + map->regions[1].size;
 #endif
     return UIOX_BOOT_OK;
 }
 
 uiox_boot_err_t uiox_boot_mem_probe(uint64_t dtb_pa, uiox_mem_map_t *map)
 {
     uiox_boot_memset(map, 0, sizeof(*map));
     if (dtb_pa != 0u) {
         uiox_boot_err_t rc =
             fdt_parse_memory((const uint8_t *)(uintptr_t)dtb_pa, map);
         if (rc == UIOX_BOOT_OK) return rc;
     }
     return probe_fallback(map);
 }
 
 void uiox_boot_mem_print(const uiox_mem_map_t *map)
 {
     static const char *type_names[] = {
         "USABLE", "RESERVED", "FIRMWARE", "MMIO", "ACPI", "BAD"
     };
     uiox_boot_printf("Memory map (%u regions):\n", map->count);
     for (uint32_t i = 0u; i < map->count; i++) {
         const uiox_mem_region_t *r = &map->regions[i];
         uint8_t t = (uint8_t)r->type;
         uiox_boot_printf("  base=%016llx size=%016llx %s\n",
                           (unsigned long long)r->base,
                           (unsigned long long)r->size,
                           t < UIOX_ARRAY_SIZE(type_names)
                               ? type_names[t] : "?");
     }
     uiox_boot_printf("Usable: %llu MB\n",
                       (unsigned long long)(map->total_usable >> 20u));
 }
 
 uiox_boot_err_t uiox_boot_mem_alloc_init(uiox_bump_alloc_t *a,
                                           const uiox_mem_map_t *map,
                                           uintptr_t reserved_start,
                                           size_t    reserved_size)
 {
     /* Find first usable region large enough for our bump arena */
     for (uint32_t i = 0u; i < map->count; i++) {
         const uiox_mem_region_t *r = &map->regions[i];
         if (r->type != UIOX_MEM_USABLE) continue;
         if (r->size < (reserved_size + 256u * 1024u)) continue;
 
         uintptr_t base = (uintptr_t)r->base;
         /* Skip the reserved range if it overlaps */
         if (reserved_start >= base &&
             reserved_start < base + (uintptr_t)r->size)
             base = UIOX_ALIGN_UP(reserved_start + reserved_size, 4096u);
 
         a->base  = base;
         a->top   = base;
         a->limit = (uintptr_t)r->base + (uintptr_t)r->size;
         return UIOX_BOOT_OK;
     }
     return UIOX_BOOT_ERR_NOMEM;
 }
 
 void *uiox_boot_mem_alloc(uiox_bump_alloc_t *a, size_t size, size_t align)
 {
     uintptr_t ptr = UIOX_ALIGN_UP(a->top, align);
     if (ptr + size > a->limit) return NULL;
     a->top = ptr + size;
     uiox_boot_memset((void *)ptr, 0, size);
     return (void *)ptr;
 }
 