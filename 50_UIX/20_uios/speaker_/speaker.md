Application / Device Access API        (uiox_spk_device)
  → Speaker Subsystem: playback, mixing, EQ, volume     (uiox_spk_subsys)
    → Audio DSP: resampling, EQ, limiter, crossover      (uiox_spk_dsp)
    → Codec/Amplifier abstraction: I2C config, gain      (uiox_spk_codec)
    → Interface driver: I2S/PCM/PDM DMA audio path       (uiox_spk_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks     (uiox_spk_hw)
    ↔ Buffer Manager: PCM audio ring buffer pool         (uiox_spk_buf)
==============================
uiox-speaker/
├── include/
│   ├── uiox_spk_hw.h          # Layer 1  — HAL: I2S/PCM/PDM, DMA, IRQ
│   ├── uiox_spk_buf.h         # PCM audio buffer pool + ring buffer
│   ├── uiox_spk_if.h          # Layer 2  — Interface driver (DMA audio)
│   ├── uiox_spk_codec.h       # Codec/amplifier abstraction (I2C/SPI)
│   ├── uiox_spk_dsp.h         # Layer 3  — DSP: EQ, resampler, limiter
│   ├── uiox_spk_subsys.h      # Layer 4  — Subsystem: playback, mix
│   └── uiox_spk_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_spk_hw.c
    ├── uiox_spk_buf.c
    ├── uiox_spk_if.c
    ├── uiox_spk_codec.c
    ├── uiox_spk_dsp.c
    ├── uiox_spk_subsys.c
    ├── uiox_spk_device.c
    └── uiox_spk_demo.c
============================================
uiox-speaker/
├── include/
│   ├── uiox_spk_hw.h      # Layer 1   — HAL: I2S/PCM/PDM/TDM MMIO,
│   │                      #              DMA descriptor ring, audio PLL,
│   │                      #              MCLK/BCLK/LRCK, I2C codec bus,
│   │                      #              GPIO mute/power, fault IRQ
│   ├── uiox_spk_buf.h     # Layer 1.5 — PCM frame pool (4 frames, 10ms each),
│   │                      #              mix scratch pool, SPSC ring buffer
│   │                      #              (4096 stereo int16 pairs)
│   ├── uiox_spk_if.h      # Layer 2   — Interface: DMA double-buffer loop,
│   │                      #              underrun silence fill, ring→DMA refill,
│   │                      #              frame rotation, IF statistics
│   ├── uiox_spk_codec.h   # Layer 2b  — Codec abstraction: TAS5756/MAX98357/
│   │                      #              NAU8822/WM8960/TLV320, I2C reg map,
│   │                      #              volume/mute/bass/treble/format config
│   ├── uiox_spk_dsp.h     # Layer 3   — DSP: 5-band peaking EQ (biquad IIR),
│   │                      #              soft limiter (tanh saturation),
│   │                      #              master gain, fade-in/out (pop suppress)
│   ├── uiox_spk_subsys.h  # Layer 4   — Subsystem: 4-stream software mixer,
│   │                      #              per-stream gain + loop, DSP pipeline,
│   │                      #              master volume, HW+SW EQ, pause/resume,
│   │                      #              event callbacks, playback state machine
│   └── uiox_spk_device.h  # Layer 5   — Application API: open/start/stop/pause/
│                          #              resume/close/tick/play/stop_stream/
│                          #              write/set_volume/set_mute/set_eq/
│                          #              set_bass/set_treble/state/print_stats
└── src/
    ├── uiox_spk_hw.c      # HAL lifecycle: init/deinit/start/stop,
    │                      #   set_format/set_volume/set_mute, dma_submit
    ├── uiox_spk_buf.c     # PCM + mix pool: build_pool/alloc/free,
    │                      #   SPSC ring write/read/avail/space/flush
    ├── uiox_spk_if.c      # Config+start+stop, DMA refill (ring→frame),
    │                      #   silence fill on underrun, stats
    ├── uiox_spk_codec.c   # TAS5756-style I2C register map, power on,
    │                      #   volume/mute/bass/treble/format programming
    ├── uiox_spk_dsp.c     # Peaking EQ biquad build (Audio EQ Cookbook),
    │                      #   per-sample biquad chain, tanh limiter,
    │                      #   fade gain step accumulation
    ├── uiox_spk_subsys.c  # init/start/stop/pause/resume, 4-stream mixer
    │                      #   (clip-guarded accumulation), DSP process,
    │                      #   ring write, DMA refill trigger, fade stop
    ├── uiox_spk_device.c  # All API wrappers, print_stats,
    │                      #   state/event name helpers
    └── uiox_spk_demo.c    # TAS5756 + I2S stub HAL, synthetic 440/660 Hz
                           #   sines + noise, multi-stream mix, EQ/bass/treble,
                           #   pause/resume, mute, fade stop, stats
============================
Key Design Decisions
Decision	Rationale
Double-buffered DMA with silence fill	On underrun, silence is inserted rather than replaying stale data or crashing; prevents audible glitches during CPU overload
SPSC lock-free ring buffer	Application fill task and DMA ISR bottom-half operate independently without mutex; 4096-pair ring gives ~85 ms latency budget at 48 kHz
10 ms PCM frame pool (4 frames)	4 × 10 ms = 40 ms total pool; enough for one audio processing cycle to miss without underrun; deterministic, no heap
5-band peaking EQ via biquad IIR	Standard Audio EQ Cookbook coefficients; per-channel state (L+R independent); 4 multiplies + 4 adds per sample per band — NEON-friendly
tanh soft limiter	Smooth saturation curve prevents harsh digital clipping; cheaper than look-up table on targets with FPU
Fade-in/out on start/stop/mute	Eliminates power-on/off pop and mute click — mandatory for consumer audio quality
4-stream software mixer	Clip-guarded int32 accumulation before int16 clamp; per-stream gain; loop support for sound effects
Codec abstraction layer	TAS5756, MAX98357, WM8960 all use different I2C register maps but share the same volume/mute/EQ interface; swap codec type without changing upper layers
Hardware + software EQ dual path	SW DSP handles fine-grained 5-band EQ; HW codec bass/treble registers give fast coarse adjustment; both updated together
Pop-safe pause via GPIO mute	Hardware mute pin cuts output immediately; resume unmutes after DSP fade completes — no relay click
Vtable ops pattern	STM32 SAI, NXP SAI, RP2040 PIO, ESP32 I2S concrete drivers plug in without modifying upper layers