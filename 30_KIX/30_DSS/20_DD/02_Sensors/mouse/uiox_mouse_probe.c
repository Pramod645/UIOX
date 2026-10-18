/**
 * @file    uiox_mouse_probe.c
 * @brief   Bind the mouse to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/mouse/uiox_mouse_probe.c
 *
 * FAMILY 3 — EVENT, index 1.  uiox_devclass.h section 1:
 *
 *     EVENT   records — fixed-size, produced only.  Queued by
 *             00_drvbuff (small).  Producer-only.
 *
 * READ-ONLY FROM USERSPACE — no ioctl, no write
 *
 * A mouse move or button change is a record the driver produces and
 * something consumes.  There is no mode to command and no bytes to
 * accept, so userspace reaches it with read() alone:
 *
 *     read(fd, &evt, sizeof evt)
 *      → 10_scfs → scfs_getf → inode
 *      → uiox_dev_lookup(UIOX_DEVCLASS_MOUSE, 0)
 *      → uiox_mouse_event_read()   pops the driver's own ring
 *
 * The ring belongs to the DRIVER (uiox_mouse_if_t.rx) because read() has
 * no ring to hand over — uiox_mouse_if.h states it: poll() pushes into
 * rx when its dst_rb argument is NULL, and uiox_mouse_event_read() pops.
 *
 * SIMPLER THAN THE KEYBOARD, and that is the one real difference: the
 * keyboard's config takes a uiox_kbd_if_type_t (matrix or PS/2, which
 * scan differently), while the mouse has no such split —
 * uiox_mouse_if_config(mif, hw) takes two arguments.  So there is no
 * board-config value to supply here.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_mouse_if.h"   /* uiox_mouse_if_t, uiox_mouse_if_config     */
 #include "uiox_mouse_hw.h"   /* uiox_mouse_hw_t, uiox_mouse_fwbind        */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition  */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_MOUSE, UIOX_BUS_MMIO        */
 #include "uiox_klibc.h"
 
 /* ── statics ──────────────────────────────────────────────────────────
  * uiox_dss_init.c:27 — "uiox_dev_bind() stores the POINTER it is given,
  * not a copy" — so the descriptor must outlive this call.
  * ─────────────────────────────────────────────────────────────────── */
 static uiox_mouse_hw_t  s_mouse_hw;
 static uiox_mouse_if_t  s_mouse_if;
 static uiox_dev_dev_t   s_mouse_dev;
 static uiox_i2c_dev_t   s_mouse_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_mouse_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_fw_mouse_init_ps2(&b->fw, hw->irq) then
      * uiox_mouse_hw_init(hw, &s_mouse_fwops).  An IRQ and no base, as
      * the keyboard's does — both are interrupt-driven. */
     rc = uiox_mouse_fwbind(&s_mouse_hw, &s_mouse_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object ─────────────────────────────────────
      * Wires hw into the if-layer.  The owned ring (mif->rx) is part of
      * the struct, so read() has somewhere to pop from. */
     rc = uiox_mouse_if_config(&s_mouse_if, &s_mouse_hw);
     if (rc < 0)
         return rc;
 
     /* ── 3. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_MOUSE = UIOX_DEVCLASS_(EVENT, 1)
      *   .unit  0
      *   .bus   UIOX_BUS_MMIO — "MMIO base (USB/PS2 ctrl)", so the
      *          descriptor carries a real window even on the PS/2 arm.
      *   .irq   the data-ready line.
      *   .name  "mouse0" — uiox_dev_parse() maps that back to (cls, unit).
      *   .ops   the class's own vtable; uiox_devops.h declares no
      *          family-level one for EVENT. */
     s_mouse_dev.cls      = UIOX_DEVCLASS_MOUSE;
     s_mouse_dev.unit     = 0u;
     s_mouse_dev.bus      = UIOX_BUS_MMIO;
     s_mouse_dev.base     = s_mouse_hw.base_addr;
     s_mouse_dev.irq      = s_mouse_hw.irq;
     s_mouse_dev.name     = "mouse0";
     //s_mouse_dev.ops      = s_mouse_hw.ops;
     //s_mouse_dev.ops        = s_mouse_hw.ops;
         /* The class ops struct (§6 head + family-3 ev pair), NOT the flat
     * HAL vtable — 32_FS reads ev at sizeof(uiox_dev_ops_t). */
     s_mouse_dev.ops = &uiox_mouse_class_ops;

     s_mouse_dev.drv_priv = &s_mouse_if;
 
     /* ── 4. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_mouse_dev);
 }
 