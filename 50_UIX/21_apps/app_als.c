/*
 * 50_UIX/21_apps/als/app_als.c
 *
 * ALS application — EL0, on the far side of the trap.
 *
 * ── THE PATH THIS FILE EXERCISES ────────────────────────────────────
 *   21_apps/als/app_als.c          this file                    EL0
 *        ↓  uiox_dev_als_*()
 *   00_uixlibs/dev/als/            the device layer             EL0
 *        ↓  uiox_dev_reg_get() / _set()
 *   00_uixlibs/dev/uiox_dev_reg.c  the REG shape                EL0
 *        ↓  uix_ioctl()
 *   40_SCIX/uix_sys.h              my_syscallN → svc            the trap
 *        ↓
 *   32_FS/10_scfs                  the syscall table            kernel
 *        ↓  ioctl dispatch
 *   30_DSS/20_DD/.../als           uiox_als_if.c                kernel
 *
 * It includes three things, all of them userspace:
 *     uiox_dev_als.h   the ALS device layer
 *     uiox_dev.h       the shared handle and error enum
 *     uix_stdio.h      uix_printf, the PoStd print path
 * No 30_KIX header — if that ever stops being true the build fails here,
 * which is where you want to find it.
 *
 * ── WHAT IT ASKS FOR ────────────────────────────────────────────────
 *   raw counts        GET_RAW            driver has it
 *   millilux          converted in dev/  no kernel change
 *   CCT               converted in dev/  approximated, see the header
 *   gain / itime      SET_GAIN/ITIME     driver has it
 *   threshold window  SET_THRESH         driver has it
 *
 * v1.0.0  @date 2026-11-17
 */

 #include "als/uiox_dev_als.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* ═══════════════════════════════════════════════════════════════════
  * The demo
  * ═══════════════════════════════════════════════════════════════════ */
 
 int app_als_main(void)
 {
     uiox_dev_als_t     a;
     uiox_dev_als_raw_t raw;
     unsigned int       lux_milli = 0u;
     unsigned int       cct_k     = 0u;
     unsigned int       gain;
     uiox_dev_err_t     rc;
 
     uix_printf("als: opening /dev/als\n");
 
     /* The IC is named by the caller rather than probed: the conversion
      * table lives in the device layer and needs to know which row to
      * use, and the board knows what is soldered to it. */
     rc = uiox_dev_als_open(&a, 0, UIOX_DEV_ALS_IC_VEML7700, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("als: open failed (%d) — no /dev/als.\n", (int)rc);
         uix_printf("     Is the node created, and did the probe run?\n");
         return 1;
     }
     uix_printf("als: open, IC = VEML7700\n");
 
     /* ── raw counts ──────────────────────────────────────────────── */
     raw.als = raw.white = raw.ir = 0u;
     if (uiox_dev_als_get_raw(&a, &raw) == 0)
         uix_printf("als: raw  als=%u white=%u ir=%u\n",
                    raw.als, raw.white, raw.ir);
     else
         uix_printf("als: GET_RAW failed\n");
 
     /* ── converted values ────────────────────────────────────────── */
     /* These are computed in dev/als, not in the kernel — the counts
      * above are what the driver returned, the numbers below are what
      * the device layer made of them. */
     if (uiox_dev_als_get_lux(&a, &lux_milli) == 0)
         uix_printf("als: lux  %u.%03u\n",
                    lux_milli / 1000u, lux_milli % 1000u);
     else
         uix_printf("als: lux unavailable (no coefficient for this part)\n");
 
     if (uiox_dev_als_get_cct(&a, &cct_k) == 0)
         uix_printf("als: cct  %u K (estimated from the IR fraction)\n", cct_k);
     else
         uix_printf("als: cct unavailable (no signal to ratio)\n");
 
     /* ── control ─────────────────────────────────────────────────── */
     gain = 100u;                                  /* 1x, as x100 */
     if (uiox_dev_als_set_gain(&a, gain) == 0)
         uix_printf("als: gain set to 1x\n");
     else
         uix_printf("als: SET_GAIN rejected\n");
 
     if (uiox_dev_als_set_itime(&a, 100u) == 0)     /* 100 ms */
         uix_printf("als: integration time set to 100ms\n");
     else
         uix_printf("als: SET_ITIME rejected\n");
 
     /* A window: below 10 lux is too dark, above 600 lux is too bright.
      * The layer converts to what the driver expects. */
     if (uiox_dev_als_set_thresh(&a, 10000u, 600000u) == 0)
         uix_printf("als: threshold 10.000 .. 600.000 lux\n");
     else
         uix_printf("als: SET_THRESH rejected (low must be below high)\n");
 
     /* ── a second reading, after the settings changed ────────────── */
     /* Same call, different answer: get_lux applies the current gain and
      * integration time, so this is the check that those two settings
      * actually reached the driver. */
     if (uiox_dev_als_get_lux(&a, &lux_milli) == 0)
         uix_printf("als: lux  %u.%03u  (after settings)\n",
                    lux_milli / 1000u, lux_milli % 1000u);
 
     (void)uiox_dev_als_close(&a);
     uix_printf("als: closed\n");
     return 0;
 }
 
 /* The entry point.  21_apps convention: one per app, launched by the
  * shell or the service table, never a main() of its own. */
 int main(void)
 {
     return app_als_main();
 }
 