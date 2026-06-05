Application / Device Access API        (uiox_bios_device)
  → BIOS Subsystem: boot, POST, ACPI, setup, variables   (uiox_bios_subsys)
    → BIOS Services: memory map, PCI enum, ACPI tables    (uiox_bios_svc)
    → NVRAM / Variable store: EFI variables, CMOS         (uiox_bios_nvram)
    → Interface driver: SPI flash, MMIO, SMM, UEFI proto  (uiox_bios_if)
      → Hardware Abstraction: SPI ctrl, GPIO, WP, IRQ     (uiox_bios_hw)
    ↔ Buffer Manager: flash page buffer pool              (uiox_bios_buf)
=============================================================================
uiox-bios/
├── include/
│   ├── uiox_bios_hw.h          # Layer 1  — HAL: SPI, GPIO, WP, SMM
│   ├── uiox_bios_buf.h         # Flash page buffer pool
│   ├── uiox_bios_if.h          # Layer 2  — Interface: SPI flash, MMIO
│   ├── uiox_bios_nvram.h       # NVRAM: EFI variables, CMOS, ESCD
│   ├── uiox_bios_svc.h         # Layer 3  — Services: POST, memmap, ACPI
│   ├── uiox_bios_subsys.h      # Layer 4  — Subsystem: boot, setup, events
│   └── uiox_bios_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_bios_hw.c
    ├── uiox_bios_buf.c
    ├── uiox_bios_if.c
    ├── uiox_bios_nvram.c
    ├── uiox_bios_svc.c
    ├── uiox_bios_subsys.c
    ├── uiox_bios_device.c
    └── uiox_bios_demo.c
============================================================================
Key Design Decisions
Decision	Rationale
Read-modify-write in IF layer	NOR flash sectors must be erased before writing; IF layer handles partial-sector writes transparently — application never deals with erase granularity
WP# auto-manage in if_write()	Write-protect GPIO removed before write and restored after — application can't forget to re-enable WP; prevents firmware corruption from interrupted writes
Zero staging buffers on free	4 KB staging buffers may contain partial firmware images or keys; zeroing on free prevents sensitive data leaking through buffer reuse
EFI variable cache in RAM	Variables loaded once from flash into uiox_bios_var_t array; dirty flag tracks changes; nvram_flush() writes back — minimises flash wear
CMOS checksum on every write	Bytes 0x10–0x2D checksum updated atomically in cmos_set() — BIOS POST validates checksum on next boot; corruption detected before it causes problems
POST pipeline (8 phases)	Maps directly to real BIOS POST code sequence (CPU→MEM→CHIPset→PCI→ACPI→NVRAM→OROM→BOOT); POST code written to I/O port 0x80 on real hardware
E820 memory map builder	Produces correct entry types (USABLE/RESERVED/ACPI/FIRMWARE) that OS loaders (GRUB, EDK2 BDS) consume; firmware region at 0xFF000000 always reserved
ACPI table locate stub	Real implementation scans 0xF0000–0xFFFFF + EBDA for "RSD PTR " signature; stub sets known offsets for demo — swap with real scanner in production
4-entry staging pool	Read-modify-write uses one buffer; firmware update uses another; prevents pool exhaustion during concurrent flash operations
Portable Makefile	No --gc-sections/--as-needed — works with GNU ld, Apple ld (macOS), and LLVM lld
Vtable ops pattern	AMI Aptio SPI, coreboot flashrom backend, Intel SPI controller, Raspberry Pi SPI all plug into the same HAL vtable