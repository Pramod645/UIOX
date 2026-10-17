/**
 * @file    uiox_bt_probe.c
 * @brief   Bind Bluetooth to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/bt/uiox_bt_probe.c
 *
 * uiox_dss_init.c:108 reserves the call.  Shape is eth's and wifi's:
 *
 *     uiox_eth_probe              ↔  uiox_bt_probe
 *     uiox_net_fwbind             ↔  uiox_bt_fwbind
 *     dev->drv_priv = &netif      ↔  dev->drv_priv = &bif
 *     uiox_netif_register/_up     ↔  uiox_bt_if_config
 *
 * FAMILY 2 — STREAM, index 2.  Pooled by 00_drvbuff, unkeyed, because the
 * driver defines the frame's shape (uiox_devclass.h section 1).
 *
 * UART, NOT SDIO AND NOT I2C.  The transport is a UART plus two GPIO
 * pins, so the adapter takes no base address for the transfer:
 *
 *     uiox_fw_bt_init_uart(&b->fw, hw->bt_wake_pin, hw->bt_en_pin)
 *
 * Two arguments, no base.  But uiox_bt_hw_t does carry uart_base — the
 * UART is a memory-mapped peripheral — so .base is NOT zero here, unlike
 * any class whose bus is a pure address line.
 *
 * The i2c handle is unused by the adapter; its guard rejects NULL and
 * dereferences the pointer, so a zeroed static satisfies it honestly.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_bt_if.h"      /* uiox_bt_if_t, uiox_bt_if_config          */
 #include "uiox_bt_hw.h"      /* uiox_bt_hw_t, uiox_bt_fwbind             */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_BT, UIOX_BUS_MMIO,
                                 uiox_dev_dev_t, uiox_dev_bind            */
 #include "uiox_klibc.h"
 
 /* ── statics — uiox_dss_init.c:27: bind() keeps the POINTER ────────── */
 static uiox_bt_hw_t    s_bt_hw;
 static uiox_bt_if_t    s_bt_if;
 static uiox_dev_dev_t  s_bt_dev;
 static uiox_i2c_dev_t  s_bt_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_bt_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_fw_bt_init_uart(&b->fw, hw->bt_wake_pin,
      * hw->bt_en_pin) then uiox_bt_hw_init(hw, &s_bt_fwops), so hw->ops
      * is bound by the time this returns. */
     rc = uiox_bt_fwbind(&s_bt_hw, &s_bt_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object ─────────────────────────────────────
      * Wires hw into the if-layer, which owns the HCI event ring and the
      * transport statistics.  Sets .primed. */
     rc = uiox_bt_if_config(&s_bt_if, &s_bt_hw);
     if (rc < 0)
         return rc;
 
     /* ── 3. bring the transport up ────────────────────────────────
      * if_start is separate from if_config for this class, unlike eth's
      * netif_up and wifi's single config — the HCI transport has to be
      * primed before it can carry commands. */
     rc = uiox_bt_if_start(&s_bt_if);
     if (rc < 0)
         return rc;
 
     /* ── 4. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_BT = UIOX_DEVCLASS_(STREAM, 2)
      *   .unit  0 — first controller
      *   .bus   UIOX_BUS_MMIO — the UART's window.  uiox_devclass.h's
      *          bus enum has no UART arm, and the UART IS memory-mapped
      *          (uart_base is a real field on uiox_bt_hw_t), so MMIO is
      *          the honest answer.
      *   .base  uart_base — NOT zero.  The two GPIO pins travel through
      *          the adapter's init call, but the transport itself is the
      *          UART window.
      *   .irq   the HOST_WAKE line.
      *   .name  "bt0" — uiox_dev_parse() takes the prefix and an optional
      *          decimal unit, so "bt" alone would be unit 0 too. */
     s_bt_dev.cls      = UIOX_DEVCLASS_BT;
     s_bt_dev.unit     = 0u;
     s_bt_dev.bus      = UIOX_BUS_MMIO;
     s_bt_dev.base     = s_bt_hw.uart_base;
     s_bt_dev.irq      = s_bt_hw.irq;
     s_bt_dev.name     = "bt0";
     s_bt_dev.ops      = s_bt_hw.ops;
     s_bt_dev.drv_priv = &s_bt_if;
 
     /* ── 5. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_bt_dev);
 }
 