Architecture Overview:
Application / Device Access API        (uiox_tb4_device)
  → TB4 Subsystem: topology, security, power, hotplug     (uiox_tb4_subsys)
    → TB4 Protocol: XDomain, DMA, PCIe tunnel, DP tunnel  (uiox_tb4_proto)
    → Router abstraction: path config, adapter mgmt        (uiox_tb4_router)
    → Interface driver: NHI MMIO, ring DMA, IRQ            (uiox_tb4_if)
      → Hardware Abstraction: MMIO, PCIe, GPIO, IRQ        (uiox_tb4_hw)
    ↔ Buffer Manager: TX/RX ring buffer pool               (uiox_tb4_buf)
==========================================================================
uiox-tb4/
├── include/
│   ├── uiox_tb4_hw.h          # Layer 1  — HAL: NHI MMIO, PCIe, GPIO
│   ├── uiox_tb4_buf.h         # TX/RX ring descriptor pool
│   ├── uiox_tb4_if.h          # Layer 2  — Interface: NHI DMA, IRQ
│   ├── uiox_tb4_router.h      # Router: path config, adapter, topology
│   ├── uiox_tb4_proto.h       # Layer 3  — Protocol: XDomain, tunnels
│   ├── uiox_tb4_subsys.h      # Layer 4  — Subsystem: hotplug, security
│   └── uiox_tb4_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_tb4_hw.c
    ├── uiox_tb4_buf.c
    ├── uiox_tb4_if.c
    ├── uiox_tb4_router.c
    ├── uiox_tb4_proto.c
    ├── uiox_tb4_subsys.c
    ├── uiox_tb4_device.c
    └── uiox_tb4_demo.c
===========================================================================
Key Design Decisions:
Decision	Rationale
ICM (Internal Connection Manager) abstraction	The ICM firmware running inside the TB4 controller handles low-level topology discovery; the driver communicates via mailbox messages — this cleanly separates controller firmware from host driver
Router route string addressing	Each TB4 router is uniquely identified by an 8-byte route string (route_hi:route_lo); path setup between any two endpoints uses this without enumeration overhead
Two-phase approve (ICM + tunnel)	Device connection triggers ICM approval; only after authorisation are PCIe/DP/USB tunnels activated — matches Intel TB4 security model
Auto-approve for SEC_NONE	When security is NONE (lab/debug), devices are automatically approved on hotplug — avoids user interaction requirement for development
DMA ring descriptor pool (32 TX + 64 RX)	Pre-allocated DMA-aligned frames; zero heap allocation in hot path; deeper RX pool absorbs burst of incoming tunnelled frames
NHI interrupt mask register pattern	Uses NHI_INTERRUPT_MASK_SET/CLR (separate set/clear) rather than read-modify-write — prevents race condition when masking IRQs from ISR context
ICM mailbox as icm_cmd abstraction	Single function handles request + response polling; upper layers never deal with mailbox producer/consumer pointer management
Separate topology (topo) + protocol (proto) layers	Topology layer handles physical router discovery and config space; protocol layer handles ICM commands and tunnel negotiation — clean separation for testing
Portable Makefile	LDFLAGS:= empty; no --gc-sections/--as-needed — works with GNU ld (Linux), Apple ld (macOS), LLVM lld
Vtable ops pattern	Intel Maple Ridge NHI, Goshen Ridge NHI, USB4 v2 controllers all plug into the same 18-op vtable without changing upper layers