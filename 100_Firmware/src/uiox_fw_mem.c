/**
 * @file  uiox_fw_mem.c
 * @brief UIOX Firmware — memory map, memset/memcpy, early MMU.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 /* ── No-libc helpers ─────────────────────────────────────────────── */
 
 void *uiox_fw_memset(void *dst, int c, size_t n)
 { uint8_t *d = (uint8_t *)dst; while (n--) *d++ = (uint8_t)c; return dst; }
 
 void *uiox_fw_memcpy(void *dst, const void *src, size_t n)
 {
     uint8_t *d = (uint8_t *)dst;
     const uint8_t *s = (const uint8_t *)src;
     while (n--) *d++ = *s++;
     return dst;
 }
 
 int uiox_fw_memcmp(const void *a, const void *b, size_t n)
 {
     const uint8_t *p = (const uint8_t *)a;
     const uint8_t *q = (const uint8_t *)b;
     while (n--) { if (*p != *q) return (int)*p - (int)*q; p++; q++; }
     return 0;
 }
 
 size_t uiox_fw_strlen(const char *s)
 { size_t n = 0u; while (*s++) n++; return n; }
 
 /* ── Memory map probe ───────────────────────────────────────────────── */
 
 static void add_region(uiox_fw_mem_map_t *m,
                         uint64_t base, uint64_t size,
                         uiox_fw_mem_type_t type,
                         bool cacheable, bool exec, const char *name)
 {
     if (m->count >= UIOX_FW_MEM_MAX_REGIONS) return;
     uiox_fw_mem_region_t *r = &m->regions[m->count++];
     r->base       = base;
     r->size       = size;
     r->type       = type;
     r->cacheable  = cacheable;
     r->executable = exec;
     /* Copy name (no strncpy in bare-metal) */
     for (uint32_t i = 0u; i < 15u && name[i]; i++) r->name[i] = name[i];
     if (type == UIOX_FW_MEM_RAM) m->total_ram += size;
 }
 
 uiox_fw_err_t uiox_fw_mem_init(uiox_fw_mem_map_t *map, uint64_t dtb_pa)
 {
     UIOX_FW_UNUSED(dtb_pa);
     if (!map) return UIOX_FW_ERR_INVAL;
     uiox_fw_memset(map, 0, sizeof(*map));
 
 #if defined(__aarch64__)
     /* ARM64 QEMU virt */
     add_region(map, UIOX_MEM_ARM64_RAM_BASE, UIOX_MEM_ARM64_RAM_SIZE,
                UIOX_FW_MEM_RAM,  true,  true,  "DRAM");
     add_region(map, UIOX_MEM_ARM64_GIC_DIST, 0x10000ULL,
                UIOX_FW_MEM_MMIO, false, false, "GIC-DIST");
     add_region(map, UIOX_MEM_ARM64_GIC_CPU,  0x10000ULL,
                UIOX_FW_MEM_MMIO, false, false, "GIC-CPU");
     add_region(map, UIOX_MEM_ARM64_UART0,    0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "PL011-0");
     add_region(map, UIOX_MEM_ARM64_GPIO,     0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "GPIO");
     add_region(map, UIOX_MEM_ARM64_PCIE,     0x20000000ULL,
                UIOX_FW_MEM_MMIO, false, false, "PCIe");
 #elif defined(__arm__)
     /* ARM32 QEMU versatilepb */
     add_region(map, UIOX_MEM_ARM32_RAM_BASE, UIOX_MEM_ARM32_RAM_SIZE,
                UIOX_FW_MEM_RAM,  true,  true,  "RAM");
     add_region(map, UIOX_MEM_ARM32_UART0,    0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "PL011-0");
     add_region(map, UIOX_MEM_ARM32_TIMER0,   0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "SP804");
     add_region(map, UIOX_MEM_ARM32_VIC,      0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "VIC");
     add_region(map, UIOX_MEM_ARM32_GPIO,     0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "GPIO");
 #else
     /* x86_64 QEMU q35 */
     add_region(map, UIOX_MEM_X86_RAM_BASE, UIOX_MEM_X86_RAM_SIZE,
                UIOX_FW_MEM_RAM,  true,  true,  "RAM");
     add_region(map, UIOX_MEM_X86_LAPIC, 0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "LAPIC");
     add_region(map, UIOX_MEM_X86_IOAPIC, 0x1000ULL,
                UIOX_FW_MEM_MMIO, false, false, "IOAPIC");
     add_region(map, UIOX_MEM_X86_BIOS, 0x10000ULL,
                UIOX_FW_MEM_ROM,  false, false, "BIOS");
 #endif
     return UIOX_FW_OK;
 }
 
 void uiox_fw_mem_print(const uiox_fw_mem_map_t *map)
 {
     static const char *type_name[] = {
         "RAM","MMIO","ROM","RESERVED","FIRMWARE"
     };
     uiox_fw_printf("[FW] Memory map (%u regions, %llu MB RAM):\n",
                     map->count,
                     (unsigned long long)(map->total_ram >> 20u));
     for (uint32_t i = 0u; i < map->count; i++) {
         const uiox_fw_mem_region_t *r = &map->regions[i];
         uint8_t t = (uint8_t)r->type;
         uiox_fw_printf("  [%u] %016llx + %8llx  %-8s  %s\n",
                         i,
                         (unsigned long long)r->base,
                         (unsigned long long)r->size,
                         t < 5u ? type_name[t] : "?",
                         r->name);
     }
 }
 
 uiox_fw_err_t uiox_fw_mem_mmu_early(void)
 {
     /* Enable I-cache and D-cache via arch HAL */
     uiox_fw_hw_cache_enable();
     uiox_fw_hw_dsb();
     uiox_fw_hw_isb();
     return UIOX_FW_OK;
 }
 