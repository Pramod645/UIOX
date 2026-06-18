Application / Device Access API        (uiox_bms_device)
  → BMS Subsystem: SoC, SoH, balancing, protection       (uiox_bms_subsys)
    → BMS Algorithms: coulomb counter, Kalman, OCV-SoC    (uiox_bms_algo)
    → Cell Balancing: passive/active, equalisation         (uiox_bms_bal)
    → Interface driver: AFE register map, IRQ, I2C/SPI    (uiox_bms_if)
      → Hardware Abstraction: AFE, GPIO, ADC, IRQ          (uiox_bms_hw)
    ↔ Buffer Manager: measurement log, telemetry pool      (uiox_bms_buf)
===========================================================================
uiox-bms/
├── include/
│   ├── uiox_bms_hw.h          # Layer 1  — HAL: AFE, GPIO, ADC, IRQ
│   ├── uiox_bms_buf.h         # Measurement log + telemetry pool
│   ├── uiox_bms_if.h          # Layer 2  — Interface: AFE reg map, IRQ
│   ├── uiox_bms_bal.h         # Cell balancing: passive/active
│   ├── uiox_bms_algo.h        # Layer 3  — Algorithms: SoC, SoH, OCV
│   ├── uiox_bms_subsys.h      # Layer 4  — Subsystem: protection, events
│   └── uiox_bms_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_bms_hw.c
    ├── uiox_bms_buf.c
    ├── uiox_bms_if.c
    ├── uiox_bms_bal.c
    ├── uiox_bms_algo.c
    ├── uiox_bms_subsys.c
    ├── uiox_bms_device.c
    └── uiox_bms_demo.c
=======================================================
uiox-bms/
├── include/
│   ├── uiox_bms_hw.h      # Layer 1   — HAL: I2C/SPI/HDQ/SMBus AFE access,
│   │                      #              cell voltage trigger, pack current
│   │                      #              (shunt), NTC temperature, coulomb
│   │                      #              counter, CHG/DSG FET GPIO, balance
│   │                      #              FET GPIO, alert IRQ, ops vtable
│   ├── uiox_bms_buf.h     # Layer 1.5 — Event log (64-entry circular FIFO),
│   │                      #              telemetry pool (8 snapshots),
│   │                      #              19 event types (OVP/UVP/OCP/SCP/
│   │                      #              OTP/SOC_LOW/FULL/EMPTY/BAL etc.)
│   ├── uiox_bms_if.h      # Layer 2   — Interface: BQ76940-compatible reg map,
│   │                      #              ADC gain/offset calibration read,
│   │                      #              full measurement cycle, IRQ→event
│   │                      #              dispatch, telemetry snapshot, stats
│   ├── uiox_bms_bal.h     # Layer 2b  — Cell balancing: passive resistive,
│   │                      #              delta-mV threshold trigger, max 4
│   │                      #              cells simultaneous, balance mask
│   │                      #              update per tick, bal_time tracking
│   ├── uiox_bms_algo.h    # Layer 3   — Algorithms: coulomb integration
│   │                      #              (ΔQ=I×Δt/3600), OCV-SoC linear
│   │                      #              interpolation (21-point NMC table),
│   │                      #              CC+OCV blended SoC, SoH from full
│   │                      #              capacity, TTE/TTF estimation,
│   │                      #              full-charge detection (V+taper)
│   ├── uiox_bms_subsys.h  # Layer 4   — Subsystem: periodic measurement tick,
│   │                      #              charge/discharge state detection,
│   │                      #              protection FET override on fault,
│   │                      #              balancing during charge, SoC low/
│   │                      #              critical alerts, event dispatch
│   └── uiox_bms_device.h  # Layer 5   — Application API: open/start/stop/
│                          #              close/tick/soc/soh/current/pack_mv/
│                          #              tte_min/ttf_min/remain_mah/charging/
│                          #              present/set_chg_fet/set_dsg_fet/
│                          #              get_telemetry/print_info/print_stats
└── src/
    ├── uiox_bms_hw.c      # HAL lifecycle: init/deinit, measure_cells/
    │                      #   current/temp, set_chg_fet/dsg_fet, set_balance,
    │                      #   fault_status/clear, pack_present
    ├── uiox_bms_buf.c     # Circular event log push/pop/count;
    │                      #   telemetry pool alloc/free with zero on init
    ├── uiox_bms_if.c      # Config + start (ADC gain cal + FET enable),
    │                      #   full measurement cycle, IRQ→fault bitmask→
    │                      #   event push, telemetry fill, stats
    ├── uiox_bms_bal.c     # Min/max cell find, delta threshold check,
    │                      #   balance mask build (cells > vmin+delta/2),
    │                      #   HW mask write, bal_time accumulation
    ├── uiox_bms_algo.c    # Coulomb counter integration (mAh), OCV-SoC
    │                      #   21-point linear interpolation, 70/30 CC/OCV
    │                      #   blend at low current, SoH from full cap,
    │                      #   TTE/TTF from current, full-charge C/20 taper
    ├── uiox_bms_subsys.c  # init chain, start (FETs + ADC + CC enable),
    │                      #   tick: IRQ handle → measure → SoC update →
    │                      #   state detect → full/low/critical fire →
    │                      #   balance tick → event dispatch
    ├── uiox_bms_device.c  # All API wrappers, print_info/print_stats
    │                      #   (per-cell, SoC, SoH, TTE, balance, faults),
    │                      #   print_events, state/event name helpers
    └── uiox_bms_demo.c    # BQ76940 stub HAL (4S NMC 3.5 Ah), discharge
                           #   simulation with voltage droop, charge switch,
                           #   OCV→SoC lookup, OVP fault inject, telemetry,
                           #   balancing, statistics, event log
===========================================================================
Key Design Decisions:
Decision	Rationale
Coulomb counter + OCV blend (70/30)	Coulomb counting drifts over time; OCV lookup corrects it at low current (< 100 mA rest); blend prevents sudden SoC jumps while maintaining long-term accuracy
21-point OCV-SoC NMC table	Covers 2800–4200 mV with linear interpolation between points; swap table for LFP (flat OCV curve) or NCA without changing API
Full-charge detection via V + taper	Both conditions required: pack at vfull_mv AND current below C/20; prevents false-full detection during high-rate charge
Passive balancing during charge only	Balancing during discharge wastes energy; during charge the balancer shunts excess current from high cells — thermally safer
Max 4 cells simultaneous balancing	Limits thermal dissipation in the balance resistors; cells above vmin + delta/2 sorted, highest selected first
Protection FET disable on OVP/SCP	OVP and SCP are immediate safety hazards — FETs cut in the irq_handle() path without waiting for the application tick
ADC gain/offset calibration on start	BQ76940 has per-chip trim values in registers 0x50/0x51/0x59; reading at startup gives correct µV/LSB for that specific IC
64-entry FIFO event log	Captures bursts of events (e.g. OCP + SCP + OTP in quick succession); ring-buffer overwrites oldest — never blocks ISR path
Telemetry pool (8 snapshots)	Pre-allocated snapshots for zero-allocation telemetry in ISR/tick context; pool returned after application processes them
Portable Makefile (no GNU ld flags)	LDFLAGS:= empty — works with GNU ld, Apple ld (macOS), and LLVM lld; -lm added for sqrtf/log10f in algo layer
Vtable ops pattern	BQ76940, BQ40Z50, DS2782, ISL94202, MC33772 all plug in without modifying upper layers
SoH from full-charge capacity	SoH = measured_full_mah / design_mah × 100%; updated at every full-charge event detected; reflects real battery ageing