Application / Device Access API       (uiox_hdmi_device)
  → HDMI Subsystem: hotplug, EDID, HDCP, AVI infoframe (uiox_hdmi_subsys)
    → HDMI Protocol: infoframes, CEC, ARC, eARC         (uiox_hdmi_proto)
    → Panel/Sink: EDID parse, mode select, HDCP          (uiox_hdmi_sink)
    → Interface driver: TMDS/FRL lanes, PHY, scrambling  (uiox_hdmi_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, PLL        (uiox_hdmi_hw)
    ↔ Buffer Manager: audio/video packet buffer pool     (uiox_hdmi_buf)
/////////////////////
uiox-hdmi/
├── include/
│   ├── uiox_hdmi_hw.h         # Layer 1  — HAL: MMIO, PHY, PLL, IRQ
│   ├── uiox_hdmi_buf.h        # AV packet / infoframe buffer pool
│   ├── uiox_hdmi_if.h         # Layer 2  — Interface: TMDS/FRL, PHY, lanes
│   ├── uiox_hdmi_sink.h       # Sink: EDID parse, HDCP, mode select
│   ├── uiox_hdmi_proto.h      # Layer 3  — Protocol: infoframes, CEC, ARC
│   ├── uiox_hdmi_subsys.h     # Layer 4  — Subsystem: hotplug, pipeline
│   └── uiox_hdmi_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_hdmi_hw.c
    ├── uiox_hdmi_buf.c
    ├── uiox_hdmi_if.c
    ├── uiox_hdmi_sink.c
    ├── uiox_hdmi_proto.c
    ├── uiox_hdmi_subsys.c
    ├── uiox_hdmi_device.c
    └── uiox_hdmi_demo.c
///////////
uiox-hdmi/
├── include/
│   ├── uiox_hdmi_hw.h      # Layer 1   — HAL: MMIO regs, TMDS/FRL PHY,
│   │                       #              PLL, DDC I2C, CEC, HDCP engine,
│   │                       #              audio injection, VBlank/HPD IRQ
│   ├── uiox_hdmi_buf.h     # Layer 1.5 — Framebuffer pool (triple-buf),
│   │                       #              HDMI packet pool (infoframes),
│   │                       #              page-flip state machine
│   ├── uiox_hdmi_if.h      # Layer 2   — Interface: TMDS char rate,
│   │                       #              FRL lane training, scrambling,
│   │                       #              YCbCr 4:2:0 path, N/CTS ACR,
│   │                       #              audio write, stats
│   ├── uiox_hdmi_sink.h    # Layer 2b  — Sink: EDID block 0 + CEA-861
│   │                       #              parse, SAD/VIC/HDR/VRR/ALLM,
│   │                       #              mode selection, HDCP start/stop
│   ├── uiox_hdmi_proto.h   # Layer 3   — Protocol: AVI/Audio/HDR/SPD/
│   │                       #              VSIF infoframes, GCP AVMUTE,
│   │                       #              CEC command builder, periodic
│   │                       #              infoframe re-transmission
│   ├── uiox_hdmi_subsys.h  # Layer 4   — Subsystem: hotplug re-probe,
│   │                       #              triple-buffer pipeline, HDCP
│   │                       #              lifecycle, DPMS auto-blank,
│   │                       #              event callbacks, frame stats
│   └── uiox_hdmi_device.h  # Layer 5   — Application API: open/start/
│                           #              stop/close/tick/activity/
│                           #              acquire/present/set_hdr/
│                           #              set_audio/audio_write/cec_send/
│                           #              connected/get_resolution/stats
└── src/
    ├── uiox_hdmi_hw.c      # HAL lifecycle, set_timing, flip, vblank,
    │                       #   ddc_read, connected (HPD)
    ├── uiox_hdmi_buf.c     # Triple FB pool + packet pool: init/alloc/
    │                       #   free/ref, clear_fb
    ├── uiox_hdmi_if.c      # N/CTS ACR, TMDS/FRL selection, scrambling,
    │                       #   PLL program, colorspace, flip, vsync,
    │                       #   audio cfg + write, stats
    ├── uiox_hdmi_sink.c    # EDID checksum, manufacturer decode, DTD
    │                       #   parser, CEA-861 block parser (audio/video/
    │                       #   HDR/VRR/ALLM/FRL), mode select, HDCP,
    │                       #   
    ├── uiox_hdmi_sink.c    # EDID checksum, manufacturer decode, DTD
    │                       #   parser, CEA-861 block parser (audio/video/
    │                       #   HDR/VRR/ALLM/FRL), mode select, HDCP
    │                       #   start/stop, sink_print
    ├── uiox_hdmi_proto.c   # AVI infoframe build (Y/C/M/R/VIC),
    │                       #   Audio infoframe, HDR Static Metadata,
    │                       #   SPD (space-fill UTF-8), GCP AVMUTE,
    │                       #   CEC send/recv, periodic tick
    ├── uiox_hdmi_subsys.c  # Subsystem init/start/stop, AVMUTE on
    │                       #   startup, triple-buffer acquire/present/
    │                       #   vsync rotation, hotplug poll, HDCP
    │                       #   status poll, infoframe refresh, DPMS
    ├── uiox_hdmi_device.c  # Device open/close, all API wrappers,
    │                       #   print_info/print_stats, evt/state name
    │                       #   string helpers
    └── uiox_hdmi_demo.c    # End-to-end demo: synthetic EDID build,
                            #   stub HDMI TX HAL, EDID parse, 4K@60
                            #   mode, HDR10 metadata, 2ch LPCM audio,
                            #   CEC commands, colour-bar/solid frames,
                            #   HDCP auth, DPMS, statistics

///////////////////////////
build/
└── X86_64/              ← or ARM64 / ARM32
    └── debug/           ← or release
        ├── obj/
        │   ├── uiox_hdmi_hw.o      + .d
        │   ├── uiox_hdmi_buf.o     + .d
        │   ├── uiox_hdmi_if.o      + .d
        │   ├── uiox_hdmi_sink.o    + .d
        │   ├── uiox_hdmi_proto.o   + .d
        │   ├── uiox_hdmi_subsys.o  + .d
        │   ├── uiox_hdmi_device.o  + .d
        │   └── uiox_hdmi_demo.o    + .d
        ├── lib/
        │   └── libuiox_hdmi.a
        └── bin/
            └── uiox_hdmi_demo
//////////////////
Makefile Usage Reference
Command	Effect
make	Native debug build (auto-detects platform)
make BUILD=release	Optimised, stripped release build
make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-	Cross-compile for ARM64
make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf-	Cross-compile for ARM32
make lib	Build static library only
make demo	Build demo binary (depends on lib)
make install PREFIX=/opt/uiox	Install headers + lib + binary
make uninstall PREFIX=/opt/uiox	Remove installed files
make size	Print text/data/bss breakdown
make dump	Generate disassembly → .asm file
make lint	Run cppcheck static analysis
make format	Auto-format all sources with clang-format
make docs	Generate Doxygen HTML documentation
make tags	Generate ctags index
make info	Print full build configuration summary
make clean	Remove current platform/mode build artefacts
make distclean	Remove entire build/ tree
make V=1	Verbose — print full compiler commands
/////////////
Key Design Decisions
Decision	Rationale
Triple framebuffer pool	Eliminates tearing at 4K@60; render buffer stays with CPU while front/pending buffers are owned by display controller
Zero-copy page flip	DMA scanout address switched at VBlank; no pixel memcpy on the display path — essential at 594 MHz pixel clock
TMDS vs FRL auto-select	pixel_clk_khz > 600 MHz automatically selects FRL (HDMI 2.1); stays on TMDS below — transparent to application
HDMI 2.0 scrambling	LFSR scrambling enabled automatically when pixel clock exceeds 340 MHz per HDMI 2.0 specification
N/CTS ACR table	Standard CEA-861 N values pre-tabulated per sample rate; CTS computed from pixel clock — correct on all sinks
CEA-861 extension parser	Handles audio (SAD), video (VIC), speaker allocation, HDR static metadata, Forum VSDB (FRL/VRR/ALLM), HDCP tags
Infoframe checksum	Header + payload sum = 0x100 (two's complement); verified against CEA-861-H section 6
AVI infoframe from state	Built dynamically from uiox_hdmi_if_t colorspace at each send — guaranteed consistent with active pixel format
AVMUTE on start/stop	GCP AVMUTE set before timing change, cleared after — prevents audio/video glitch on connected AV receivers
HDCP lifecycle in subsystem	Starts automatically on connect if sink supports it; re-tries on hotplug; stops cleanly on disconnect
DPMS idle timer	PHY powered down after configurable timeout; activity signal immediately restores — standard power management
Hotplug re-probe	EDID re-read on every connect event; mode re-selected; handles monitor swap during runtime
Periodic infoframe resend	AVI + Audio + HDR re-sent every infoframe_interval_ms (default 1 s) — required by CEA-861-H §6.1
CEC builder	Header byte = `(src_la << 4)
Vtable ops pattern	Synopsys DWC-HDMI, Amlogic HDMITX, Rockchip HDMI, i.MX8 HDMI concrete drivers plug in without modifying upper layers
