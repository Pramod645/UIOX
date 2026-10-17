/**
 * @file    uiox_wifi_probe.c
 * @brief   Bind Wi-Fi to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/WiFi/uiox_wifi_probe.c
 *
 * uiox_dss_init.c:106 reserves the call.  The shape is eth's, transposed,
 * which is the shape emmc's set:
 *
 *     uiox_eth_probe            ↔  uiox_wifi_probe
 *     uiox_net_fwbind           ↔  uiox_wifi_fwbind
 *     dev->drv_priv = &netif    ↔  dev->drv_priv = &wif
 *     uiox_netif_register/_up   ↔  uiox_wifi_if_config
 *
 * FAMILY 2 — STREAM, index 1.  Its pool is 00_drvbuff's, unkeyed, because
 * the driver defines the frame's shape: uiox_devclass.h section 1 has
 * STREAM as "bytes/frames; pooled by 00_drvbuff".  The same rule as eth,
 * not the cache that serves emmc.
 *
 * SDIO, NOT I2C.  The attach is the SD host window.  The adapter's entry
 * point is the _sdio one — uiox_fw_wifi_init_sdio(dev, base, irq)
 * supplies its own wifi_sdio_ops, which was the fix that opened this
 * session — and the i2c handle is unused.  But its guard rejects NULL and
 * dereferences the pointer, so a zeroed static satisfies it honestly: no
 * bus is claimed, none is used.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_wifi_if.h"    /* uiox_wifi_if_t, uiox_wifi_if_config      */
 #include "uiox_wifi_hw.h"    /* uiox_wifi_hw_t, uiox_wifi_fwbind         */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_WIFI, UIOX_BUS_MMIO,
                                 uiox_dev_dev_t, uiox_dev_bind            */
 #include "uiox_klibc.h"
 
 /* ── statics ──────────────────────────────────────────────────────────
  * uiox_dss_init.c:27 is explicit — "uiox_dev_bind() stores the POINTER it
  * is given, not a copy" — so the descriptor must outlive this call.  hw
  * and if are static for the same reason: the descriptor points at both.
  * ─────────────────────────────────────────────────────────────────── */
 static uiox_wifi_hw_t  s_wifi_hw;
 static uiox_wifi_if_t  s_wifi_if;
 static uiox_dev_dev_t  s_wifi_dev;
 static uiox_i2c_dev_t  s_wifi_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_wifi_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_fw_wifi_init_sdio(&b->fw, hw->base_addr,
      * hw->irq), then uiox_wifi_hw_init(hw, &s_wifi_fwops) — so hw->ops
      * is bound by the time this returns. */
     rc = uiox_wifi_fwbind(&s_wifi_hw, &s_wifi_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object ─────────────────────────────────────
      * Wires hw into the if-layer, which owns the four WMM TX queues
      * (VO/VI/BE/BK, depth 16), the RX demux and rate control.  Sets
      * .primed. */
     rc = uiox_wifi_if_config(&s_wifi_if, &s_wifi_hw);
     if (rc < 0)
         return rc;
 
     /* ── 3. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_WIFI = UIOX_DEVCLASS_(STREAM, 1)
      *   .unit  0 — first instance; a second radio would be unit 1
      *   .bus   UIOX_BUS_MMIO — the SD host controller's window.
      *          uiox_devclass.h's bus enum has no SDIO arm, and the SD
      *          host IS memory-mapped, so MMIO is the honest answer
      *          rather than adding an enum entry for one class.
      *   .name  "wlan0" — uiox_netif.h:26 uses that exact spelling in its
      *          own example, which is what uiox_dev_parse() maps back to
      *          (cls, unit).
      *   .ops   the class's own vtable.  uiox_devops.h is explicit that
      *          family 2 has NO family-level one: "a NIC's ring and a
      *          camera's frame queue have nothing in common but the word
      *          'buffer'." */
     s_wifi_dev.cls      = UIOX_DEVCLASS_WIFI;
     s_wifi_dev.unit     = 0u;
     s_wifi_dev.bus      = UIOX_BUS_MMIO;
     s_wifi_dev.base     = s_wifi_hw.base_addr;
     s_wifi_dev.irq      = s_wifi_hw.irq;
     s_wifi_dev.name     = "wlan0";
     s_wifi_dev.ops      = s_wifi_hw.ops;
     s_wifi_dev.drv_priv = &s_wifi_if;
 
     /* ── 4. bind ──────────────────────────────────────────────────
      * Stores the pointer.  Returns -EEXIST if (cls, unit) is already
      * bound, -ENOSPC past UIOX_DEV_MAX (32). */
     return uiox_dev_bind(&s_wifi_dev);
 }
 