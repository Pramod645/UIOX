uiox-rtc/
├── include/
│   ├── uiox_rtc_hw.h        # Layer 1 — HAL: port I/O, register map, GPIO
│   ├── uiox_rtc_buf.h       # Alarm/event queue pool
│   ├── uiox_rtc_if.h        # Layer 2 — Interface: register access, IRQ
│   ├── uiox_rtc_clock.h     # Layer 3 — Clock: time, alarm, epoch math
│   ├── uiox_rtc_subsys.h    # Layer 4 — Subsystem: power, events, battery
│   └── uiox_rtc_device.h    # Layer 5 — Application-facing API
└── src/
    ├── uiox_rtc_hw.c
    ├── uiox_rtc_buf.c
    ├── uiox_rtc_if.c
    ├── uiox_rtc_clock.c
    ├── uiox_rtc_subsys.c
    ├── uiox_rtc_device.c
    └── uiox_rtc_demo.c
===========================================================================
File	Layer	Mirrors TB4
uiox_rtc_hw.h/.c	Hardware — port I/O, register map, vtable	uiox_tb4_hw
uiox_rtc_buf.h/.c	Buffer pool — event + alarm queues	uiox_tb4_buf
uiox_rtc_if.h/.c	Interface driver — UIP guard, IRQ, reg access	uiox_tb4_if
uiox_rtc_clock.h/.c	Clock / Protocol — BDT time, alarm, epoch math	uiox_tb4_proto + uiox_tb4_router
uiox_rtc_subsys.h/.c	Subsystem — battery monitor, event dispatch	uiox_tb4_subsys
uiox_rtc_device.h/.c	Application API — open/start/stop/tick/stats	uiox_tb4_device
uiox_rtc_demo.c	Demo — stub HAL, full stack exercise	uiox_tb4_demo