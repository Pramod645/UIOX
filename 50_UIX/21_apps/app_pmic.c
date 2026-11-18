/*
 * 50_UIX/21_apps/app_pmic.c
 *
 * PMIC application — EL0.  Read-only, matching the layer.
 *
 *   21_apps/app_pmic.c → dev/pmic/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the PMIC driver
 *
 * No 30_KIX header.
 *
 * ── THREE OF FOUR COMMANDS ──────────────────────────────────────────
 *   GET_TELEM 0x01 · GET_VSYS 0x02 · GET_STATS 0x04
 * FIELD_WR 0x03 is not used: it carries {reg, mask, shift, val}, which
 * are kernel register-layout facts.  So this app observes the PMIC and
 * does not command it — no rail control, no OV/UV configuration.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "pmic/uiox_dev_pmic.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* Signed decimal without float: -20 prints as -20, not as 236. */
 static void print_i(const char *label, int v)
 {
     if (v < 0) { uix_printf("%s-%u", label, (unsigned int)(-v)); }
     else       { uix_printf("%s%u",  label, (unsigned int)v); }
 }
 
 int app_pmic_main(void)
 {
     uiox_dev_pmic_t       p;
     uiox_dev_pmic_telem_t t;
     uiox_dev_err_t        rc;
     unsigned int          mv = 0u;
 
     uix_printf("pmic: opening /dev/pmic\n");
 
     rc = uiox_dev_pmic_open(&p, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("pmic: open failed (%d) — no /dev/pmic.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_pmic_probe() run?\n");
         return 1;
     }
 
     /* ── one GET_TELEM carries the supply rails ──────────────────── */
     if (uiox_dev_pmic_get_telemetry(&p, &t) != UIOX_DEV_OK) {
         uix_printf("pmic: GET_TELEM failed\n");
         (void)uiox_dev_pmic_close(&p);
         return 1;
     }
 
     uix_printf("pmic: vsys   %u mV\n", t.vsys_mv);
     uix_printf("pmic: vbat   %u mV\n", t.vbat_mv);
     uix_printf("pmic: vbus   %u mV\n", t.vbus_mv);
     uix_printf("pmic: ibat   %u mA\n", t.ibat_ma);
 
     /* Signed, so a sub-zero die temperature reads as one. */
     print_i("pmic: die    ", t.die_temp_c);
     uix_printf(" C\n");
     print_i("pmic: ntc    ", t.ntc_temp_c);
     uix_printf(" C\n");
 
     /* ── GET_VSYS, showing the single-value command ──────────────── */
     if (uiox_dev_pmic_get_vsys(&p, &mv) == UIOX_DEV_OK)
         uix_printf("pmic: vsys via GET_VSYS 0x02 = %u mV\n", mv);
     else
         uix_printf("pmic: GET_VSYS failed\n");
 
     /* ── stats ───────────────────────────────────────────────────── */
     {
         unsigned char stats[128];
         unsigned int  j;
         for (j = 0u; j < sizeof stats; j++) stats[j] = 0u;
         if (uiox_dev_pmic_get_stats(&p, stats) == UIOX_DEV_OK)
             uix_printf("pmic: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("pmic: GET_STATS failed\n");
     }
 
     uix_printf("pmic: note — no rail commands exist at EL0.\n");
     uix_printf("      rail_on/off/voltage and adc_read are kernel-side.\n");
 
     (void)uiox_dev_pmic_close(&p);
     uix_printf("pmic: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_pmic_main();
 }
 