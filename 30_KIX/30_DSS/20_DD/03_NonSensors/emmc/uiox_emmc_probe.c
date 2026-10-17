/**
 * @file    uiox_emmc_probe.c
 * @brief   Bind the eMMC to the device registry — the node the whole
 *          family-1 path hangs from.
 *
 * PATH    30_KIX/30_DSS/20_DD/03_NonSensors/emmc/uiox_emmc_probe.c
 *
 * uiox_dss_init.c:64 declares `int uiox_emmc_probe(void);` and :93 calls
 * it as the only family-1 entry point.  Nothing defined it, so
 * uiox_dev_lookup() found no device and every read returned zeroed bytes
 * through bcache's silent-return path.
 *
 * ORDER — read off the two .c files, not invented
 *
 *   1. uiox_emmc_fwbind(&s_emmc_hw, &s_emmc_i2c)
 *        the 10_DI adapter.  Its body (uiox_emmc_fwops.c:197) guards
 *        `if (!hw || !i2c) return -EINVAL;`, stores `b->i2c = *i2c`,
 *        calls uiox_fw_emmc_init_sdhost(&b->fw, hw->base), then
 *        uiox_emmc_hw_init(hw, &s_emmc_fwops) — which is where hw->ops
 *        is set and ops->init(hw) runs.  After this the HAL is wired.
 *
 *   2. uiox_emmc_if_config(&s_emmc_if, &s_emmc_hw)
 *        memsets eif, .hw = hw, .primed = true, .bus_width = 1,
 *        .speed = IDENT, uiox_emmc_buf_init().  No hardware I/O.
 *
 *   3. uiox_emmc_if_start(&s_emmc_if)
 *        power_on → set_clock(400 kHz) → two interrupt-enable register
 *        writes.  Real I/O; an absent card fails here.
 *
 *   4. fill the static descriptor, then uiox_dev_bind().
 *
 * @version 1.0.0  @date 2026-10-16
 */

 #include "uiox_emmc_hw.h"       /* uiox_emmc_hw_t, uiox_emmc_fwbind       */
 #include "uiox_emmc_if.h"       /* uiox_emmc_if_t, _config, _start        */
 #include "uiox_fw_i2c.h"        /* struct uiox_i2c_dev — DEFINED at :160  */
 #include "uiox_devclass.h"      /* UIOX_DEVCLASS_EMMC, UIOX_BUS_MMIO,
                                    uiox_dev_dev_t, uiox_dev_bind         */
 #include "uiox_devops.h"        /* uiox_blk_ops_t                         */
 #include "uiox_klibc.h"
 
 /* The BLOCK vtable from uiox_emmc_blk.c — non-static there, so extern. */
 extern const uiox_blk_ops_t s_emmc_blk_ops;
 
 /* ── statics ──────────────────────────────────────────────────────────
  * uiox_dss_init.c:27 — "uiox_dev_bind() stores the POINTER it is given,
  * not a copy" — so the descriptor must outlive this call.  hw and if are
  * static for the same reason: the descriptor points at both.
  *
  * s_emmc_i2c is a placeholder that must EXIST, not one that must mean
  * anything: eMMC on this board is MMIO, uiox_devclass.h section 3 says
  * so ("a block device may be MMIO (emmc), PCIe (nvme) or USB"), and the
  * SD host address comes from hw->base.  But fwbind rejects NULL and
  * dereferences the pointer, so a zeroed struct satisfies it honestly —
  * no bus is claimed, none is used.
  * ─────────────────────────────────────────────────────────────────── */
 static uiox_emmc_hw_t   s_emmc_hw;
 static uiox_emmc_if_t   s_emmc_if;
 static uiox_dev_dev_t   s_emmc_dev;
 static uiox_i2c_dev_t   s_emmc_i2c;
 
 int uiox_emmc_probe(void)
 {
     int rc;
 
     /* ── 1. HAL → FwHal ───────────────────────────────────────────
      * Every step named above happens inside this call. */
     rc = uiox_emmc_fwbind(&s_emmc_hw, &s_emmc_i2c);
     if (rc < 0)
         return rc;
 
     /* ── 2. driver → hardware ─────────────────────────────────────
      * Sets .primed WITHOOUT hardware I/O, so a read that arrives
      * between here and step 3 would pass uiox_emmc_blk.c's guard and
      * reach an unclocked controller.  The two calls are adjacent for
      * that reason; nothing may run between them. */
     rc = uiox_emmc_if_config(&s_emmc_if, &s_emmc_hw);
     if (rc < 0)
         return rc;
 
     /* ── 3. the init sequence ─────────────────────────────────────
      * power_on, ident clock, interrupt enables.  An absent card fails
      * here — and uiox_dss_init.c:84 accepts that: "A class that fails
      * to probe is reported by its own return value and skipped". */
     rc = uiox_emmc_if_start(&s_emmc_if);
     if (rc < 0)
         return rc;

     /* ── 3b. learn the card's geometry ────────────────────────────
     * if_start() brings the controller up but reads no card data, so
     * hw->ident is zeroed and emmc_blk_num_blocks() reports 0 —
     * "unknown", per bcache_internal.h, leaving the fs correct but
     * sizeless. */
    static uint8_t s_ext_csd[UIOX_EMMC_EXT_CSD_LEN];
    rc = uiox_emmc_if_read_ext_csd(&s_emmc_if, s_ext_csd);
    if (rc < 0)
        return rc;

    uiox_emmc_hw_parse_ext_csd(&s_emmc_hw, s_ext_csd);



 
     /* ── 4. descriptor — every field sourced, none invented ───────
      *   .cls      UIOX_DEVCLASS_EMMC = UIOX_DEVCLASS_(BLOCK, 0)
      *   .unit     0 — first instance; a second would be unit 1
      *   .bus      UIOX_BUS_MMIO — uiox_devclass.h section 3
      *   .name     "emmc0" — uiox_devparse() parses that prefix, which
      *             is how mknod resolves a node name back to (cls, unit)
      *   .ops      written once, never reassigned
      *   .drv_priv the driver's own state — the DRIVER owns this slot,
      *             which is what makes uiox_emmc_blk.c's cast safe */
     s_emmc_dev.cls      = UIOX_DEVCLASS_EMMC;
     s_emmc_dev.unit     = 0u;
     s_emmc_dev.bus      = UIOX_BUS_MMIO;
     s_emmc_dev.base     = s_emmc_hw.base;
     s_emmc_dev.irq      = s_emmc_hw.irq;
     s_emmc_dev.name     = "emmc0";
     s_emmc_dev.ops      = &s_emmc_blk_ops;
     s_emmc_dev.drv_priv = &s_emmc_if;
 
     /* ── 5. bind ──────────────────────────────────────────────────
      * Stores the pointer.  0, or -EEXIST if (cls, unit) is already
      * bound, -ENOSPC past UIOX_DEV_MAX (32). */
     return uiox_dev_bind(&s_emmc_dev);
 }
 