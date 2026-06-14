uiox-chg/
├── include/
│   ├── uiox_chg_hw.h        # Layer 1 — HAL: I²C/GPIO, register map, PD PHY
│   ├── uiox_chg_buf.h       # Event / fault queue pool
│   ├── uiox_chg_if.h        # Layer 2 — Interface: reg access, IRQ, ADC
│   ├── uiox_chg_policy.h    # Layer 3 — Policy: PD negotiation, CV/CC curve
│   ├── uiox_chg_subsys.h    # Layer 4 — Subsystem: safety, thermal, events
│   └── uiox_chg_device.h    # Layer 5 — Application-facing API
└── src/
    ├── uiox_chg_hw.c
    ├── uiox_chg_buf.c
    ├── uiox_chg_if.c
    ├── uiox_chg_policy.c
    ├── uiox_chg_subsys.c
    ├── uiox_chg_device.c
    └── uiox_chg_demo.c
==============================================================================
File	Layer	Mirrors
uiox_chg_hw.h/.c	Hardware — I²C, GPIO, register map, PD PHY, 18-op vtable	uiox_rtc_hw
uiox_chg_buf.h/.c	Buffer pool — event + fault records	uiox_rtc_buf
uiox_chg_if.h/.c	Interface driver — reg access, ADC, IRQ dispatch, poll	uiox_rtc_if
uiox_chg_policy.h/.c	Policy — USB-C PD state machine, CV/CC profile, barrel-jack	uiox_rtc_clock + uiox_tb4_proto
uiox_chg_subsys.h/.c	Subsystem — safety, thermal throttle, fault events	uiox_rtc_subsys
uiox_chg_device.h/.c	Application API — open/start/stop/tick/stats	uiox_rtc_device
uiox_chg_demo.c	Demo — stub BQ25895 HAL, full 10-scenario stack exercise	uiox_rtc_demo
=========================================================
Key Design Decisions:
Decision	Rationale
Two-phase PD negotiation (GET_CAPS → REQUEST → ACCEPT)	Mirrors Intel TB4's two-phase ICM approve; keeps policy state machine testable without real USB-C PHY
reg_rmw in vtable	BQ25895 requires atomic read-modify-write on several registers; elevating it to the vtable avoids races on shared I²C buses
Thermal throttle in subsys tick	Die-temp ADC polled every tick; halves fast_charge_ma above 80 °C, restores at 70 °C — hysteresis prevents chatter
Barrel-jack as fixed-PDO fast path	No PD negotiation overhead; policy_on_plug(BARREL) directly programs iin_lim / ichg / vchg from profile
Fault pool separate from event pool	Faults carry extra fields (tdie_mc, raw vbus_mv) for post-mortem logging without inflating the hot-path event record
wdog_reset called every policy tick	Matches BQ25895 requirement: host must kick watchdog within 40/80/160 s or IC disables charging