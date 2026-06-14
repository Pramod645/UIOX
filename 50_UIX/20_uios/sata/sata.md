uiox-sata/
├── include/
│   ├── uiox_sata_hw.h       # Layer 1 — HAL: AHCI MMIO, FIS, PxCLB/FIS
│   ├── uiox_sata_buf.h      # PRD / command table / event pool
│   ├── uiox_sata_if.h       # Layer 2 — Interface: command issue, IRQ
│   ├── uiox_sata_proto.h    # Layer 3 — Protocol: ATA cmds, IDENTIFY, NCQ
│   ├── uiox_sata_subsys.h   # Layer 4 — Subsystem: hotplug, power, events
│   └── uiox_sata_device.h   # Layer 5 — Application-facing API
└── src/
    ├── uiox_sata_hw.c
    ├── uiox_sata_buf.c
    ├── uiox_sata_if.c
    ├── uiox_sata_proto.c
    ├── uiox_sata_subsys.c
    ├── uiox_sata_device.c
    └── uiox_sata_demo.c
====================================================================
6-layer vtable design, same file layout, same Makefile pattern, same coding style. Targets AHCI (Advanced Host Controller Interface) SATA III for 2.5" HDD/SSD.
============================================================
File	Layer	Mirrors
uiox_sata_hw.h/.c	Hardware — AHCI BAR5 MMIO, FIS structs, per-port regs, 18-op vtable	uiox_sd_hw
uiox_sata_buf.h/.c	Buffer pool — NCQ command slots, 4 KB sector buffers, event records	uiox_sd_buf
uiox_sata_if.h/.c	Interface driver — FIS framing, slot management, IRQ dispatch	uiox_sd_if
uiox_sata_proto.h/.c	Protocol — IDENTIFY, ATA R/W DMA EXT, NCQ, TRIM, SMART, power	uiox_sd_proto
uiox_sata_subsys.h/.c	Subsystem — hotplug, auto flush-on-stop, event dispatch	uiox_sd_subsys
uiox_sata_device.h/.c	Application API — open/start/stop/tick/read/write/flush/trim/smart	uiox_sd_device
uiox_sata_demo.c	Demo — stub AHCI HAL, 14-scenario full stack exercise	uiox_sd_demo
=======================================================================
Key Design Decisions:
Decision	Rationale
AHCI spec-compliant register layout	GHC at BAR5+0x000, per-port at BAR5+0x100+port×0x80 — matches AHCI 1.3.1 §3; real drivers can swap in MMIO-mapped pointers without changing upper layers
FIS structs as packed types	uiox_sata_fis_h2d_t / uiox_sata_fis_d2h_t are __attribute__((packed)) — they can be DMA-mapped directly to the command table in real hardware
NCQ tag bitmap in hw descriptor	hw->ncq_active tracks outstanding tags as a 32-bit bitmask; proto_ncq_read/write allocates the lowest free bit, matching AHCI SActive register semantics
count == 0 → IDENTIFY in stub	The stub read_sectors uses count == 0 as a sentinel for IDENTIFY data, keeping the vtable single-entry for both operations; real drivers would use a separate cmd_issue path
Flush before subsys stop	subsys_stop calls proto_flush (FLUSH CACHE EXT) before disabling the port — prevents data loss on surprise shutdown, matching Linux libata behaviour
SMART warn surfaced via IRQ	The SMART stub sets UIOX_SATA_IRQ_ERROR when a pre-fail attribute is detected, letting the subsystem raise UIOX_SATA_EV_SMART_WARN through the normal IRQ path
ATA string byte-swap in proto	ATA IDENTIFY strings are word-byte-swapped; ata_str_fixup() corrects this in the protocol layer, so upper layers always see ASCII strings
====================================================================