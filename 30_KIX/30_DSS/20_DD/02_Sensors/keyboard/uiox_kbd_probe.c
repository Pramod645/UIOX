/**
 * @file    uiox_kbd_probe.c
 * @brief   Bind the keyboard to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/keyboard/uiox_kbd_probe.c
 *
 * FAMILY 3 — EVENT, index 0.  uiox_devclass.h section 1:
 *
 *     EVENT   records — fixed-size, produced only.  Queued by
 *             00_drvbuff (small).  Producer-only.
 *
 * WHICH MAKES THIS DIFFERENT FROM EVERY OTHER FAMILY
 *
 * There is no ioctl dispatch here and no write path.  A keypress is a
 * record the driver produces and something above consumes; a keyboard has
 * no mode to command and no bytes to accept.  Userspace reaches it with
 * read() alone:
 *
 *     read(fd, &evt, sizeof evt)
 *      → 10_scfs → scfs_getf → inode
 *      → uiox_dev_lookup(UIOX_DEVCLASS_KBD, 0)
 *      → uiox_kbd_event_read()   pops the driver's own ring
 *
 * The ring belongs to the DRIVER (uiox_kbd_if_t.rx), because read() has
 * no ring to hand over — uiox_kbd_if.h states it: scan() pushes into rx
 * when its ring argument is NULL, and uiox_kbd_event_read() pops from it.
 *
 * THE ONE DESCRIPTOR CHOICE
 *
 * uiox_kbd_if_config() takes a TYPE — uiox_kbd_if_type_t — because the
 * keyboard may be a GPIO matrix or a PS/2 device, and the two scan
 * differently (matrix[] versus direct_state).  Which one is board
 * configuration, the same category as .base and .irq; the value below is
 * the default and belongs in a board config when one exists.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_kbd_if.h"     /* uiox_kbd_if_t, uiox_kbd_if_config          */
 #include "uiox_kbd_hw.h"     /* uiox_kbd_hw_t, uiox_kbd_fwbind             */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition   */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_KBD, UIOX_BUS_MMIO           */
 #include "uiox_klibc.h"
 
 /* ── the keyboard type — BOARD CONFIGURATION ─────────────────────────
  * uiox_kbd_if_config() needs it, and it decides which scan runs.  The
  * matrix arms are in uiox_kbd_if_type_t; a PS/2 keyboard is the other
  * family.  Confirm the arm name before compiling — this is the one value
  * this file cannot derive from the hardware. */
 #define UIOX_KBD_PROBE_TYPE   UIOX_KBD_IF_MATRIX
 
 static uiox_kbd_hw_t   s_kbd_hw;
 static uiox_kbd_if_t   s_kbd_if;
 static uiox_dev_dev_t  s_kbd_dev;
 static uiox_i2c_dev_t  s_kbd_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_kbd_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_fw_kbd_init_ps2(&b->fw, hw->irq) then
      * uiox_kbd_hw_init(hw, &s_kbd_fwops).  Note the FwHal call takes an
      * IRQ and no base — the class is either matrix-scanned or PS/2, and
      * both are interrupt-driven. */
     rc = uiox_kbd_fwbind(&s_kbd_hw, &s_kbd_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object, with the type ──────────────────────
      * Wires hw into the if-layer and selects the scan path.  The
      * owned ring (kif->rx) is part of the struct, so read() has
      * somewhere to pop from. */
     rc = uiox_kbd_if_config(&s_kbd_if, &s_kbd_hw, UIOX_KBD_PROBE_TYPE);
     if (rc < 0)
         return rc;
 
     /* ── 3. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_KBD = UIOX_DEVCLASS_(EVENT, 0)
      *   .unit  0
      *   .bus   UIOX_BUS_MMIO — the timer/GPIO controller's window.
      *          uiox_kbd_hw_t carries base_addr ("MMIO base (timer/GPIO
      *          ctrl)") even for the PS/2 arm, since the scan is driven
      *          from that block.
      *   .irq   the key-change line.
      *   .name  "kbd0" — uiox_dev_parse() maps that back to (cls, unit).
      *   .ops   the class's own vtable; uiox_devops.h declares no
      *          family-level one for EVENT, for the same reason it
      *          declares none for STREAM. */
     s_kbd_dev.cls      = UIOX_DEVCLASS_KBD;
     s_kbd_dev.unit     = 0u;
     s_kbd_dev.bus      = UIOX_BUS_MMIO;
     s_kbd_dev.base     = s_kbd_hw.base_addr;
     s_kbd_dev.irq      = s_kbd_hw.irq;
     s_kbd_dev.name     = "kbd0";
     //s_kbd_dev.ops      = s_kbd_hw.ops;
     s_kbd_dev.ops      = &uiox_kbd_class_ops;   /* was: s_kbd_hw.ops */
     s_kbd_dev.drv_priv = &s_kbd_if;
 
     /* ── 4. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_kbd_dev);
 }
 