Application / Device Access API        (uiox_mouse_device)
  → Mouse Subsystem: cursor, acceleration, gestures     (uiox_mouse_subsys)
    → Event Processing: debounce, click, double-click    (uiox_mouse_event)
    → Protocol: USB HID, PS/2, I2C trackpad              (uiox_mouse_proto)
    → Interface driver: USB/PS2/I2C RX, report parse     (uiox_mouse_if)
      → Hardware Abstraction: MMIO, USB, UART, IRQ       (uiox_mouse_hw)
    ↔ Buffer Manager: mouse event ring buffer            (uiox_mouse_buf)
==========================================================================
uiox-mouse/
├── include/
│   ├── uiox_mouse_hw.h         # Layer 1  — HAL: USB/PS2/I2C/UART, IRQ
│   ├── uiox_mouse_buf.h        # Event ring buffer
│   ├── uiox_mouse_if.h         # Layer 2  — Interface driver (report parse)
│   ├── uiox_mouse_proto.h      # Layer 3  — Protocol: HID, PS/2, trackpad
│   ├── uiox_mouse_event.h      # Event processing: debounce, click, gesture
│   ├── uiox_mouse_subsys.h     # Layer 4  — Subsystem: cursor, accel, zones
│   └── uiox_mouse_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_mouse_hw.c
    ├── uiox_mouse_buf.c
    ├── uiox_mouse_if.c
    ├── uiox_mouse_proto.c
    ├── uiox_mouse_event.c
    ├── uiox_mouse_subsys.c
    ├── uiox_mouse_device.c
    └── uiox_mouse_demo.c
=============================================
uiox-mouse/
├── include/
│   ├── uiox_mouse_hw.h     # Layer 1   — HAL: USB/PS2/I2C/UART/GPIO,
│   │                       #              IRQ, read_report, set_rate/DPI,
│   │                       #              connection detect, ops vtable
│   ├── uiox_mouse_buf.h    # Layer 1.5 — SPSC ring buffer (128-deep),
│   │                       #              mouse event struct (all types),
│   │                       #              button/gesture constants
│   ├── uiox_mouse_if.h     # Layer 2   — Interface: poll, button change
│   │                       #              detection, scroll events,
│   │                       #              connect/disconnect, IF statistics
│   ├── uiox_mouse_proto.h  # Layer 3   — Protocol decode: HID Boot,
│   │                       #              PS/2 standard + IntelliMouse,
│   │                       #              Serial Microsoft, Synaptics/ELAN,
│   │                       #              PS/2 packet reassembly, sync check
│   ├── uiox_mouse_event.h  # Layer 3b  — Event processing: debounce,
│   │                       #              click/double-click detection,
│   │                       #              pointer acceleration, sub-pixel,
│   │                       #              cursor clamping, scroll invert
│   ├── uiox_mouse_subsys.h # Layer 4   — Subsystem: full pipeline tick,
│   │                       #              hot zone registry, zone callbacks
│   │                       #              (enter/leave/click), event
│   │                       #              callbacks, statistics
│   └── uiox_mouse_device.h # Layer 5   — Application API: open/start/stop/
│                           #              close/tick/poll/add_zone/
│                           #              add_callback/cursor/warp/
│                           #              connected/print_stats
└── src/
    ├── uiox_mouse_hw.c     # HAL lifecycle: init/deinit/enable/disable,
    │                       #   read_report, connected — all via ops vtable
    ├── uiox_mouse_buf.c    # SPSC ring: init/push/pop/peek/empty/count/flush
    ├── uiox_mouse_if.c     # Poll → button change detect → scroll events,
    │                       #   connect/disconnect events, stats
    ├── uiox_mouse_proto.c  # HID Boot decode (3/4 byte), PS/2 packet
    │                       #   reassembly + sync validation + sign-extend,
    │                       #   Serial Microsoft 3-byte, Synaptics/ELAN 6-byte
    ├── uiox_mouse_event.c  # Velocity-based accel, sub-pixel accumulation,
    │                       #   debounce per-button, click hold-time check,
    │                       #   double-click gap check, cursor clamp, warp
    ├── uiox_mouse_subsys.c # Tick pipeline: poll→event_process→dispatch,
    │                       #   zone hit-test (enter/leave/click), callback
    │                       #   dispatch, statistics, state management
    ├── uiox_mouse_device.c # All API wrappers, print_stats, evt/state names
    └── uiox_mouse_demo.c   # USB HID stub HAL, 17-step sim sequence,
                            #   3 hot zones, global callback, warp, disconnect,
                            #   reconnect, final stats
===================
Key Design Decisions
Decision	Rationale
SPSC lock-free ring buffer	Single-producer (ISR/poll) / single-consumer (application) — no mutex needed; 128-slot power-of-2 mask avoids modulo
Two-stage ring buffer pipeline	raw_rb holds IF events; cooked_rb holds fully processed events — scan rate and application rate fully decoupled
Protocol decode in separate layer	PS/2, HID, Serial, I2C trackpad all have different byte formats; decoder