/**
 * @file    uiox_mic_probe.c
 * @brief   Bind the microphone to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/mic/uiox_mic_probe.c
 *
 * FAMILY 2 — STREAM, index 4.  Pooled by 00_drvbuff; the driver defines
 * the frame's shape (uiox_devclass.h section 1).
 *
 * ── THE ONE THING THAT DIFFERS FROM THE OTHER FIVE ──────────────────
 *
 * The adapter binds, but every vtable body returns -ENOSYS
 * (uiox_mic_fwops.c:53–169) because 02_FwHal has NO uiox_fw_mic_init_<bus>
 * entry point — only uiox_fw_mic_init(dev, const uiox_mic_ops_t *ops,
 * uint32_t rate, uint8_t ch, uint8_t bits), which needs an ops table and
 * a format this probe cannot supply.
 *
 * So the device binds into the registry and uiox_dev_lookup() finds it,
 * but a transfer will not reach silicon until that entry point exists.
 * mic is one of only TWO classes in the 22 with no firmware path at all;
 * speaker is the other.  Stated rather than worked around.
 *
 * TWO IRQs, ONE FIELD.  uiox_mic_hw_t carries irq_dma AND irq_overrun;
 * uiox_dev_dev_t carries one .irq, so the descriptor gets irq_dma — the
 * line the DMA path is built around.  The overrun line stays the driver's
 * own business, reached through uiox_mic_if_refill() and the stats.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_mic_if.h"     /* uiox_mic_if_t, _config/_start/_stats_get */
 #include "uiox_mic_hw.h"     /* uiox_mic_hw_t, uiox_mic_audio_fmt_t,
                                 uiox_mic_fwbind                           */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_MIC, UIOX_BUS_MMIO        */
 #include "uiox_klibc.h"
 
 /* ── the capture format — BOARD CONFIGURATION ────────────────────────
  * uiox_mic_if_config() takes it as its third argument, the way camera's
  * takes a capture mode.  Common defaults: 48 kHz, mono, 16-bit signed
  * little-endian.  A board that differs changes them here until a board
  * config exists. */
 static const uiox_mic_audio_fmt_t s_mic_fmt = {
     .sample_rate = 48000u,
     .channels    = 1u,       /* mono capture */
     .bit_depth   = 16u,
     .is_signed   = true,
     .big_endian  = false,
 };
 
 /* ── statics — uiox_dss_init.c:27: bind() keeps the POINTER ────────── */
 static uiox_mic_hw_t   s_mic_hw;
 static uiox_mic_if_t   s_mic_if;
 static uiox_dev_dev_t  s_mic_dev;
 static uiox_i2c_dev_t  s_mic_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_mic_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal.  Binds; every body is -ENOSYS — see header. */
     rc = uiox_mic_fwbind(&s_mic_hw, &s_mic_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object, with the format ────────────────────
      * Wires hw into the if-layer, which owns the capture ring and the
      * double buffer (active_frame / next_frame). */
     rc = uiox_mic_if_config(&s_mic_if, &s_mic_hw, &s_mic_fmt);
     if (rc < 0)
         return rc;
 
     /* ── 3. start the capture path ────────────────────────────────
      * Separate from config, as BT's and camera's are: the DMA ring has
      * to be primed before anything can be captured. */
     rc = uiox_mic_if_start(&s_mic_if);
     if (rc < 0)
         return rc;
 
     /* ── 4. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_MIC = UIOX_DEVCLASS_(STREAM, 4)
      *   .bus   UIOX_BUS_MMIO — the I2S/PCM controller's window
      *   .irq   irq_dma — the DMA-complete line.  irq_overrun is the
      *          driver's own concern; the descriptor has one field.
      *   .name  "mic0" */
     s_mic_dev.cls      = UIOX_DEVCLASS_MIC;
     s_mic_dev.unit     = 0u;
     s_mic_dev.bus      = UIOX_BUS_MMIO;
     s_mic_dev.base     = s_mic_hw.base_addr;
     s_mic_dev.irq      = s_mic_hw.irq_dma;
     s_mic_dev.name     = "mic0";
     s_mic_dev.ops      = s_mic_hw.ops;
     s_mic_dev.drv_priv = &s_mic_if;
 
     /* ── 5. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_mic_dev);
 }
 