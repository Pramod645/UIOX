Application / Device Access API        (uiox_therm_device)
  → Thermal Subsystem: zones, trips, throttle, alerts     (uiox_therm_subsys)
    → Thermal Policy: cooling, DVFS link, hysteresis       (uiox_therm_policy)
    → Sensor abstraction: NTC, PCT2075, LM75, TMP117       (uiox_therm_sensor)
    → Interface driver: I2C/SPI/ADC register map, IRQ      (uiox_therm_if)
      → Hardware Abstraction: MMIO, I2C, ADC, GPIO, IRQ    (uiox_therm_hw)
    ↔ Buffer Manager: measurement log + alert pool         (uiox_therm_buf)
============================================================================
uiox-thermal/
├── include/
│   ├── uiox_therm_hw.h          # Layer 1  — HAL: I2C/SPI/ADC, GPIO, IRQ
│   ├── uiox_therm_buf.h         # Measurement log + alert pool
│   ├── uiox_therm_if.h          # Layer 2  — Interface: reg map, IRQ
│   ├── uiox_therm_sensor.h      # Sensor abstraction: NTC, digital ICs
│   ├── uiox_therm_policy.h      # Layer 3  — Policy: trips, throttle, DVFS
│   ├── uiox_therm_subsys.h      # Layer 4  — Subsystem: zones, alerts
│   └── uiox_therm_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_therm_hw.c
    ├── uiox_therm_buf.c
    ├── uiox_therm_if.c
    ├── uiox_therm_sensor.c
    ├── uiox_therm_policy.c
    ├── uiox_therm_subsys.c
    ├── uiox_therm_device.c
    └── uiox_therm_demo.c
=============================================================================
Key Design Decisions:
Decision	Rationale
NTC Beta equation (Steinhart-Hart simplified)	More accurate than linear approximation over wide range; only one extra float (logf) vs. lookup table; Beta coefficient available on all NTC datasheets
Running average per sensor	Configurable window size (1–255 samples); removes transient noise without introducing a fixed latency; separate cur_dc and avg_dc allow both instantaneous and filtered values
Calibration offset per sensor	Each physical sensor placement has thermal coupling error (PCB self-heating, airflow gradient); per-sensor offset_dc corrects without changing the driver — set from characterisation data
Four trip point types (PASSIVE/ACTIVE/HOT/CRITICAL)	Maps directly to Linux thermal framework concepts; PASSIVE = SW CPU throttle, ACTIVE = fan speed-up, HOT = OS warning, CRITICAL = emergency shutdown — familiar to system integrators
Per-trip hysteresis	Without hysteresis, a sensor hovering at the trip threshold causes rapid repeated THROTTLE_ON/OFF events (thermal oscillation); independent hysteresis per trip allows fine-grained tuning
Per-trip callback	Application registers a function pointer at zone creation time; executed synchronously on trip crossing — no event dispatch overhead for time-critical throttle actions
Alert threshold in hardware	PCT2075/TMP112 have hardware comparators that assert ALERT# GPIO independently of the software poll cycle; IRQ handler wakes the subsystem tick without burning CPU on polling
Interrupt + comparator modes	Interrupt mode (latched, cleared by read) used by default; comparator mode (deasserts below T_hyst) suitable for fan control without software overhead
64-entry event log FIFO	Captures burst of events during thermal spike (TRIP_CROSSED + ZONE_HOT + CRITICAL in one tick); ring-buffer never blocks; oldest entry overwritten on overflow
Measurement interval configurable	Default 1000 ms; reduce to 200 ms for high-performance mode; increase to 5000 ms for battery-save; set at open_params without recompilation
Emergency state fires immediately	Critical trip callback + state transition happen inside policy_tick() within the same subsys_tick() call — no additional tick latency for emergency shutdown
Sensor error path	If read_temp fails, sensor is marked error=true, SENSOR_ERROR event pushed, channel skipped in policy evaluation — prevents false throttle from a bad reading
Portable Makefile	LDFLAGS:= empty; -lm only for logf in NTC conversion; no --gc-sections/--as-needed — works with GNU ld, Apple ld (macOS), LLVM lld
Vtable ops pattern	PCT2075, TMP117, TMP112, MAX31875, NTC all plug into the same HAL vtable; sensor type determines which fields of the ops table are populated
=======================================================================================
uiox-thermal/
├── include/
│   ├── uiox_therm_hw.h      # Layer 1   — HAL: I2C/SPI/ADC/MMIO bus,
│   │                        #              temperature read (all types),
│   │                        #              T_high/T_hyst/T_crit register
│   │                        #              programming, alert clear,
│   │                        #              resolution, shutdown, oneshot,
│   │                        #              NTC config, alert GPIO, ops vtable
│   ├── uiox_therm_buf.h     # Layer 1.5 — 64-entry circular event log,
│   │                        #              8-slot telemetry pool,
│   │                        #              11 event types (alert, critical,
│   │                        #              trip, throttle, zone, sensor error)
│   ├── uiox_therm_if.h      # Layer 2   — Interface: PCT2075/LM75 register
│   │                        #              map, alert threshold programming,
│   │                        #              full measurement cycle with
│   │                        #              per-channel alert/critical check,
│   │                        #              IRQ→event dispatch, telemetry,
│   │                        #              statistics
│   ├── uiox_therm_sensor.h  # Layer 2b  — Sensor abstraction: NTC Beta
│   │                        #              equation (Steinhart-Hart), per-
│   │                        #              sensor calibration offset, running
│   │                        #              average filter, valid range clamp,
│   │                        #              error flag, sensor manager registry
│   ├── uiox_therm_policy.h  # Layer 3   — Thermal policy: 4 trip types
│   │                        #              (PASSIVE/ACTIVE/HOT/CRITICAL),
│   │                        #              per-trip hysteresis, crossing
│   │                        #              detection, per-trip callback,
│   │                        #              throttle + emergency flags
│   ├── uiox_therm_subsys.h  # Layer 4   — Subsystem: IRQ alert check,
│   │                        #              periodic measurement, sensor
│   │                        #              update, policy tick, state FSM
│   │                        #              (RUNNING/ALERT/EMERGENCY),
│   │                        #              event dispatch, callback
│   └── uiox_therm_device.h  # Layer 5   — Application API: open/start/stop/
│                            #              close/tick/add_sensor/add_zone/
│                            #              set_alert/read/read_ch/alert_active/
│                            #              throttled/emergency/get_telemetry/
│                            #              print_info/print_stats/print_events
└── src/
    ├── uiox_therm_hw.c      # HAL lifecycle: init/deinit, read_temp,
    │                        #   set_t_high/hyst/crit, alert_clear
    ├── uiox_therm_buf.c     # Circular event log push/pop/count,
    │                        #   telemetry pool alloc/free
    ├── uiox_therm_if.c      # Config + start (threshold regs + alert cfg),
    │                        #   stop (shutdown mode), measure (all channels
    │                        #   + alert/critical event push), IRQ handle,
    │                        #   telemetry fill, set_alert wrapper, stats
    ├── uiox_therm_sensor.c  # NTC: Beta equation (voltage divider →
    │                        #   R_ntc → T via 1/T = 1/T0 + ln(R/R0)/B),
    │                        #   sensor register, update (digital + NTC),
    │                        #   running average, calibration offset, clamp
    ├── uiox_therm_policy.c  # Zone init, trip crossing (up/down with hyst),
    │                        #   event push per crossing type, callback fire,
    │                        #   throttle/emergency flag management
    ├── uiox_therm_subsys.c  # Init chain, start (IF start), stop (IF stop),
    │                        #   tick: alert IRQ → measure → sensor update
    │                        #   → policy tick → state update → event dispatch
    ├── uiox_therm_device.c  # All API wrappers, print_info (model/type/
    │                        #   channels/thresholds/caps/zones),
    │                        #   print_stats (per-sensor cur/avg/error,
    │                        #   throttle/emergency counts, IF stats),
    │                        #   print_events, state/event/type name helpers
    └── uiox_therm_demo.c    # PCT2075 stub HAL (3 channels), 3 named sensors
                             #   (CPU/GPU/AMB), CPU thermal zone with 3 trips
                             #   + GPU zone with 2 trips, 8-tick temperature
                             #   simulation ramp-up/cool-down, hardware alert
                             #   inject, new alert threshold, telemetry snap,
                             #   statistics, event log
