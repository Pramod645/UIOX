Application / Device Access API        (uiox_bt_device)
  → BT Subsystem: pairing, profiles, power, events        (uiox_bt_subsys)
    → BT Protocol: HCI, L2CAP, SDP, GATT, A2DP           (uiox_bt_proto)
    → BT Manager: device scan, pair, connect, disconnect   (uiox_bt_mgr)
    → Interface driver: HCI UART/USB/SDIO transport        (uiox_bt_if)
      → Hardware Abstraction: UART, USB, GPIO, IRQ         (uiox_bt_hw)
    ↔ Buffer Manager: HCI command/event/data pool         (uiox_bt_buf)
========================================================================
uiox-bt/
├── include/
│   ├── uiox_bt_hw.h          # Layer 1  — HAL: UART/USB/GPIO/IRQ
│   ├── uiox_bt_buf.h         # HCI packet buffer pool
│   ├── uiox_bt_if.h          # Layer 2  — Interface: HCI transport
│   ├── uiox_bt_mgr.h         # Device manager: scan, pair, connect
│   ├── uiox_bt_proto.h       # Layer 3  — Protocol: HCI, L2CAP, GATT
│   ├── uiox_bt_subsys.h      # Layer 4  — Subsystem: profiles, events
│   └── uiox_bt_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_bt_hw.c
    ├── uiox_bt_buf.c
    ├── uiox_bt_if.c
    ├── uiox_bt_mgr.c
    ├── uiox_bt_proto.c
    ├── uiox_bt_subsys.c
    ├── uiox_bt_device.c
    └── uiox_bt_demo.c
=====================================================================
uiox-bt/
├── include/
│   ├── uiox_bt_hw.h       # Layer 1   — HAL: UART/USB/SDIO transport,
│   │                      #              BT_EN/BT_WAKE/HOST_WAKE GPIO,
│   │                      #              HCI write/read, FW download,
│   │                      #              BT version/caps, ops vtable,
│   │                      #              all HCI opcodes + event codes
│   ├── uiox_bt_buf.h      # Layer 1.5 — HCI command pool (16 slots),
│   │                      #              ACL data pool (32 slots),
│   │                      #              async event ring buffer (64),
│   │                      #              push/pop/empty for event ring
│   ├── uiox_bt_if.h       # Layer 2   — Interface: HCI cmd send + wait
│   │                      #              (CMD_COMPLETE with timeout),
│   │                      #              ACL TX with L2CAP framing,
│   │                      #              RX poll → event ring push,
│   │                      #              statistics (cmds/events/bytes)
│   ├── uiox_bt_mgr.h      # Layer 2b  — Device manager: 16-device table,
│   │                      #              classic inquiry scan, BLE scan,
│   │                      #              BLE advertising, LE create conn,
│   │                      #              HCI event → device table update
│   │                      #              (INQ_RESULT/CONN/DISCONN/LE_META)
│   ├── uiox_bt_proto.h    # Layer 3   — Protocol: L2CAP TX framing,
│   │                      #              GATT discover (ATT_READ_BY_GROUP),
│   │                      #              GATT write (ATT_WRITE_REQ),
│   │                      #              GATT read (ATT_READ_REQ),
│   │                      #              common service/char UUIDs
│   ├── uiox_bt_subsys.h   # Layer 4   — Subsystem: init chain (if→mgr→proto),
│   │                      #              power-on + HCI reset + BD addr,
│   │                      #              periodic RX poll, event ring drain,
│   │                      #              HCI event → app callback dispatch,
│   │                      #              13 event types, state FSM
│   └── uiox_bt_device.h   # Layer 5   — Application API: open/start/stop/
│                          #              close/tick/set_name/scan_start/
│                          #              scan_stop/le_scan/adv/connect/
│                          #              disconnect/gatt_write/acl_send/
│                          #              hci_cmd/find_device/print_*
└── src/
    ├── uiox_bt_hw.c       # HAL lifecycle: init/deinit/power,
    │                      #   hci_write/read, fw_download
    ├── uiox_bt_buf.c      # CMD + ACL pool alloc/free, event ring
    │                      #   push/pop/empty (power-of-2 mask)
    ├── uiox_bt_if.c       # hci_cmd (build pkt→write→wait CMD_COMPLETE),
    │                      #   acl_tx (HCI ACL header), rx_poll (read→ring),
    │                      #   stats
    ├── uiox_bt_mgr.c      # init_ctrl (Reset+BD_addr+SSP), set_name,
    │                      #   set_disc, scan_start/stop (classic INQUIRY),
    │                      #   adv_start/stop, le_scan_start/stop,
    │                      #   connect (LE_CREATE_CONN), disconnect,
    │                      #   process_evt (INQ/CONN/DISCONN/LE_META),
    │                      #   print_devices
    ├── uiox_bt_proto.c    # gatt_discover (ATT_READ_BY_GROUP_TYPE_REQ),
    │                      #   gatt_write (ATT_WRITE_REQ), gatt_read
    │                      #   (ATT_READ_REQ), l2cap_tx (header+payload)
    ├── uiox_bt_subsys.c   # init chain, start (if_start+init_ctrl+fire),
    │                      #   stop (if_stop+fire), tick (poll+drain+dispatch)
    ├── uiox_bt_device.c   # All API wrappers, print_info (BD addr/model/
    │                      #   version/caps/state), print_stats, print_devices,
    │                      #   state/event/version name helpers
    └── uiox_bt_demo.c     # Intel AX211 stub HAL (simulated HCI responses
                           #   per opcode), classic inquiry, BLE ADV, BLE scan
                           #   + simulated ADV report, raw HCI cmd, ACL send,
                           #   GATT discover + write, tick loop, statistics
===========================================================================
Key Design Decisions
Decision	Rationale
HCI CMD_COMPLETE wait loop (10 ms poll)	No blocking OS sleep; 10 ms granularity sufficient for all BT HCI timeouts (1–30 s); replace with semaphore/event on RTOS
64-entry async event ring buffer	Decouples ISR/poll from application dispatch; handles bursts (ADV reports at high scan density); overflow counted not blocking
Direct find_or_add device table	O(n) search over 16 entries — acceptable; direct array avoids dynamic allocation in HCI event path
L2CAP header prepend in proto layer	ATT/GATT data always needs L2CAP framing; centralising in l2cap_tx() prevents every caller from duplicating the 4-byte header
HCI event parsing in mgr (not subsys)	Device table management belongs to the manager; subsys only drives the pipeline and fires callbacks — clean separation
Separate CMD pool (16) and ACL pool (32)	Commands are rare and small; ACL data is frequent and may burst; separate pools prevent command starvation during heavy data transfer
BLE scan simulated ADV inject at step 6	Demo simulates an async ADV_REPORT arriving between poll cycles — validates the ring buffer path without real hardware
Simple Pairing enabled at init	HCI_OP_WRITE_SIMPLE_PAIR_MODE enables Secure Simple Pairing (SSP) on classic BT; required for modern devices; always safe to set
Portable Makefile	LDFLAGS:= empty; no --gc-sections/--as-needed — works with GNU ld, Apple ld (macOS), LLVM lld
Vtable ops pattern	Intel AX211 UART, Qualcomm WCN6855 USB, Broadcom BCM4389 SDIO all plug into the same 12-op vtable without modifying upper layers