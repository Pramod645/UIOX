Application / Device Access API      (uiox_mon_device)
  → Monitor Subsystem: pipeline, OSD, backlight, EDID  (uiox_mon_subsys)
    → Display Signal Processing: scaling, color, gamma  (uiox_mon_dsp)
    → Panel abstraction: timing, EDID, mode setting     (uiox_mon_panel)
    → Interface driver: HDMI/DP/MIPI/LVDS signal path   (uiox_mon_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks     (uiox_mon_hw)
    ↔ Buffer Manager: framebuffer pool, zero-copy        (uiox_mon_buf)
/////////////////
uiox-monitor/
├── include/
│   ├── uiox_mon_hw.h         # Layer 1  — HAL: MMIO, DMA, IRQ, PLL, clocks
│   ├── uiox_mon_buf.h        # Framebuffer pool, zero-copy page-flip
│   ├── uiox_mon_if.h         # Layer 2  — Interface driver (HDMI/DP/MIPI/LVDS)
│   ├── uiox_mon_panel.h      # Panel abstraction: EDID, timing, modes
│   ├── uiox_mon_dsp.h        # Layer 3  — DSP: scaling, color, gamma, OSD
│   ├── uiox_mon_subsys.h     # Layer 4  — Subsystem: pipeline, hotplug, DPMS
│   └── uiox_mon_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_mon_hw.c
    ├── uiox_mon_buf.c
    ├── uiox_mon_if.c
    ├── uiox_mon_panel.c
    ├── uiox_mon_dsp.c
    ├── uiox_mon_subsys.c
    ├── uiox_mon_device.c
    └── uiox_mon_demo.c

////////////////////////
uiox-monitor/
├── include/
│   ├── uiox_mon_hw.h       # Layer 1   — HAL: MMIO regs, PLL/pixel-clock,
│   │                       #              DMA scanout, HDMI/DP/MIPI/LVDS PHY,
│   │                       #              VBlank IRQ, hotplug IRQ, DDC I2C,
│   │                       #              backlight PWM, gamma LUT, DPMS
│   ├── uiox_mon_buf.h      # Layer 1.5 — Triple framebuffer pool,
│   │                       #              zero-copy page-flip, state machine,
│   │                       #              clear/copy helpers
│   ├── uiox_mon_if.h       # Layer 2   — Interface driver: HDMI TMDS,
│   │                       #              DP link training, MIPI DSI v/cmd mode,
│   │                       #              LVDS serialiser, pixel clock PLL lock,
│   │                       #              flip submission, vsync wait, stats
│   ├── uiox_mon_panel.h    # Layer 2b  — Panel abstraction: EDID E-EDID 1.4
│   │                       #              parsing, DTD→timing decode, mode DB,
│   │                       #              preferred/native mode selection,
│   │                       #              power-on/off sequence
│   ├── uiox_mon_dsp.h      # Layer 3   — DSP: power-law gamma LUT,
│   │                       #              brightness/contrast/saturation,
│   │                       #              colour temperature (warm/cool/neutral),
│   │                       #              blue-light filter, nearest/bilinear
│   │                       #              scaling, OSD compositor
│   ├── uiox_mon_subsys.h   # Layer 4   — Subsystem: acquire/present/triple-buf,
│   │                       #              DSP pipeline, hotplug re-probe,
│   │                       #              DPMS auto-blank, frame pacing,
│   │                       #              VBlank/hotplug callbacks, statistics
│   └── uiox_mon_device.h   # Layer 5   — Application API: open/start/stop/
│                           #              close/acquire/present/tick/activity/
│                           #              set_backlight/set_dpms/set_gamma/
│                           #              set_brightness/set_contrast/
│                           #              set_colour_mode/osd_add/osd_remove/
│                           #              connected/get_resolution/print_stats
└── src/
    ├── uiox_mon_hw.c       # HAL lifecycle: init/deinit/enable/disable,
    │                       #   set_timing/set_pixfmt/flip/wait_vblank,
    │                       #   read_edid/set_dpms/set_backlight/connected
    ├── uiox_mon_buf.c      # Triple framebuffer pool: init/alloc/free/ref,
    │                       #   clear (solid fill), copy (pixel memcpy)
    ├── uiox_mon_if.c       # IF config (timing+pixfmt→HAL), enable/disable,
    │                       #   flip submit, vsync wait, stats get/reset
    ├── uiox_mon_panel.c    # EDID checksum, manufacturer decode, DTD parser,
    │                       #   name/range descriptor parsing, mode selection,
    │                       #   power_on/off sequence, panel_print
    ├── uiox_mon_dsp.c      # Gamma LUT build (power-law + blue filter +
    │                       #   colour temperature), per-pixel brightness/
    │                       #   contrast/saturation, nearest+bilinear scaler,
    │                       #   OSD rect fill (alpha blend) + text stub
    ├── uiox_mon_subsys.c   # Pipeline init/start/stop, acquire/present
    │                       #   (DSP→flip→vsync→buffer rotation),
    │                       #   hotplug tick, DPMS auto-blank, activity wake
    ├── uiox_mon_device.c   # Device open/close, all API wrappers,
    │                       #   dpms_name string helper, print_stats
    └── uiox_mon_demo.c     # End-to-end demo: stub HDMI HAL + fake EDID,
                            #   solid/gradient/colour-bars/grid patterns,
                            #   colour adjustments, backlight, OSD, DPMS,
                            #   VBlank/hotplug callbacks, statistics
////////////////////////////
Makefile Usage Reference
Command,Effect
make,Native debug build (auto-detects platform)
make BUILD=release,"Optimised, stripped release build"
make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-,Cross-compile for ARM64
make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf-,Cross-compile for ARM32
make lib,Build static library only
make demo,Build demo binary (depends on lib)
make install PREFIX=/opt/uiox,Install headers + lib + binary
make uninstall PREFIX=/opt/uiox,Remove installed files
make size,Print text/data/bss breakdown
make dump,Generate disassembly → .asm file
make lint,Run cppcheck static analysis
make format,Auto-format all sources with clang-format
make docs,Generate Doxygen HTML documentation
make tags,Generate ctags index
make info,Print full build configuration summary
make clean,Remove current platform/mode build artefacts
make distclean,Remove entire build/ tree
make V=1,Verbose — print full compiler commands
/////////////////
Key Design Decisions
Decision,Rationale
Triple framebuffer pool,"Eliminates tearing (render into back while front is displayed), absorbs one frame of latency without stalling the CPU"
Zero-copy page flip,DMA scanout address is switched at VBlank; no pixel memcpy on the display path — critical for FHD/4K at 60 Hz
EDID E-EDID 1.4 parser,Full DTD decode from raw 128-byte block; manufacturer/name/range descriptors; no external EDID library dependency
Power-law gamma LUT,"Pre-computed 256-entry table replaces per-pixel powf() in the hot path; rebuild on parameter change, not per frame"
Bilinear + nearest scaler,Both modes in software for when hardware scaler is unavailable; bilinear avoids blockiness for UI upscaling
OSD alpha compositor,Fixed-point alpha blend for rectangles; text uses a bitmap-font stub (extend with FreeType for production)
Colour temperature presets,Warm/cool channel adjustments baked into the gamma LUT at build time — zero per-pixel overhead at render time
DPMS auto-blank,Inactivity timer drives STANDBY/SUSPEND/OFF; uiox_mon_activity() wakes immediately — standard X11 DPMS model
Hotplug re-probe on connect,"On reconnect, EDID is re-read and best mode re-selected automatically; application receives callback"
Frame pacing,target_fps cap prevents tearing and reduces power on panels that support variable refresh
Vtable ops pattern,"HDMI, DP, MIPI DSI, LVDS, parallel-RGB drivers plug in without modifying upper layers"
DDC I2C in HAL,Keeps EDID read in hardware-specific layer; upper layers always see parsed uiox_mon_edid_t — portable across controller families
////////////////////////////////
