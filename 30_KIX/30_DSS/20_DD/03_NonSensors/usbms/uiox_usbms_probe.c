/**
 * @file    uiox_usbms_probe.c
 * @brief   Bind a USB mass-storage device to the registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/usbms/uiox_usbms_probe.c
 *
 * ── why this probe differs from the other three BLOCK classes ────────
 * eMMC, SD and NVMe each own a window the probe reads directly.  A USB
 * mass-storage device owns nothing: it is a child the host controller
 * enumerated, and the tree has no registry entry for it yet.  So this
 * probe reaches the controller through uiox_usb_hw_get() and returns
 * -ENODEV when no mass-storage device is attached — the correct answer
 * for a hot-pluggable class, and one uiox_dss_init.c accepts ("A class
 * that fails to probe is reported by its own return value and skipped").
 */
#include "uiox_usbms_if.h"
#include "uiox_usb_if.h"        /* uiox_usb_if_t, uiox_usb_hw_get   */
#include "uiox_usb_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"        /* uiox_blk_ops_t */
#include "uiox_fw_usb.h"     /* uiox_usb_hc_dev_t — the struct body */
#include "uiox_klibc.h"

// also#include "uiox_usb_hw.h"        /* uiox_usb_hc_dev_t, uiox_usb_hw_fwdev */

extern const uiox_blk_ops_t s_usbms_blk_ops;

/* uiox_dev_bind() stores the POINTER it is given, not a copy, so the
 * descriptor must outlive the call. */
static uiox_usbms_if_t  s_usbms_if;
static uiox_dev_dev_t   s_usbms_dev;

/* USB class code for mass storage: base 0x08, subclass 0x06 (SCSI
 * transparent), protocol 0x50 (Bulk-Only Transport). */
#define UIOX_USB_CLASS_MSC_BOT   0x080650u

int uiox_usbms_probe(void)
{
    /* ── 1. the controller ──────────────────────────────────────────
     * Bound by uiox_usb_probe.c at boot; this only reads it. */
    uiox_usb_hw_t *hw = uiox_usb_hw_get();
    if (!hw) return -ENODEV;

    /* The device must be host-capable and have something attached: a
     * controller in DEVICE role cannot carry a mass-storage child. */
    if (hw->role != UIOX_USB_ROLE_HOST && hw->role != UIOX_USB_ROLE_OTG)
        return -ENODEV;
    if (!hw->connected) return -ENODEV;

        /* ── 2. the device ──────────────────────────────────────────────
     * The FwHal enumerated it; this reads that list rather than walking
     * hardware.  The bind record holds the uiox_usb_hc_dev_t
     * (uiox_usb_fwbind_t.fw, uiox_usb_fwops.c:26) — hw->drv_priv is not
     * that pointer, so uiox_usb_hw_fwdev() is the route.
     *
     * class_code is the one-byte BASE class only.  Mass storage's full
     * identity is three bytes — 0x08 base, 0x06 subclass (SCSI
     * transparent), 0x50 protocol (BOT) — and the record carries just
     * the first, so a UAS or vendor device would also match.  Named
     * here rather than hidden: the descriptor walk narrows it later. */
    uiox_usb_hc_dev_t *hc = uiox_usb_hw_fwdev(hw);
    if (!hc || !hc->num_devices) return -ENODEV;

    uint8_t dev_addr = 0u;
    bool found = false;

    for (uiox_uint32_t i = 0u; i < hc->num_devices; i++) {
        if (!hc->devices[i].connected) continue;
        if (hc->devices[i].class_code != 0x08u) continue;  /* not MSC */
        dev_addr = hc->devices[i].addr;
        found = true;
        break;
    }
    if (!found) return -ENODEV;      /* nothing mass-storage attached */

    uiox_usb_if_t *uif = uiox_usb_if_get();
    if (!uif) return -ENODEV;        /* controller bound but no interface */


    /* ── 3. endpoint pair ─────────────────────────────────────────
     * BOT uses bulk OUT for CBW/data-to-device and bulk IN for
     * data-to-host and CSW.  EP1 is the first bulk pair on a
     * single-interface MSC device. */
    const uint8_t ep_out = 0x01u;              /* EP1 OUT */
    const uint8_t ep_in  = 0x81u;              /* EP1 IN  */

    int rc = uiox_usbms_if_config(&s_usbms_if, uif, ep_in, ep_out);
    if (rc < 0) return rc;

    /* ── 4. init sequence ─────────────────────────────────────────
     * The class driver binds (opens both EPs) and READ CAPACITY(10)
     * runs so geometry is known before the descriptor is filled.  An
     * empty reader or an unresponsive stick fails here. */
    rc = uiox_usbms_if_start(&s_usbms_if);
    if (rc < 0) return rc;

    /* ── 5. descriptor — every field sourced, none invented ───────
     *   .cls      UIOX_DEVCLASS_USBMS = UIOX_DEVCLASS_(BLOCK, 3)
     *   .unit     0 — first instance
     *   .bus      UIOX_BUS_USB — uiox_devclass.h:152 "endpoint-
     *             addressed", which is what a USB device is
     *   .base     0 — a USB device has no register window; the
     *             controller's mmio_base belongs to the host class
     *   .addr     the device address the host assigned (1 after
     *             enumeration); the inode's addr field carries it
     *   .name     "usbms0" — uiox_devparse() parses that prefix (:51)
     *   .ops      the BLOCK vtable; written once, never reassigned
     *   .drv_priv this driver's state, which makes blk.c's cast safe */
    s_usbms_dev.cls      = UIOX_DEVCLASS_USBMS;
    s_usbms_dev.unit     = 0u;
    s_usbms_dev.bus      = UIOX_BUS_USB;
    s_usbms_dev.base     = 0u;
    //s_usbms_dev.addr     = hw->address;
    s_usbms_dev.addr     = dev_addr;     /* the child's, not hw->address */
    s_usbms_dev.irq      = hw->irq;
    s_usbms_dev.name     = "usbms0";
    s_usbms_dev.ops      = &s_usbms_blk_ops;
    s_usbms_dev.drv_priv = &s_usbms_if;

    /* ── 6. bind ──────────────────────────────────────────────────
     * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
     * bound, -ENOSPC past UIOX_DEV_MAX (32). */
    return uiox_dev_bind(&s_usbms_dev);
}
