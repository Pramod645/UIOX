/**
 * @file    uiox_tpwd_probe.c
 * @brief   Bind the touch-password pad to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/touchpwd/uiox_tpwd_probe.c
 *
 * FAMILY 3 — EVENT, index 2.  Same shape as kbd and mouse:
 *
 *     EVENT   records — fixed-size, produced only.  Queued by
 *             00_drvbuff (small).  Producer-only.
 *
 * READ-ONLY FROM USERSPACE — no ioctl, no write
 *
 *     read(fd, &evt, sizeof evt)
 *      → 10_scfs → scfs_getf → inode
 *      → uiox_dev_lookup(UIOX_DEVCLASS_TOUCHPWD, 0)
 *      → uiox_tpwd_event_read()   pops tif->raw_rb
 *
 * THIS CLASS OWNS ITS RING ALREADY — uiox_tpwd_if.h says so: the `rx`
 * field kbd and mouse needed was never added here, because
 * uiox_tpwd_if_t has carried `uiox_tpwd_evtbuf_t raw_rb` since the
 * start and uiox_tpwd_if_config() runs uiox_tpwd_evtbuf_init() on it.
 *
 * I2C, NOT MMIO.  The pad is an addressed slave with two GPIO pins
 * (rst_pin, int_pin), so .bus is UIOX_BUS_I2C and .addr carries the
 * slave address — uiox_devclass.h:145 has the field for exactly this:
 * "addr  I2C slave / SPI CS; 0 for MMIO".
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_tpwd_if.h"    /* uiox_tpwd_if_t, uiox_tpwd_if_config       */
 #include "uiox_tpwd_hw.h"    /* uiox_tpwd_hw_t, uiox_tpwd_fwbind          */
 #include "uiox_fw_i2c.h"     /* uiox_i2c_dev_t — the struct's definition  */
 #include "uiox_devclass.h"   /* UIOX_DEVCLASS_TOUCHPWD, UIOX_BUS_I2C      */
 #include "uiox_klibc.h"
 
 /* ── the three timing values — BOARD CONFIGURATION ───────────────────
  * uiox_tpwd_if_config() takes debounce, hold threshold and lift-off
  * timeout.  None is derivable from the hardware; all three are tuning,
  * the same category as .base and .irq.  These are sensible defaults for
  * a numeric pad and belong in a board config when one exists. */
 #define UIOX_TPWD_PROBE_DEBOUNCE_MS   30u
 #define UIOX_TPWD_PROBE_HOLD_MS      800u    /* long-press threshold      */
 #define UIOX_TPWD_PROBE_TIMEOUT_MS  5000u    /* lift-off after no touch   */
 
 static uiox_tpwd_hw_t  s_tpwd_hw;
 static uiox_tpwd_if_t  s_tpwd_if;
 static uiox_dev_dev_t  s_tpwd_dev;
 static uiox_i2c_dev_t  s_tpwd_i2c;   /* unused; the guard needs non-NULL */
 
 int uiox_tpwd_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_fw_fp_init_fpc1020(&b->fw, i2c, gpio_rst,
      * gpio_int) — four arguments at the FwHal layer, the camera-like
      * shape — then uiox_tpwd_hw_init(hw, &s_tpwd_fwops). */
     rc = uiox_tpwd_fwbind(&s_tpwd_hw, &s_tpwd_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the driver object, with the three timings ─────────────
      * config() also runs uiox_tpwd_evtbuf_init(&tif->raw_rb) — which is
      * why this class needed no `rx` field added, unlike kbd and mouse. */
     rc = uiox_tpwd_if_config(&s_tpwd_if, &s_tpwd_hw,
                              UIOX_TPWD_PROBE_DEBOUNCE_MS,
                              UIOX_TPWD_PROBE_HOLD_MS,
                              UIOX_TPWD_PROBE_TIMEOUT_MS);
     if (rc < 0)
         return rc;
 
         /* ── 3. descriptor ────────────────────────────────────────────
     *   .cls   UIOX_DEVCLASS_TOUCHPWD = UIOX_DEVCLASS_(EVENT, 2)
     *   .unit  0
     *   .bus   UIOX_BUS_I2C — an addressed slave, not a plain window.
     *   .addr  hw.i2c_addr — the 7-bit address of the touch IC.  The
     *          struct carries it separately from i2c_base, which is the
     *          CONTROLLER's window; the same two-value split camera
     *          needed.
     *   .base  hw.i2c_base — the controller's MMIO window.
     *   .irq   the touch-ready line.
     *   .name  "tpwd0" — uiox_dev_parse() maps that to (cls, unit). */
     s_tpwd_dev.cls      = UIOX_DEVCLASS_TOUCHPWD;
     s_tpwd_dev.unit     = 0u;
     s_tpwd_dev.bus      = UIOX_BUS_I2C;
     s_tpwd_dev.addr     = s_tpwd_hw.i2c_addr;
     s_tpwd_dev.base     = s_tpwd_hw.i2c_base;
     s_tpwd_dev.irq      = s_tpwd_hw.irq;
     s_tpwd_dev.name     = "tpwd0";
     //s_tpwd_dev.ops      = s_tpwd_hw.ops;
         /* The class ops struct (§6 head + family-3 ev pair), NOT the flat
     * HAL vtable — 32_FS reads ev at sizeof(uiox_dev_ops_t). */
     s_tpwd_dev.ops      = &uiox_tpwd_class_ops;

     s_tpwd_dev.drv_priv = &s_tpwd_if;
 
     /* ── 4. bind ─────────────────────────────────────────────────── */
     return uiox_dev_bind(&s_tpwd_dev);
 }
 