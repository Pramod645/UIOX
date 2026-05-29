Application / Device Access API  (uiox_radar_device)
  → Radar Subsystem: pipeline, CFAR, FFT, tracking  (uiox_radar_subsys)
    → Radar Signal Processing: range/doppler/angle  (uiox_radar_dsp)
    → Sensor abstraction: SPI/UART reg map, chirp cfg (uiox_radar_sensor)
    → Interface driver: SPI DMA, LVDS, CSI-2 data    (uiox_radar_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks  (uiox_radar_hw)
    ↔ Buffer Manager: ADC frame buffers, zero-copy    (uiox_radar_buf)
/////////////////////////////////////////////////////////////////////
uiox-radar/
├── include/
│   ├── uiox_radar_hw.h       # Layer 1  — HAL: MMIO regs, DMA rings,
│   │                         #             IRQ, SPI read/write ops vtable
│   ├── uiox_radar_buf.h      # Layer 1.5 — Raw ADC + DSP output buffer
│   │                         #             pools, zero-copy frame descriptors
│   ├── uiox_radar_if.h       # Layer 2  — Interface driver (LVDS/CSI-2/SPI)
│   │                         #             DMA buffer priming and dequeue
│   ├── uiox_radar_sensor.h   # Layer 2b — Sensor abstraction: chirp config,
│   │                         #             SPI register map, perf metrics
│   ├── uiox_radar_dsp.h      # Layer 3  — DSP: range FFT, Doppler FFT,
│   │                         #             CA-CFAR, angle FFT beamforming
│   ├── uiox_radar_subsys.h   # Layer 4  — Pipeline, nearest-neighbour
│   │                         #             tracker, point cloud output
│   └── uiox_radar_device.h   # Layer 5  — Application API: open/start/
│                             #             stop/close/get_point_cloud
└── src/
    ├── uiox_radar_hw.c       # HAL lifecycle: init/start/stop/deinit
    ├── uiox_radar_buf.c      # Static pool alloc/free/ref, two pools
    ├── uiox_radar_if.c       # IF config, DMA prime, frame dequeue
    ├── uiox_radar_sensor.c   # Chirp programming, perf computation,
    │                         #   soft-reset, stream enable
    ├── uiox_radar_dsp.c      # FFT (radix-2 DIT), windowing, CFAR,
    │                         #   angle estimation, full pipeline
    ├── uiox_radar_subsys.c   # Pipeline build/config/start/stop,
    │                         #   tracker update, Cartesian conversion
    ├── uiox_radar_device.c   # Device open/start/stop/close,
    │                         #   set_chirp, get_perf, get_tracks
    └── uiox_radar_demo.c     # End-to-end demo with stub SPI + HAL ops
/////////////////
Decision,Rationale
Dual static buffer pools,Separate raw ADC and DSP pools prevent cross-contamination; no heap fragmentation on RTOS
Radix-2 DIT FFT,"Self-contained, no FFTW dependency; portable to bare-metal ARM with NEON vectorisation"
CA-CFAR (2D),Cell-averaging gives constant false-alarm rate across varying clutter; guard cells prevent self-masking
Angle FFT beamforming,Virtual aperture across RX channels; maps naturally to ULA with half-wavelength spacing
α-β tracker,"Simple, deterministic, low-memory — sufficient for embedded radar; extend to Kalman for production"
Nearest-neighbour association,Low complexity O(D×T); g