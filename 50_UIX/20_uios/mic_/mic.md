Application / Device Access API        (uiox_mic_device)
  → Mic Subsystem: AGC, VAD, beamform, noise cancel      (uiox_mic_subsys)
    → Audio DSP: FFT, bandpass, spectral subtract         (uiox_mic_dsp)
    → Codec abstraction: I2C config, gain, sample rate    (uiox_mic_codec)
    → Interface driver: I2S/PDM/TDM DMA capture path     (uiox_mic_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks      (uiox_mic_hw)
    ↔ Buffer Manager: PCM capture ring buffer pool        (uiox_mic_buf)
=========================================================================
uiox-mic/
├── include/
│   ├── uiox_mic_hw.h          # Layer 1  — HAL: I2S/PDM/TDM, DMA, IRQ
│   ├── uiox_mic_buf.h         # PCM capture buffer pool + ring buffer
│   ├── uiox_mic_if.h          # Layer 2  — Interface driver (DMA capture)
│   ├── uiox_mic_codec.h       # Codec/MEMS abstraction (I2C/SPI config)
│   ├── uiox_mic_dsp.h         # Layer 3  — DSP: AGC, VAD, noise, beamform
│   ├── uiox_mic_subsys.h      # Layer 4  — Subsystem: capture, processing
│   └── uiox_mic_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_mic_hw.c
    ├── uiox_mic_buf.c
    ├── uiox_mic_if.c
    ├── uiox_mic_codec.c
    ├── uiox_mic_dsp.c
    ├── uiox_mic_subsys.c
    ├── uiox_mic_device.c
    └── uiox_mic_demo.c
==========================================================================
uiox-mic/
├── include/
│   ├── uiox_mic_hw.h      # Layer 1   — HAL: I2S/PDM/TDM MMIO,
│   │                      #              DMA descriptor ring, audio PLL,
│   │                      #              MCLK/BCLK/LRCK, I2C codec bus,
│   │                      #              GPIO enable/mute/power, overrun IRQ,
│   │                      #              hw_ops vtable
│   ├── uiox_mic_buf.h     # Layer 1.5 — PCM capture frame pool (4 frames,
│   │                      #              10ms each), processing scratch pool,
│   │                      #              SPSC ring buffer (8192 mono int16)
│   ├── uiox_mic_if.h      # Layer 2   — Interface: DMA double-buffer prime,
│   │                      #              frame rotation on completion,
│   │                      #              multi-channel → mono downmix,
│   │                      #              ring write, read, IF statistics
│   ├── uiox_mic_codec.h   # Layer 2b  — Codec abstraction: ICS43434/SPH0645/
│   │                      #              MP34DT05/ADMP441/WM8731/TLV320/NAU8822,
│   │                      #              I2C gain/mute/format/sample-rate regs
│   ├── uiox_mic_dsp.h     # Layer 3   — DSP: DC offset removal (leaky int.),
│   │                      #              high-pass biquad IIR (80 Hz),
│   │                      #              spectral subtraction noise gate,
│   │                      #              AGC (attack/release per-sample),
│   │                      #              VAD (energy threshold dBFS)
│   ├── uiox_mic_subsys.h  # Layer 4   — Subsystem: full pipeline tick,
│   │                      #              DMA refill trigger, VAD transition
│   │                      #              events (VOICE_START/END), gain/mute,
│   │                      #              event callbacks, statistics
│   └── uiox_mic_device.h  # Layer 5   — Application API: open/start/stop/
│                          #              close/tick/read/set_gain/set_mute/
│                          #              voice_active/energy_dbfs/print_stats
└── src/
    ├── uiox_mic_hw.c      # HAL lifecycle: init/deinit/start/stop,
    │                      #   set_fmt/set_gain/set_mute, dma_submit
    ├── uiox_mic_buf.c     # PCM + proc pool: build_pool/alloc/free,
    │                      #   SPSC ring write/read/avail/space/flush
    ├── uiox_mic_if.c      # Config + start (prime 2 frames), stop,
    │                      #   DMA refill (rotate + downmix + ring write),
    │                      #   read from ring, stats
    ├── uiox_mic_codec.c   # WM8731-style I2C register map, power on,
    │                      #   gain/mute/format/sample-rate programming
    ├── uiox_mic_dsp.c     # HP biquad build (EQ Cookbook), DC removal,
    │                      #   noise floor estimation, spectral gate,
    │                      #   AGC attack/release per-sample coefficients,
    │                      #   VAD energy accumulation + dBFS decision
    ├── uiox_mic_subsys.c  # init/start/stop, tick (DMA refill + VAD events),
    │                      #   set_gain/set_mute, read (ring + DSP process)
    ├── uiox_mic_device.c  # All API wrappers, print_stats (VAD/AGC/ring),
    │                      #   state/event name helpers
    └── uiox_mic_demo.c    # ICS43434 stub HAL, 1 kHz sine + noise generator,
                           #   AGC + VAD + noise cancel, mute/unmute, stats
===========================================================================
Key Design Decisions:
Decision	Rationale
Static PCM frame pool (4 × 10 ms)	40 ms total latency budget; no heap allocation in the audio ISR path; deterministic on embedded/RTOS
SPSC lock-free ring (8192 samples)	Producer = DMA refill bottom-half, Consumer = application read; 512 ms at 16 kHz — generous buffer for burst reads
Multi-channel → mono downmix in IF layer	DSP and application always see mono int16 regardless of hardware channel count; simplifies all upper layers
HP biquad IIR at 80 Hz	Removes wind/handling noise and DC drift below 80 Hz; Q=1/√2 Butterworth — maximally flat, no ringing
Leaky integrator DC removal	α=0.001 per-sample removes slow DC drift without affecting speech frequencies; computationally trivial
Spectral subtraction noise gate	Noise floor estimated from first 200 ms; samples below 1.5× floor attenuated by 90% — effective for stationary noise
AGC per-sample attack/release	Attack 5 ms (fast gain reduction prevents clipping), release 100 ms (slow gain recovery avoids pumping)
VAD energy threshold (dBFS)	Simple but effective for embedded: no FFT needed; configurable threshold default −40 dBFS; fires VOICE_START/END events
Codec abstraction layer	ICS43434 (I2S, no config needed), SPH0645 (I2S), WM8731 (I2C) all use different register maps but share gain/mute/format API
Portable Makefile (no GNU ld flags)	LDFLAGS left empty — no --gc-sections / --as-needed — works with both GNU ld and Apple ld (macOS Clang)
Vtable ops pattern	STM32 SAI, NXP SAI, RP2040 PIO, ESP32 I2S concrete drivers plug in without modifying upper layers
Pop-free mute via codec	Hardware mute register cuts output before DSP — no click from abrupt gain change