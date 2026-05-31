Application / Device Access API      (uiox_usb_device)
  → USB Subsystem: enumeration, class drivers, power  (uiox_usb_subsys)
    → USB Protocol: descriptors, control, requests     (uiox_usb_proto)
    → Class drivers: HID, CDC, MSC, Audio, Vendor      (uiox_usb_class)
    → Interface driver: EP management, URB, transfer   (uiox_usb_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, PHY      (uiox_usb_hw)
    ↔ Buffer Manager: URB/transfer buffer pool          (uiox_usb_buf)
/////////////////////////////
uiox-usb/
├── include/
│   ├── uiox_usb_hw.h          # Layer 1  — HAL: MMIO, DMA, IRQ, PHY
│   ├── uiox_usb_buf.h         # URB/transfer buffer pool
│   ├── uiox_usb_if.h          # Layer 2  — Interface driver (EP, URB)
│   ├── uiox_usb_class.h       # Class drivers: HID, CDC, MSC, Vendor
│   ├── uiox_usb_proto.h       # Layer 3  — Protocol: descriptors, requests
│   ├── uiox_usb_subsys.h      # Layer 4  — Subsystem: enum, power, events
│   └── uiox_usb_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_usb_hw.c
    ├── uiox_usb_buf.c
    ├── uiox_usb_if.c
    ├── uiox_usb_class.c
    ├── uiox_usb_proto.c
    ├── uiox_usb_subsys.c
    ├── uiox_usb_device.c
    └── uiox_usb_demo.c
//////////////////////////////////////////////
uiox-usb/
├── include/
│   ├── uiox_usb_hw.h       # Layer 1   — HAL: MMIO regs, USB PHY,
│   │                       #              DMA TX/RX rings, EP table,
│   │                       #              VBUS/OTG, IRQ top-half vtable
│   │                       #              FS/HS/SS/SS+ speed support
│   ├── uiox_usb_buf.h      # Layer 1.5 — URB pool: setup packet struct,
│   │                       #              transfer types, URB status,
│   │                       #              completion callback
│   ├── uiox_usb_if.h       # Layer 2   — Interface driver: EP queue
│   │                       #              per endpoint, URB submission,
│   │                       #              completion dispatch, EP0 state
│   │                       #              machine, IF statistics
│   ├── uiox_usb_class.h    # Layer 2b  — Class driver vtable + HID
│   │                       #              (keyboard boot/report protocol),
│   │                       #              CDC (virtual COM, line coding,
│   │                       #              DTR/RTS), MSC (BOT/SCSI CBW/CSW)
│   ├── uiox_usb_proto.h    # Layer 3   — Protocol: standard descriptors
│   │                       #              (Device/Config/Interface/EP/
│   │                       #              String), standard requests,
│   │                       #              device state machine, string table
│   ├── uiox_usb_subsys.h   # Layer 4   — Subsystem: enumeration pipeline,
│   │                       #              class driver registration/binding,
│   │                       #              SETUP dispatch, SOF, suspend,
│   │                       #              resume, USB event callbacks
│   └── uiox_usb_device.h   # Layer 5   — Application API: open/start/stop/
│                           #              close/tick/process/register_class/
│                           #              inject_setup/inject_ep_complete/
│                           #              inject_reset/connected/configured/
│                           #              print_stats
└── src/
    ├── uiox_usb_hw.c       # HAL lifecycle: init/deinit/start/stop,
    │                       #   ep_config/ep_stall, tx/rx submit, connected
    ├── uiox_usb_buf.c      # URB pool: init/alloc/free/ref, 32-slot pool
    │                       #   with DMA-aligned data buffers
    ├── uiox_usb_if.c       # EP queue per-endpoint, URB submit/complete,
    │                       #   priority drain, EP0 setup RX, stats
    ├── uiox_usb_class.c    # HID: report desc, GET/SET_REPORT, idle,
    │                       #   protocol; CDC: line coding, DTR/RTS,
    │                       #   bulk TX/RX; MSC: CBW parse, CSW build,
    │                       #   SCSI dispatch
    ├── uiox_usb_proto.c    # GET_DESCRIPTOR (Device/Config/String),
    │                       #   SET_ADDRESS, SET_CONFIGURATION,
    │                       #   GET/SET_FEATURE, GET_STATUS, bus reset,
    │                       #   suspend, resume, UTF-16LE string builder
    ├── uiox_usb_subsys.c   # Subsystem init/start/stop, class register,
    │                       #   SETUP dispatch (standard→class→vendor),
    │                       #   SET_CONFIGURATION → class bind, SOF counter,
    │                       #   EP complete dispatch, connect/disconnect poll
    ├── uiox_usb_device.c   # Device open/close, all API wrappers,
    │                       #   inject_* test helpers, print_stats,
    │                       #   state/event/speed name helpers
    └── uiox_usb_demo.c     # CDC + HID composite device demo:
                            #   stub DWC2 HAL, full enumeration sequence,
                            #   HID keyboard reports, CDC TX/RX,
                            #   class-specific requests, suspend/resume
////////////////////
Key Design Decisions
Decision,Rationale
Static URB pool (32 entries),Prevents heap fragmentation; each URB carries 4 KB DMA buffer; no kmalloc equivalent needed on bare-metal
Per-endpoint URB queue,Each EP has an independent 8-deep FIFO; completion of one URB automatically submits the next — zero application involvement in chaining
Generic class driver vtable,"bind/unbind/setup/ep_event/tick interface works for any class (HID, CDC, MSC, Audio, Vendor) without modifying the subsystem"
EP0 state machine,IDLE→SETUP→DATA_IN/OUT→STATUS enforces correct USB control transfer sequencing and prevents race conditions
Standard request handler in proto,Keeps class drivers small — they only see class-specific requests; standard requests are handled once in uiox_usb_proto_setup()
String descriptor UTF-16LE encoding,Built inline without wchar.h; correct for all USB hosts including Windows which requires strict 2-byte-per-character encoding
CDC BOT/SCSI dispatch in MSC,"CBW signature validation + SCSI command callback allows plugging in any storage backend (RAM disk, SD card, NAND flash)"
inject_ test helpers*,Allow full enumeration simulation without real hardware; critical for CI testing of descriptor correctness and class driver logic
Suspend timeout in subsystem,Configurable suspend_timeout_ms drives automatic remote wakeup if device has pending data — standard power management pattern
Composite device support,Multiple class drivers registered in order; SETUP packets dispatched standard→class→vendor with first-match semantics
Speed detection in HAL,uiox_usb_speed_t propagated from PHY chirp negotiation up to application; used by descriptor builder and rate-control
Vtable ops pattern,"DWC2, DWC3, ChipIdea, musb concrete controllers plug in without changing upper layers"
/////////////
