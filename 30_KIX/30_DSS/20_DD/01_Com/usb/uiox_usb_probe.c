/**
 * @file    uiox_usb_probe.c
 * @brief   Bind USB to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/usb/uiox_usb_probe.c
 *
 * TWO CLASSES, ONE DIRECTORY — and that is the whole reason this probe
 * differs from eth's, wifi's and bt's.  uiox_devclass.h declares USB
 * twice:
 *
 *     UIOX_DEVCLASS_USBHID = (STREAM, 3)   family 2 — a HID device
 *     UIOX_DEVCLASS_USBHC  = (BUS,    1)   family 6 — a host controller
 *
 * which is why the header says "USB is three classes, not one: mass
 * storage, HID and the host controller are three drivers with three
 * interfaces".  The registry key is the class value, so binding the
 * wrong one means uiox_dev_lookup() silently returns NULL for the right
 * key — the failure would be invisible.
 *
 * The hardware says which: uiox_usb_hw_t carries .role, and the enum has
 * a host arm and a device arm.  This probe reads it and binds the class
 * that matches, rather than assuming either.
 *
 * The pool holds URBs (uiox_usb_urb_t), not payloads — six alloc sites
 * in uiox_usb_class.c and two frees in uiox_usb_if.c.  A different grain
 * from eth's packet pool or Wi-Fi's frames.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_usb_if.h"     /* uiox_usb_if_t, uiox_usb_if_config         */
 #include "uiox_usb_hw.h"     /* uiox_usb_hw_t, uiox_usb_fwbind, role      */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition  */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_USBHID / _USBHC, UIOX_BUS_*  */
 #include "uiox_devops.h"     /* uiox_blk_ops_t */
 #include "uiox_fw_usb.h"     /* uiox_usb_hc_dev_t — the struct body */
 #include "uiox_klibc.h"
 
 static uiox_usb_hw_t   s_usb_hw;
 static uiox_usb_if_t   s_usb_if;
 /* s_usb_hw and s_usb_if are file-static here, so a class driver that is
 * not the probe (usbms now, HID/CDC later) has no route to either.  Same
 * shape as uiox_soc_pcie_get() in uiox_soc_main.c. */
uiox_usb_hw_t *uiox_usb_hw_get(void) { return &s_usb_hw; }
uiox_usb_if_t *uiox_usb_if_get(void) { return &s_usb_if; }

 static uiox_dev_dev_t  s_usb_dev;
 static uiox_i2c_dev_t  s_usb_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_usb_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ─────────────────────────────────────────── */
     rc = uiox_usb_fwbind(&s_usb_hw, &s_usb_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object ─────────────────────────────────────
      * Wires hw into the if-layer: per-endpoint URB queues, the EP0
      * state machine, stats.  Sets .primed. */
     rc = uiox_usb_if_config(&s_usb_if, &s_usb_hw);
     if (rc < 0)
         return rc;
 
         /* ── 3. WHICH CLASS — read from the hardware, not assumed ─────
     * uiox_devclass.h declares USB twice, in two different families:
     *
     *   UIOX_DEVCLASS_USBHID = (STREAM, 3)   family 2 — a HID device
     *   UIOX_DEVCLASS_USBHC  = (BUS,    1)   family 6 — a host ctrlr
     *
     * The registry key IS the class value, so binding the wrong one
     * means uiox_dev_lookup() silently returns NULL for the right key —
     * an invisible failure.  The hardware says which: .role, whose arms
     * are confirmed at uiox_usb_hw.h:69-73.
     *
     * OTG counts as host: UIOX_USB_CAP_OTG means the block can act as
     * one, and ops->set_role() can flip it at runtime.  A controller
     * that can be a host is a USBHC until told otherwise. */
     const bool is_host = (s_usb_hw.role == UIOX_USB_ROLE_HOST ||
                           s_usb_hw.role == UIOX_USB_ROLE_OTG);

     s_usb_dev.cls = is_host ? UIOX_DEVCLASS_USBHC : UIOX_DEVCLASS_USBHID;

 
     /* ── 4. descriptor ────────────────────────────────────────────
      *   .unit  0 — first controller or first HID device
      *   .bus   UIOX_BUS_MMIO — the xHCI register window.  The host
      *          controller is memory-mapped; a HID device behind it would
      *          be reached through that window too, so MMIO holds for
      *          both arms.
      *   .name  "usb0" for the host controller, "usbhid0" for the device.
      *          uiox_dev_parse() takes the prefix plus optional decimal
      *          unit, so both resolve. */
     s_usb_dev.unit     = 0u;
     s_usb_dev.bus      = UIOX_BUS_MMIO;
     s_usb_dev.base     = s_usb_hw.base_addr;
     s_usb_dev.irq      = s_usb_hw.irq;
     s_usb_dev.name     = is_host ? "usb0" : "usbhid0";
     s_usb_dev.ops      = s_usb_hw.ops;
     s_usb_dev.drv_priv = &s_usb_if;
 
     /* ── 5. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_usb_dev);
 }
 