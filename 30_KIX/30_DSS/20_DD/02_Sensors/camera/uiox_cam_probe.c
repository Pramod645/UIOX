/**
 * @file    uiox_cam_probe.c
 * @brief   Bind the camera to the device registry.
 *
 * PATH    30_KIX/30_DSS/20_DD/02_Sensors/camera/uiox_cam_probe.c
 *
 * uiox_dss_init.c:126 reserves the call.  FAMILY 2 — STREAM, index 6,
 * and the exception the family's own comment names:
 *
 *     "CAMERA is the exception among these: it produces FRAMES, so its
 *      pool is sized in frames and has no headroom."
 *
 * Two things follow, and both differ from the other five STREAM classes:
 *
 *   1. The pool is sized from a CAPTURE MODE, not at compile time.
 *      uiox_cam_buf_init(width, height, stride, fmt) takes four
 *      arguments — every other class's _buf_init() takes none.  So the
 *      probe must know the mode before the pool can exist, and that mode
 *      is board configuration, not a constant this file should invent.
 *
 *   2. The capture ring is PRIMED with empty frames before it can
 *      produce anything: uiox_cam_if_prime(cif, count) submits N frames
 *      to the controller, and uiox_cam_if_complete() returns each one as
 *      it fills.  Same shape as USB's RX priming in cdc_bind().
 *
 * I2C, NOT MMIO.  The sensor is an addressed slave with two GPIO pins
 * (reset, power-down), so .bus is UIOX_BUS_I2C and .addr carries the
 * slave address — uiox_devclass.h:145 has the field for exactly this:
 * "addr  I2C slave / SPI CS; 0 for MMIO".  First class in family 2 where
 * MMIO is not the answer.
 *
 * @version 1.0.0  @date 2026-10-17
 */

 #include "uiox_cam_if.h"      /* uiox_cam_if_t, _config, _prime           */
 #include "uiox_cam_hw.h"      /* uiox_cam_hw_t, uiox_cam_fwbind           */
 #include "uiox_cam_buf.h"     /* uiox_cam_buf_init, UIOX_CAM_POOL_FRAMES  */
 #include "uiox_fw_i2c.h"      /* uiox_i2c_dev_t — the struct's definition */
 #include "uiox_devclass.h"    /* UIOX_DEVCLASS_CAMERA, UIOX_BUS_I2C       */
 #include "uiox_klibc.h"

 /* ── the capture mode and CSI-2 link — BOARD CONFIGURATION ───────────
 * uiox_cam_if_config() needs all six, and none belongs in this file.
 * Two may be readable from the hardware descriptor rather than set here:
 *   lanes  — uiox_cam_hw_t.max_lanes ("e.g., 2 or 4 for CSI-2")
 *   w/h    — a board-named default mode
 * The rest (vc, dt, fmt, stride) come from the board config alongside
 * .base and .irq, which are equally unwired today.
 *
 * These values match the pool header's own comment — "Enough for 1080p
 * YUV422" — which is the only mode the tree names. */
 #define UIOX_CAM_PROBE_LANES   2u
 #define UIOX_CAM_PROBE_VC      0u
#define UIOX_CAM_PROBE_DT      UIOX_CSI_DT_YUV422
#define UIOX_CAM_PROBE_FMT     UIOX_CAM_PIX_FMT_YUV422
 #define UIOX_CAM_PROBE_W       1920u
 #define UIOX_CAM_PROBE_H       1080u
 #define UIOX_CAM_PROBE_STRIDE  (1920u * 2u)   /* YUV422: 2 bytes/px */

 
 static uiox_cam_hw_t   s_cam_hw;
 static uiox_cam_if_t   s_cam_if;
 static uiox_dev_dev_t  s_cam_dev;
 static uiox_i2c_dev_t  s_cam_i2c;
 
 /* ── the capture mode ────────────────────────────────────────────────
  * PLACEHOLDER.  uiox_cam_buf_init() needs four values, and none of them
  * belongs in this file — a capture mode is board configuration, the same
  * way .base and .irq are.  These match the pool header's own comment
  * ("Enough for 1080p YUV422"), which is the only mode the tree names.
  *
  * When a board config exists, these four come from it along with
  * s_cam_hw.  Until then the pool is sized for a mode nothing may ask
  * for, which is honest but not yet wired.
  * ─────────────────────────────────────────────────────────────────── */
 
 int uiox_cam_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * The adapter runs uiox_cam_init_ov5640(&b->fw, i2c, gpio_reset,
      * gpio_pwdn) — four arguments at the FwHal layer — then
      * uiox_cam_hw_init(hw, &s_cam_fwops).  The probe's call is the
      * uniform (hw, i2c) form, as every class's is. */
     rc = uiox_cam_fwbind(&s_cam_hw, &s_cam_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. the capture mode AND the CSI-2 link, in one call ──────
     * uiox_cam_if_config() takes nine arguments: hw, then the link
     * (lanes, virtual channel, data type) and the mode (fmt, w, h,
     * stride).  It configures the CSI-2 receiver and sizes the frame
     * pool from the same values — which is why there is no separate
     * uiox_cam_buf_init() call here, unlike every other class's pool.
     *
     * Every value below is BOARD CONFIGURATION, the same category as
     * .base and .irq.  None of them can be invented in this file. */
     rc = uiox_cam_if_config(&s_cam_if, &s_cam_hw,
                             UIOX_CAM_PROBE_LANES,      /* lanes  */
                             UIOX_CAM_PROBE_VC,         /* vc     */
                             UIOX_CAM_PROBE_DT,         /* dt     */
                             UIOX_CAM_PROBE_FMT,        /* fmt    */
                             UIOX_CAM_PROBE_W,          /* w      */
                             UIOX_CAM_PROBE_H,          /* h      */
                             UIOX_CAM_PROBE_STRIDE);    /* stride */
     if (rc < 0)
         return rc;
 
     /* ── 3. PRIME the capture ring ────────────────────────────────
     * Unchanged: empty frames queued before the controller produces
     * anything.  The pool now exists, because config() sized it. */
     rc = uiox_cam_if_prime(&s_cam_if, UIOX_CAM_POOL_FRAMES);
     if (rc < 0)
         return rc;
 
     /* ── 4. PRIME the capture ring ────────────────────────────────
      * The step no other class in family 2 has.  A capture pipeline
      * needs empty frames queued before it produces anything — the
      * controller fills them and uiox_cam_if_complete() returns each as
      * it lands.  A board with only 8 pool frames (UIOX_CAM_POOL_FRAMES)
      * can prime at most that many. */
     rc = uiox_cam_if_prime(&s_cam_if, UIOX_CAM_POOL_FRAMES);
     if (rc < 0)
         return rc;
 
     /* ── 5. descriptor ────────────────────────────────────────────
      *   .cls   UIOX_DEVCLASS_CAMERA = UIOX_DEVCLASS_(STREAM, 6)
      *   .unit  0 — first sensor
      *   .bus   UIOX_BUS_I2C — the sensor is an addressed slave.  First
      *          class in family 2 that is not MMIO.
      *   .addr  the I2C slave address — the field exists for this
      *   .base  0 — no MMIO window; this is what the addr field is for
      *   .irq   the sensor's interrupt line
      *   .name  "cam0" */
     s_cam_dev.cls      = UIOX_DEVCLASS_CAMERA;
     s_cam_dev.unit     = 0u;
     s_cam_dev.bus      = UIOX_BUS_I2C;
     s_cam_dev.addr     = 0u;      /* ← from board config, like .base */
     s_cam_dev.base     = 0u;      /* I2C-attached: no MMIO window    */
     s_cam_dev.irq      = s_cam_hw.irq;
     s_cam_dev.name     = "cam0";
     s_cam_dev.ops      = s_cam_hw.ops;
     s_cam_dev.drv_priv = &s_cam_if;
 
     return uiox_dev_bind(&s_cam_dev);
 }
 