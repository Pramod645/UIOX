/**
 * @file    uiox_bios_svc.c
 * @brief   UIOX BIOS services implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_svc.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 /* POST code → message table */
 static const struct { uint32_t code; const char *msg; }
 s_post_codes[] = {
     { 0x01u, "CPU reset"            },
     { 0x10u, "Memory init"          },
     { 0x20u, "Chipset init"         },
     { 0x30u, "PCI enumeration"      },
     { 0x40u, "ACPI init"            },
     { 0x50u, "NVRAM init"           },
     { 0x60u, "Option ROM"           },
     { 0x70u, "Boot device select"   },
     { 0x00u, "POST complete"        },
     { 0xFFu, "POST error"           },
 };
 
 static void set_post(uiox_bios_svc_t *svc,
                       uint32_t code, const char *msg)
 {
     svc->post_code = code;
     strncpy(svc->post_msg, msg, sizeof(svc->post_msg) - 1);
 }
 
 int uiox_bios_svc_init(uiox_bios_svc_t *svc, uiox_bios_nvram_t *nvram)
 {
     if (!svc || !nvram) return -EINVAL;
     memset(svc, 0, sizeof(*svc));
     svc->nvram      = nvram;
     svc->post_phase = UIOX_POST_PHASE_RESET;
     return 0;
 }
 
 int uiox_bios_svc_post(uiox_bios_svc_t *svc)
 {
     if (!svc) return -EINVAL;
     svc->post_ok = false;
 
     /* Phase 1: CPU / Reset vector */
     svc->post_phase = UIOX_POST_PHASE_RESET;
     set_post(svc, 0x01u, "CPU reset");
 
     /* Phase 2: Memory init */
     svc->post_phase = UIOX_POST_PHASE_MEMORY_INIT;
     set_post(svc, 0x10u, "Memory init");
     svc->total_ram_mb = 8192u;  /* Stub: 8 GB */
 
     /* Phase 3: Chipset */
     svc->post_phase = UIOX_POST_PHASE_CHIPSET_INIT;
     set_post(svc, 0x20u, "Chipset init");
 
     /* Phase 4: PCI enumeration */
     svc->post_phase = UIOX_POST_PHASE_PCI_ENUM;
     set_post(svc, 0x30u, "PCI enumeration");
 
     /* Phase 5: ACPI */
     svc->post_phase = UIOX_POST_PHASE_ACPI_INIT;
     set_post(svc, 0x40u, "ACPI init");
 
     /* Phase 6: NVRAM */
     svc->post_phase = UIOX_POST_PHASE_NVRAM_INIT;
     set_post(svc, 0x50u, "NVRAM init");
     uiox_bios_nvram_load(svc->nvram);
 
     /* Phase 7: Option ROM */
     svc->post_phase = UIOX_POST_PHASE_OPTION_ROM;
     set_post(svc, 0x60u, "Option ROM scan");
 
     /* Phase 8: Boot device selection */
     svc->post_phase = UIOX_POST_PHASE_BOOT_SELECT;
     set_post(svc, 0x70u, "Boot device select");
     uiox_bios_svc_select_boot(svc);
 
     svc->post_phase = UIOX_POST_PHASE_DONE;
     set_post(svc, 0x00u, "POST complete");
     svc->post_ok = true;
     return 0;
 }
 
 int uiox_bios_svc_build_memmap(uiox_bios_svc_t *svc,
                                 uint64_t ram_base, uint64_t ram_bytes)
 {
     if (!svc) return -EINVAL;
     uiox_e820_map_t *m = &svc->memmap;
     m->count = 0;
 
     /* Conventional memory 0..640 KB */
     m->entries[m->count++] = (uiox_e820_entry_t){
         .base = 0x00000000ULL, .length = 0x000A0000ULL,
         .type = UIOX_E820_USABLE };
 
     /* Video / ROM 640 KB..1 MB */
     m->entries[m->count++] = (uiox_e820_entry_t){
         .base = 0x000A0000ULL, .length = 0x00060000ULL,
         .type = UIOX_E820_RESERVED };
 
     /* Extended RAM */
     m->entries[m->count++] = (uiox_e820_entry_t){
         .base = ram_base, .length = ram_bytes,
         .type = UIOX_E820_USABLE };
 
     /* ACPI reclaimable at top of RAM */
     m->entries[m->count++] = (uiox_e820_entry_t){
         .base = ram_base + ram_bytes,
         .length = 0x00100000ULL,
         .type = UIOX_E820_ACPI_RECLAIM };
 
     /* Flash / BIOS ROM */
     m->entries[m->count++] = (uiox_e820_entry_t){
         .base = 0xFF000000ULL, .length = 0x01000000ULL,
         .type = UIOX_E820_FIRMWARE };
 
     return 0;
 }
 
 int uiox_bios_svc_find_acpi(uiox_bios_svc_t *svc,
                               uint64_t search_base, uint64_t search_size)
 {
     if (!svc) return -EINVAL;
     /* Stub: in production scan physical memory for "RSD PTR " signature */
     svc->acpi.rsdp_phys = search_base + 0x40u;
     svc->acpi.rsdt_phys = search_base + 0x1000u;
     svc->acpi.madt_phys = search_base + 0x2000u;
     svc->acpi.fadt_phys = search_base + 0x3000u;
     svc->acpi.dsdt_phys = search_base + 0x4000u;
     svc->acpi.valid     = true;
     (void)search_size;
     return 0;
 }
 
 /* PCI config space via I/O ports 0xCF8/0xCFC (x86) */
 static uint32_t pci_addr(uint8_t bus, uint8_t dev,
                            uint8_t fn, uint8_t reg)
 {
     return (1u << 31u) |
            ((uint32_t)bus << 16u) |
            ((uint32_t)(dev & 0x1Fu) << 11u) |
            ((uint32_t)(fn  & 0x07u) <<  8u) |
            (reg & 0xFCu);
 }
 
 uint32_t uiox_bios_svc_pci_read(uiox_bios_svc_t *svc,
                                   uint8_t bus, uint8_t dev,
                                   uint8_t fn, uint8_t reg)
 {
     (void)svc;
     (void)pci_addr(bus, dev, fn, reg);
     /* Stub: on real hardware write addr to 0xCF8, read 0xCFC */
     return 0xFFFFFFFFu;
 }
 
 void uiox_bios_svc_pci_write(uiox_bios_svc_t *svc,
                                uint8_t bus, uint8_t dev,
                                uint8_t fn, uint8_t reg, uint32_t val)
 {
     (void)svc; (void)bus; (void)dev; (void)fn; (void)reg; (void)val;
 }
 
 int uiox_bios_svc_select_boot(uiox_bios_svc_t *svc)
 {
     if (!svc || !svc->nvram) return -EINVAL;
     /* Read BootOrder from NVRAM */
     static const uiox_efi_guid_t global = UIOX_GUID_GLOBAL;
     uint16_t boot_order[8];
     uint32_t size = sizeof(boot_order);
     int rc = uiox_bios_nvram_get_var(svc->nvram, &global, "BootOrder",
                                       boot_order, &size, NULL);
     if (rc == 0 && size >= 2u)
         svc->boot_device = UIOX_BOOT_DEVICE_SSD_NVME; /* stub */
     else
         svc->boot_device = UIOX_BOOT_DEVICE_HDD;
     return 0;
 }
 
 void uiox_bios_svc_print_post(const uiox_bios_svc_t *svc)
 {
     if (!svc) return;
     static const char *phases[] = {
         "RESET","MEM_INIT","CHIPSET","PCI_ENUM","ACPI","NVRAM",
         "OPTION_ROM","BOOT_SEL","DONE","ERROR"
     };
     printf("  POST phase     : %s\n",
            phases[svc->post_phase < 10u ? svc->post_phase : 9u]);
     printf("  POST code      : 0x%02X  %s\n",
            svc->post_code, svc->post_msg);
     printf("  POST result    : %s\n", svc->post_ok ? "OK" : "FAIL");
     printf("  Total RAM      : %u MB\n", svc->total_ram_mb);
     static const char *bdev_names[] = {
         "NONE","HDD","NVMe","USB","PXE","OPTICAL","UEFI_SHELL"
     };
     printf("  Boot device    : %s\n",
            bdev_names[svc->boot_device < 7u ? svc->boot_device : 0u]);
 }
 
 void uiox_bios_svc_print_memmap(const uiox_bios_svc_t *svc)
 {
     if (!svc) return;
     static const char *types[] = {
         "?","USABLE","RESERVED","ACPI_RECLAIM","ACPI_NVS","BAD","FIRMWARE"
     };
     printf("  E820 memory map (%u entries):\n", svc->memmap.count);
     for (uint8_t i = 0; i < svc->memmap.count; i++) {
         const uiox_e820_entry_t *e = &svc->memmap.entries[i];
         uint8_t t = (uint8_t)(e->type < 7u ? e->type : 0u);
         printf("    [%u] base=0x%016llX  size=%7llu KB  %s\n",
                i, (unsigned long long)e->base,
                (unsigned long long)(e->length / 1024u),
                types[t]);
     }
 }
 