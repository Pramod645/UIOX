Application / Device Access API    (uiox_can_device)
  → CAN Subsystem: protocol, filters, error handling  (uiox_can_subsys)
    → CAN Protocol: framing, ACK, bus-off recovery    (uiox_can_proto)
    → Sensor/Node abstraction: mailbox, node config   (uiox_can_node)
    → Interface driver: TX/RX FIFO, acceptance filter  (uiox_can_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks   (uiox_can_hw)
    ↔ Buffer Manager: TX/RX message buffers, zero-copy (uiox_can_buf)
///////////////////
uiox-can/
├── include/
│   ├── uiox_can_hw.h       # Layer 1   — HAL: MMIO regs, bit-timing,
│   │                       #              DMA rings, IRQ, SPI ops vtable
│   │                       #              CAN-FD + Classic CAN 2.0A/2.0B
│   ├── uiox_can_buf.h      # Layer 1.5 — TX + RX message buffer pools
│   │                       #              DLC↔length helpers, ref counting
│   ├── uiox_can_if.h       # Layer 2   — Interface driver: TX/RX FIFO,
│   │                       #              acceptance filters, stats,
│   │                       #              bit-timing computation
│   ├── uiox_can_node.h     # Layer 2b  — Node abstraction: mailboxes,
│   │                       #              NMT state machine, heartbeat,
│   │                       #              periodic TX scheduling
│   ├── uiox_can_proto.h    # Layer 3   — Protocol: CANopen SDO/PDO,
│   │                       #              NMT master, EMCY, SYNC,
│   │                       #              bus-off recovery state machine
│   ├── uiox_can_subsys.h   # Layer 4   — Subsystem: multi-bus management,
│   │                       #              global RX dispatch, gateway,
│   │                       #              bus health monitoring
│   └── uiox_can_device.h   # Layer 5   — Application API: open/add_bus/
│                           #              start/stop/tx/rx/sdo/nmt/emcy/
│                           #              gateway/tick/process/health/stats
└── src/
    ├── uiox_can_hw.c       # HAL lifecycle: init/start/stop/deinit,
    │                       #   set_mode/set_filter/get_err_cnt/recover
    ├── uiox_can_buf.c      # Static TX+RX pool alloc/free/ref,
    │                       #   DLC↔length helpers
    ├── uiox_can_if.c       # IF config, bit-timing calc, filter mgmt,
    │                       #   TX/RX SW queue, stats
    ├── uiox_can_node.c     # Node init, mailbox add/dispatch,
    │                       #   NMT tx, heartbeat tick,
    │                       #   periodic mailbox scheduling
    ├── uiox_can_proto.c    # NMT cmd, SDO expedited rd/wr,
    │                       #   EMCY, SYNC, PDO tx,
    │                       #   RX dispatcher, bus-off tick
    ├── uiox_can_subsys.c   # Multi-bus add/start/stop, global RX
    │                       #   handler dispatch, gateway routing,
    │                       #   health update, stats snapshot
    ├── uiox_can_device.c   # Device open/close, all API wrappers,
    │                       #   health_name string helper
    └── uiox_can_demo.c     # End-to-end demo: 2-bus CAN-FD + Classic,
                            #   NMT/SDO/PDO/EMCY/gateway/health/stats
//////////////
Decision,Rationale
Dual static TX/RX buffer pools,Prevents heap fragmentation; safe for RTOS/bare-metal; TX and RX pools sized independently
CAN-FD + Classic CAN unified frame,Single uiox_can_msg_t with 64-byte data field works for both; DLC helpers translate correctly
Bit-timing auto-computation,bitrate_to_timing() finds BRP/TSEG1/TSEG2 at 80% sample point automatically; no manual register fiddling
Software TX/RX queues,Hardware FIFO overflow → software queue absorbs bursts; prevents frame loss under load
Filter programming abstraction,uiox_can_if_add_filter() programs both SW filter table and HW acceptance filter atomically
CANopen NMT state machine,INIT → PRE-OP → OPERATIONAL → STOPPED transitions enforce correct bus behaviour
SDO expedited read/write,Covers >90% of real-world SDO use cases without segmented transfer complexity
Bus-off recovery automaton,Configurable retry delay + max retries; fires recover() HAL call periodically until error-active
Multi-bus gateway mode,uiox_can_subsys_process() ref-counts and forwards messages across buses without extra copy
Global RX handler registry,ID-filtered callbacks decouple protocol layers from application; up to 8 handlers per device
Vtable ops pattern,"M_CAN, FlexCAN, SJA1000 concrete drivers plug in without modifying upper layers"
CANopen heartbeat via NMT tick,uiox_can_node_tick() drives heartbeat + periodic mailboxes from a single 1 ms periodic call