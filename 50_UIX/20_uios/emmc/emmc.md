uiox-emmc/
├── include/
│   ├── uiox_emmc_hw.h       # Layer 1 — HAL: SDIO MMIO, CMD/DAT, eMMC regs
│   ├── uiox_emmc_buf.h      # Block / command / event queue pool
│   ├── uiox_emmc_if.h       # Layer 2 — Interface: CMD framing, CRC, bus width
│   ├── uiox_emmc_proto.h    # Layer 3 — Protocol: eMMC init, EXT_CSD, R/W, HS400
│   ├── uiox_emmc_subsys.h   # Layer 4 — Subsystem: health, cache, partitions
│   └── uiox_emmc_device.h   # Layer 5 — Application-facing API
└── src/
    ├── uiox_emmc_hw.c
    ├── uiox_emmc_buf.c
    ├── uiox_emmc_if.c
    ├── uiox_emmc_proto.c
    ├── uiox_emmc_subsys.c
    ├── uiox_emmc_device.c
    └── uiox_emmc_demo.c
============================================================================================
File	Layer	Mirrors
uiox_emmc_hw.h/.c	Hardware — SDIO MMIO, MMC/eMMC commands, GPIO RST_N/PWR, 19-op vtable	uiox_sata_hw
uiox_emmc_buf.h/.c	Buffer pool — 4 KB block buffers, command records, event records	uiox_sata_buf
uiox_emmc_if.h/.c	Interface driver — CMD/DAT framing, SWITCH, bus config, IRQ dispatch	uiox_sata_if
uiox_emmc_proto.h/.c	Protocol — CMD0→CMD1→CID→RCA→CSD→SELECT→EXT_CSD→BUS_WIDTH→HS400→cache	uiox_sata_proto
uiox_emmc_subsys.h/.c	Subsystem — health monitor (30 s), flush-on-stop, PON, partition events	uiox_sata_subsys
uiox_emmc_device.h/.c	Application API — open/start/stop/tick/read/write/flush/trim/bkops/health	uiox_sata_device
uiox_emmc_demo.c	Demo — stub eMMC 5.1 HAL, 14-scenario full stack exercise	uiox_sata_demo
==========================================================================================
Key Design Decisions:
Decision	Rationale
Two-step HS400 negotiation (HS200 → tuning → HS400)	eMMC 5.1 spec §6.6.2 mandates HS200 tuning before switching to HS400; skipping tuning causes signal integrity failures at 200 MHz DDR
CMD6 SWITCH for all EXT_CSD writes	All EXT_CSD fields are written via CMD6 (SWITCH) with access=WRITE_BYTE; this single function uiox_emmc_if_switch handles bus width, speed, cache, TRIM, PON, BKOPS — one code path for all register changes
Partition routing in read/write_blocks	The stub routes reads/writes to s_flash, s_boot1, or s_boot2 based on hw->active_part; partition select via EXT_CSD[179] PARTITION_CONFIG is transparent to upper layers
Health poll every 30 s in subsys tick	Pre-EOL and life-estimate fields only change slowly; re-reading EXT_CSD every 30 s keeps overhead near zero while still catching degradation before data loss
Flush + PON on subsys stop	subsys_stop calls proto_flush (EXT_CSD FLUSH_CACHE) then proto_pon (POWER_OFF_NOTIFICATION = POWERED_OFF_LONG) before cutting power — matches JEDEC JESD84-B51 §6.6.22 safe-shutdown sequence
EXT_CSD_CACHE_SIZE as feature gate	Cache is enabled only when cache_size_kb > 0 (from EXT_CSD bytes 249–252); chips without internal SRAM cache report 0, so no SWITCH command is sent to non-capable devices
