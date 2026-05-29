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
