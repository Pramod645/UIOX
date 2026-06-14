uiox-sd/
├── include/
│   ├── uiox_sd_hw.h         # Layer 1 — HAL: SDIO/SPI, register map, GPIO
│   ├── uiox_sd_buf.h        # Block / command queue pool
│   ├── uiox_sd_if.h         # Layer 2 — Interface: CMD/DAT, CRC, bus width
│   ├── uiox_sd_proto.h      # Layer 3 — Protocol: SD init, CSD/CID, R/W
│   ├── uiox_sd_subsys.h     # Layer 4 — Subsystem: hotplug, WP, events
│   └── uiox_sd_device.h     # Layer 5 — Application-facing API
└── src/
    ├── uiox_sd_hw.c
    ├── uiox_sd_buf.c
    ├── uiox_sd_if.c
    ├── uiox_sd_proto.c
    ├── uiox_sd_subsys.c
    ├── uiox_sd_device.c
    └── uiox_sd_demo.c
======================================================================
File	Layer	Mirrors
uiox_sd_hw.h/.c	Hardware — SDIO/SPI MMIO, SD commands, GPIO CD/WP, CRC, 18-op vtable	uiox_chg_hw
uiox_sd_buf.h/.c	Buffer pool — 512 B block records, command records, event records	uiox_chg_buf
uiox_sd_if.h/.c	Interface driver — CMD/DAT framing, bus width, IRQ dispatch	uiox_chg_if
uiox_sd_proto.h/.c	Protocol — CMD0→ACMD41→CID→RCA→CSD→SELECT init state machine, R/W, erase	uiox_chg_policy + uiox_tb4_proto
uiox_sd_subsys.h/.c	Subsystem — hotplug, write-protect polling, event dispatch	uiox_chg_subsys
uiox_sd_device.h/.c	Application API — open/start/stop/tick/read/write/erase/stats	uiox_chg_device
uiox_sd_demo.c	Demo — stub SD host controller HAL, 12-scenario full stack exercise	uiox_chg_demo
//////////////////////////////////////////////////////////////////////////
Key Design Decisions
Decision	Rationale
SD init state machine in proto	CMD0→ACMD41→CID→RCA→CSD→SELECT is a strict ordered sequence; making each step a named state (uiox_sd_init_state_t) makes failures unambiguous to diagnose
LBA vs byte address branch in proto	SDSC uses byte addresses; SDHC/SDXC use block addresses — the branch is isolated in proto_read/write, keeping if clean of address arithmetic
Separate CRC7 / CRC16 in vtable	Platform SDIO controllers often provide hardware CRC acceleration; the vtable lets you plug in HW CRC without touching upper layers
Three-pool buffer design	Block pool (512 B DMA-aligned), command pool (request/response records), event pool (hotplug/done events) each sized independently — block pool is the most constrained resource
Write-protect checked at three layers	hw_write_blocks (HAL), proto_write (protocol), subsys_write (subsystem) — defence in depth for a destructive operation
WP polled every subsys tick	The WP pin is a mechanical switch that changes slowly; polling the PRESENT_STATE register each tick (10 ms) is cheaper than a dedicated GPIO IRQ and matches SD Host Spec recommendation
SDIO_REG_INT_STATUS W1C pattern	Interrupt status bits are Write-1-to-Clear — the stub and IF layer both honour this to match real SD Host Controller Spec v3 behaviour