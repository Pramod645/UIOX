Application / Device Access API        (uiox_fan_device)
  → Fan Subsystem: thermal policy, speed control, events  (uiox_fan_subsys)
    → Thermal Engine: PID, step, hysteresis controllers    (uiox_fan_thermal)
    → Fan Driver abstraction: RPM, PWM, tach, stall        (uiox_fan_drv)
    → Interface driver: I2C/SPI/GPIO/PWM register map      (uiox_fan_if)
      → Hardware Abstraction: MMIO, I2C, PWM, IRQ, GPIO   (uiox_fan_hw)
    ↔ Buffer Manager: telemetry log + event pool           (uiox_fan_buf)
=========================================================================
uiox-fan/
├── include/
│   ├── uiox_fan_hw.h          # Layer 1  — HAL: I2C/SPI/PWM/GPIO, IRQ
│   ├── uiox_fan_buf.h         # Telemetry log + event pool
│   ├── uiox_fan_if.h          # Layer 2  — Interface: reg map, IRQ
│   ├── uiox_fan_drv.h         # Fan driver: RPM/PWM/tach/stall
│   ├── uiox_fan_thermal.h     # Layer 3  — Thermal: PID, step, hysteresis
│   ├── uiox_fan_subsys.h      # Layer 4  — Subsystem: policy, zones
│   └── uiox_fan_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_fan_hw.c
    ├── uiox_fan_buf.c
    ├── uiox_fan_if.c
    ├── uiox_fan_drv.c
    ├── uiox_fan_thermal.c
    ├── uiox_fan_subsys.c
    ├── uiox_fan_device.c
    └── uiox_fan_demo.c
===========================================================================
uiox-fan/
├── include/
│   ├── uiox_fan_hw.h      # Layer 1   — HAL: I2C/SPI/LPC/GPIO bus,
│   │                      #              PWM duty set, tachometer read,
│   │                      #              temperature sensor read,
│   │                      #              fault status/clear, chan enable,
│   │                      #              GPIO, watchdog, ISR, ops vtable
│   ├── uiox_fan_buf.h     # Layer 1.5 — 64-entry circular event log,
│   │                      #              8-slot telemetry pool,
│   │                      #              12 event types (stall, PWM change,
│   │                      #              overheat, spin-up fail, etc.)
│   ├── uiox_fan_if.h      # Layer 2   — Interface: EMC2301-compatible
│   │                      #              register map, ADC ID read,
│   │                      #              full measurement cycle, IRQ→event
│   │                      #              dispatch, PWM change event push,
│   │                      #              telemetry fill, statistics
│   ├── uiox_fan_drv.h     # Layer 2b  — Fan driver: per-channel descriptor
│   │                      #              (min/max RPM/duty), spin-up sequence
│   │                      #              (500ms @ 78%), stall detection,
│   │                      #              manual override, set_duty/set_pct
│   ├── uiox_fan_thermal.h # Layer 3   — Thermal engine: step table (fan
│   │                      #              curve), hysteresis on/off, PID
│   │                      #              (anti-windup, derivative), per-zone
│   │                      #              temp sensor → fan mapping,
│   │                      #              emergency full-speed at critical temp
│   ├── uiox_fan_subsys.h  # Layer 4   — Subsystem: IRQ + measurement tick,
│   │                      #              fan driver tick (stall/spin-up),
│   │                      #              thermal tick, watchdog kick,
│   │                      #              event dispatch, state machine
│   └── uiox_fan_device.h  # Layer 5   — Application API: open/start/stop/
│                          #              close/tick/add_fan/add_zone/
│                          #              set_duty/set_pct/set_manual/
│                          #              get_rpm/get_pct/get_temp/stalled/
│                          #              get_telemetry/print_info/print_stats
└── src/
    ├── uiox_fan_hw.c      # HAL lifecycle: init/deinit, set_pwm, read_rpm,
    │                      #   read_temp, fault_status/clear, chan_enable
    ├── uiox_fan_buf.c     # Circular event log push/pop/count,
    │                      #   telemetry pool alloc/free
    ├── uiox_fan_if.c      # Config + start (ID read + chan enable + min PWM
    │                      #   + IRQ unmask), stop (ramp-down), full measure,
    │                      #   set_pwm with event, IRQ→fault→event, telemetry
    ├── uiox_fan_drv.c     # Fan registration, set_duty (spin-up detect),
    │                      #   set_pct (% → duty), tick (stall detect,
    │                      #   spin-up timeout), manual override
    ├── uiox_fan_thermal.c # zone init, PID (kp/ki/kd, anti-windup, clamp),
    │                      #   step table binary search, hysteresis,
    │                      #   emergency full-speed on critical temp,
    │                      #   tick dispatches per-zone controller
    ├── uiox_fan_subsys.c  # init chain, start (IF start + fan enable),
    │                      #   stop (IF stop → ramp down), tick: IRQ →
    │                      #   measure → drv_tick → thermal_tick → WDT kick
    ├── uiox_fan_device.c  # All API wrappers, print_info/print_stats
    │                      #   (per-fan RPM/PWM/stall, per-sensor temp,
    │                      #   IF stats), print_events, state/event names
    └── uiox_fan_demo.c    # EMC2301 stub HAL (2 fans, 2 temps), step-curve
                           #   CPU zone + hysteresis SYS zone, 8-tick thermal
                           #   simulation, manual override, stall fault,
                           #   telemetry snapshot, statistics, event log
===========================================================================
Key Design Decisions:
Decision	Rationale
Three thermal controllers (step/hysteresis/PID)	Step table is easiest to tune for laptops (direct from thermal specification); hysteresis suits simple on/off accessories (case fan); PID is best for servers where steady-state precision matters
Spin-up sequence (78% duty, 500 ms)	NMB / Delta fans need minimum voltage to overcome static friction; spin-up at 78% guarantees starting even at 0°C, then drops to target — prevents false stall events
Stall detection at STALL_RPM_MIN=200	Below 200 RPM is physically impossible for a spinning fan — distinguishes stalled from stopped (duty=0); generates STALL event with timestamp for FRU logging
PID anti-windup integral clamp	Without the clamp, the integrator saturates during prolonged high-temperature events and then produces a large undershoot when cooling — clamp at output_max prevents this
Emergency full-speed override at critical temp	Any thermal zone exceeding critical_temp_dc forces all fans to 100% regardless of controller type — hardware safety net independent of policy
Per-channel event log with PWM_CHANGE events	Every duty transition is logged with timestamp, RPM, and temperature — allows post-mortem analysis of fan profile during thermal events
Manual override with event push	Manual mode fires MANUAL_OVERRIDE/AUTO_RESTORE events — system management software can detect and warn the user that thermal policy is bypassed
64-entry FIFO event log	Non-blocking push; overwrite oldest on overflow; captures burst events (stall → fault → overheat in one tick cycle) without dropping any
8-slot telemetry pool	Pre-allocated zero-copy snapshots; no malloc in measurement path; caller returns slot after processing
Ramp-down on stop	uiox_fan_if_stop() sets PWM=0 on all channels — prevents fans from spinning indefinitely if firmware crashes after graceful shutdown
Portable Makefile	LDFLAGS:= empty; -lm only for fabs/powf in PID layer; no --gc-sections/--as-needed — works with GNU ld, Apple ld (macOS), LLVM lld
Vtable ops pattern	EMC2301, NCT6793D, MAX6639, IT8987E all plug into the same HAL vtable without modifying upper layers