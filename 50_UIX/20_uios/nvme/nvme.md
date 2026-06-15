UIOX NVMe SSD — PCIe/NVMe Stack — Full Implementation
Modeled exactly on the uiox-emmc / uiox-sata architecture: 6-layer vtable design, same file layout, same Makefile pattern, same coding style. Targets M.2 2280 NVMe SSDs over PCIe 4.0/5.0 x4 (NVMe 1.4 / 2.0).
==============================================================================================
uiox-nvme/
├── include/
│   ├── uiox_nvme_hw.h       # Layer 1 — HAL: PCIe BAR0 MMIO, NVMe regs
│   ├── uiox_nvme_buf.h      # SQ/CQ descriptor / PRD / event pool
│   ├── uiox_nvme_if.h       # Layer 2 — Interface: SQ/CQ, doorbell, IRQ
│   ├── uiox_nvme_proto.h    # Layer 3 — Protocol: Identify, NVM cmds, SMART
│   ├── uiox_nvme_subsys.h   # Layer 4 — Subsystem: health, APST, NS mgmt
│   └── uiox_nvme_device.h   # Layer 5 — Application-facing API
└── src/
    ├── uiox_nvme_hw.c
    ├── uiox_nvme_buf.c
    ├── uiox_nvme_if.c
    ├── uiox_nvme_proto.c
    ├── uiox_nvme_subsys.c
    ├── uiox_nvme_device.c
    └── uiox_nvme_demo.c
================================================================
File	Layer	Mirrors
uiox_nvme_hw.h/.c	Hardware — BAR0 MMIO, SQE/CQE structs, doorbell, MSI-X, 18-op vtable	uiox_emmc_hw
uiox_nvme_buf.h/.c	Buffer pool — 64 in-flight command records, 4 KB data blocks, events	uiox_emmc_buf
uiox_nvme_if.h/.c	Interface driver — SQ/CQ ring management, doorbell, IRQ dispatch	uiox_emmc_if
uiox_nvme_proto.h/.c	Protocol — Identify, queue creation, Set Features, I/O, SMART, shutdown	uiox_emmc_proto
uiox_nvme_subsys.h/.c	Subsystem — CFS watchdog, SMART health poll (60 s), flush-on-stop	uiox_emmc_subsys
uiox_nvme_device.h/.c	Application API — open/start/stop/tick/read/write/flush/trim/smart/format	uiox_emmc_device
uiox_nvme_demo.c	Demo — stub BAR0 HAL, 14-scenario full stack exercise	uiox_emmc_demo
==============================
Key Design Decisions:
Decision	Rationale
CC.EN → CSTS.RDY polling in ctrl_enable	NVMe spec §3.5.1 mandates waiting up to CAP.TO × 500 ms for CSTS.RDY after setting CC.EN; the stub simulates immediate assertion — real drivers poll with timeout
Phase tag tracking in io_complete	NVMe CQ phase bit toggles on each wrap of the CQ ring; the IF layer tracks io_cq_phase and only consumes entries matching the expected phase — prevents double-processing completions
Separate admin / I/O paths	admin_cmd is blocking (timeout-based, Queue 0); io_submit + io_poll is non-blocking (Queue 1+) with doorbell; this matches NVMe spec queue model and enables future async I/O expansion
CDW10 CNS field for Identify dispatch	stub_admin_cmd switches on sqe->cdw10 & 0xFF (the CNS byte) to serve Identify Controller, Namespace, or NS List — isolating three very different data structures behind one opcode
CFS polling every subsys tick	Controller Fatal Status (CSTS.CFS) can be set asynchronously by the device; checking it every 10 ms tick catches NVM media errors before the next I/O attempt silently corrupts data
Flush all active namespaces before CC.SHN	subsys_stop iterates all active NSIDs and issues Flush before setting CC.SHN (shutdown notification); NVMe spec §3.8 requires the host to issue Flush before shutdown to commit volatile write cache
Volatile WC + APST enabled in ctrl_init	Both are opt-in features: VWC improves write latency, APST enables automatic link power transitions; enabling both during init matches what nvme-cli and Linux nvme driver do by default