Application / Device Access API   (uiox_us_device)
  → Ultrasonic Subsystem: pipeline, echo processing, zone detection (uiox_us_subsys)
    → Signal Processing: envelope, threshold, ToF, temperature comp  (uiox_us_dsp)
    → Sensor abstraction: pulse config, SPI/UART reg map             (uiox_us_sensor)
    → Interface driver: GPIO trigger/echo, SPI/UART data capture      (uiox_us_if)
      → Hardware Abstraction: MMIO, timer, DMA, IRQ, clocks           (uiox_us_hw)
    ↔ Buffer Manager: echo sample buffers, zero-copy                  (uiox_us_buf)
/////////////////////////////////////

uiox-ultrasonic/
├── include/
│   ├── uiox_us_hw.h        # Layer 1   — HAL: timer, GPIO trigger/echo,
│   │                       #              DMA, IRQ, SPI, UART ops vtable
│   ├── uiox_us_buf.h       # Layer 1.5 — Raw ADC + DSP output buffer
│   │                       #              pools, zero-copy frame descriptors
│   ├── uiox_us_if.h        # Layer 2   — Interface driver: GPIO/SPI/UART/ADC
│   │                       #              trigger+echo, DMA buffer priming
│   ├── uiox_us_sensor.h    # Layer 2b  — Sensor abstraction: pulse config,
│   │                       #              temp compensation, ticks→metres
│   ├── uiox_us_dsp.h       # Layer 3   — DSP: DC removal, bandpass IIR,
│   │                       #              envelope detection, threshold,
│   │                       #              sub-sample interpolation, median
│   ├── uiox_us_subsys.h    # Layer 4   — Pipeline, zone classification,
│   │                       #              hysteresis, moving average, stats
│   └── uiox_us_device.h    # Layer 5   — Application API: open/start/stop/
│                           #              close/measure/zone/stats
└── src/
    ├── uiox_us_hw.c        # HAL lifecycle: init/trigger/echo_wait/
    │                       #   read_temp/deinit
    ├── uiox_us_buf.c       # Static pool alloc/free/ref, two pools
    ├── uiox_us_if.c        # IF config, GPIO/ADC measurement dispatch
    ├── uiox_us_sensor.c    # Pulse programming, SoS from temperature,
    │                       #   ticks-to-metres conversion
    ├── uiox_us_dsp.c       # Biquad BP filter, envelope LP filter,
    │                       #   threshold detection, parabolic interp,
    │                       #   median filter, GPIO-mode path
    ├── uiox_us_subsys.c    # Pipeline build/config/start/stop,
    │                       #   round-robin measurement, zone + hysteresis,
    │                       #   moving average, per-channel statistics
    ├── uiox_us_device.c    # Device open/start/stop/close/measure/
    │                       #   zone/set_pulse/update_temp/get_stats
    └── uiox_us_demo.c      # End-to-end demo: 4-channel HC-SR04
                            #   with stub GPIO HAL ops
////////////
Decision,Rationale
Dual static buffer pools,Separate raw ADC and DSP pools prevent cross-contamination; no heap fragmentation on RTOS
Biquad bandpass IIR,"Bilinear-transform 2nd-order filter — minimal state (4 coefficients + 4 delay cells), fixed-point-portable"
Single-pole envelope LP,One multiply-add per sample; sufficient for 40 kHz envelope at 1 Msps
Parabolic sub-sample interpolation,Improves ToF resolution beyond sample period without extra hardware
Median filter on distance output,"Rejects outlier echoes (multi-path, false detections) without introducing lag"
Hysteresis on zone transitions,Prevents zone flicker when object hovers near a threshold boundary
Moving average smoothing,Low-latency distance smoothing with configurable window; complements median
Temperature-compensated SoS,Laplace approximation 331.3 × √(1 + T/273.15); corrects ~0.6 m/s per °C
GPIO + ADC dual path,Same DSP and subsystem layers serve both simple pulse-width sensors (HC-SR04) and analog-frontend ADC sensors (TDC1000)
Round-robin multi-channel,Sequential firing prevents crosstalk between sensors; inter-trigger delay is configurable via period_ms
Vtable ops pattern,"Concrete HAL drivers (STM32 TIM, NXP FTM, RP2040 PIO) plug in without modifying upper layers"