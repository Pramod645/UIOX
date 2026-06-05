/**
 * @file    uiox_bios_svc.h
 * @brief   UIOX BIOS services: POST, memory map, ACPI, PCI enum.
 *
 * Provides:
 *   - Power-On Self-Test (POST) execution pipeline
 *   - E820 / UEFI memory map construction
 *   - ACPI table location and validation (RSDP, RSDT, MADT, FADT)
 *   - PCI device enumeration (config space read/write)
 *   - CPU microcode update dispatch
 *   - Option ROM (OROM) execution stub
 *   - Boot device selection and boot attempt
 *
 * @date    2026-06-04
 */
//Layer 3 — BIOS Services
 #ifndef UIOX_BIOS_SVC_H
 #define UIOX_BIOS_SVC_H
 
 #include "uiox_bios_nvram.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * POST phase flags
  * ====================================================================== */
 
 typedef enum {
     UIOX_POST_PHASE_RESET       = 0,
     UIOX_POST_PHASE_MEMORY_INIT,
     UIOX_POST_PHASE_CHIPSET_INIT,
     UIOX_POST_PHASE_PCI_ENUM,
     UIOX_POST_PHASE_ACPI_INIT,
     UIOX_POST_PHASE_NVRAM_INIT,
     UIOX_POST_PHASE_OPTION_ROM,
     UIOX_POST_PHASE_BOOT_SELECT,
     UIOX_POST_PHASE_DONE,
     UIOX_POST_PHASE_ERROR,
 } uiox_post_phase_t;
 
 /* =========================================================================
  * E820 memory map entry
  * ====================================================================== */
 
 #define UIOX_E820_MAX_ENTRIES   32
 
 typedef enum {
     UIOX_E820_USABLE      = 1,
     UIOX_E820_RESERVED    = 2,
     UIOX_E820_ACPI_RECLAIM= 3,
     UIOX_E820_ACPI_NVS    = 4,
     UIOX_E820_BAD         = 5,
     UIOX_E820_FIRMWARE    = 6,
 } uiox_e820_type_t;
 
 typedef struct {
     uint64_t       base;
     uint64_t       length;
     uiox_e820_type_t type;
 } uiox_e820_entry_t;
 
 typedef struct {
     uiox_e820_entry_t entries[UIOX_E820_MAX_ENTRIES];
     uint8_t           count;
 } uiox_e820_map_t;
 
 /* =========================================================================
  * ACPI table locator
  * ====================================================================== */
 
 #define UIOX_ACPI_SIG_RSDP  "RSD PTR "
 #define UIOX_ACPI_SIG_RSDT  "RSDT"
 #define UIOX_ACPI_SIG_XSDT  "XSDT"
 #define UIOX_ACPI_SIG_MADT  "APIC"
 #define UIOX_ACPI_SIG_FADT  "FACP"
 #define UIOX_ACPI_SIG_DSDT  "DSDT"
 
 typedef struct {
     uint64_t rsdp_phys;
     uint64_t rsdt_phys;
     uint64_t xsdt_phys;
     uint64_t madt_phys;
     uint64_t fadt_phys;
     uint64_t dsdt_phys;
     bool     valid;
 } uiox_acpi_info_t;
 
 /* =========================================================================
  * Boot device
  * ====================================================================== */
 
 typedef enum {
     UIOX_BOOT_DEVICE_NONE   = 0,
     UIOX_BOOT_DEVICE_HDD,
     UIOX_BOOT_DEVICE_SSD_NVME,
     UIOX_BOOT_DEVICE_USB,
     UIOX_BOOT_DEVICE_PXE,
     UIOX_BOOT_DEVICE_OPTICAL,
     UIOX_BOOT_DEVICE_UEFI_SHELL,
 } uiox_boot_device_t;
 
 /* =========================================================================
  * BIOS services context
  * ====================================================================== */
 
 typedef struct {
     uiox_bios_nvram_t  *nvram;
     uiox_post_phase_t   post_phase;
     uiox_e820_map_t     memmap;
     uiox_acpi_info_t    acpi;
     uiox_boot_device_t  boot_device;
     uint32_t            post_code;    /**< Hex POST code (0x00..0xFF)     */
     char                post_msg[64]; /**< Human-readable POST message    */
     uint32_t            total_ram_mb;
     bool                post_ok;
 } uiox_bios_svc_t;
 
 /* =========================================================================
  * Services API
  * ====================================================================== */
 
 int  uiox_bios_svc_init        (uiox_bios_svc_t *svc,
                                   uiox_bios_nvram_t *nvram);
 
 /** Execute POST pipeline (all phases). */
 int  uiox_bios_svc_post        (uiox_bios_svc_t *svc);
 
 /** Build E820 memory map from RAM descriptor + reserved regions. */
 int  uiox_bios_svc_build_memmap(uiox_bios_svc_t *svc,
                                   uint64_t ram_base, uint64_t ram_bytes);
 
 /** Locate ACPI tables in ROM/RAM. */
 int  uiox_bios_svc_find_acpi   (uiox_bios_svc_t *svc,
                                   uint64_t search_base,
                                   uint64_t search_size);
 
 /** Read PCI configuration space (32-bit aligned). */
 uint32_t uiox_bios_svc_pci_read(uiox_bios_svc_t *svc,
                                   uint8_t bus, uint8_t dev,
                                   uint8_t fn, uint8_t reg);
 
 /** Write PCI configuration space. */
 void uiox_bios_svc_pci_write   (uiox_bios_svc_t *svc,
                                   uint8_t bus, uint8_t dev,
                                   uint8_t fn, uint8_t reg, uint32_t val);
 
 /** Select boot device from NVRAM BootOrder. */
 int  uiox_bios_svc_select_boot (uiox_bios_svc_t *svc);
 
 void uiox_bios_svc_print_post  (const uiox_bios_svc_t *svc);
 void uiox_bios_svc_print_memmap(const uiox_bios_svc_t *svc);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_SVC_H */
 