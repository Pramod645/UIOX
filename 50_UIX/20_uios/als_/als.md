uiox-als/
├── include/
│   ├── uiox_als_hw.h        # Layer 1 — HAL: I²C, register map, GPIO INT
│   ├── uiox_als_buf.h       # Sample / event queue pool
│   ├── uiox_als_if.h        # Layer 2 — Interface: reg access, IRQ, gain
│   ├── uiox_als_cal.h       # Layer 3 — Calibration: lux formula, CCT
│   ├── uiox_als_subsys.h    # Layer 4 — Subsystem: threshold, auto-gain
│   └── uiox_als_device.h    # Layer 5 — Application-facing API
└── src/
    ├── uiox_als_hw.c
    ├── uiox_als_buf.c
    ├── uiox_als_if.c
    ├── uiox_als_cal.c
    ├── uiox_als_subsys.c
    ├── uiox_als_device.c
    └── uiox_als_demo.c
=====================================================
