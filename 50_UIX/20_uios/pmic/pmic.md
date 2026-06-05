Application / Device Access API        (uiox_pmic_device)
  → PMIC Subsystem: rails, sequencing, thermal, events   (uiox_pmic_subsys)
    → Power Policy: DVFS, sleep, wake, load balance       (uiox_pmic_policy)
    → Rail Manager: buck/LDO config, enable, protection   (uiox_pmic_rail)
    → Interface driver: I2C/SPI register map, IRQ         (uiox_pmic_if)
      → Hardware Abstraction: MMIO, I2C, SPI, GPIO, IRQ   (uiox_pmic_hw)
    ↔ Buffer Manager: event log, telemetry pool            (uiox_pmic_buf)
====================================================================================
uiox-pmic/
├── include/
│   ├── uiox_pmic_hw.h          # Layer 1  — HAL: I2C/SPI, GPIO, IRQ
│   ├── uiox_pmic_buf.h         # Event log + telemetry pool
│   ├── uiox_pmic_if.h          # Layer 2  — Interface: reg map, IRQ
│   ├── uiox_pmic_rail.h        # Rail manager: buck, LDO, switch
│   ├── uiox_pmic_policy.h      # Layer 3  — Policy: DVFS, sleep, wake
│   ├── uiox_pmic_subsys.h      # Layer 4  — Subsystem: seq, thermal
│   └── uiox_pmic_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_pmic_hw.c
    ├── uiox_pmic_buf.c
    ├── uiox_pmic_if.c
    ├── uiox_pmic_rail.c
    ├── uiox_pmic_policy.c
    ├── uiox_pmic_subsys.c
    ├── uiox_pmic_device.c
    └── uiox_pmic_demo.c
=========================================================================================
