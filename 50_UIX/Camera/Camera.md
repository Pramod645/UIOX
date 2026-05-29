CSI/parallel/DVP) │ ├── uiox_cam_buf.h # Buffer pool for frame capture │ ├── uiox_cam_if.h # Layer 2 — Interface driver abstraction (CSI lanes, MIPI) │ ├── uiox_cam_sensor.h # Sensor driver abstraction (I2C/SPI reg map) │ ├── uiox_cam_subsys.h # Layer 3 — Pixel formats, pipeline, controls, streaming │ └── uiox_cam_device.h # Layer 4 — Application-facing API (open/config/stream/capture) └── src/ ├── uiox_cam_hw.c ├── uiox_cam_buf.c ├── uiox_cam_if.c ├── uiox_cam_sensor.c ├── uiox_cam_subsys.c └── uiox_cam_device.c

Layer diagram (text)

Application API (uiox_cam_device) → Camera Subsystem: pipeline, controls, streaming (uiox_cam_subsys) → Sensor abstraction: modes, exposure, gain, I2C reg ops (uiox_cam_sensor) → Interface driver: CSI-2/DVP config, lanes, VC, formats (uiox_cam_if) → Hardware Abstraction: MMIO regs, DMA, IRQ, clocks, resets (uiox_cam_hw) ↔ Buffer Manager: frame buffers, queues, zero-copy (uiox_cam_buf)


