/*
 * 50_UIX/21_apps/app_fan.c
 *
 * Fan application — EL0.  Reads the controller and commands one fan.
 *
 *   21_apps/app_fan.c → dev/fan/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the fan driver
 *
 * No 30_KIX header.
 *
 * ── FOUR COMMANDS ───────────────────────────────────────────────────
 *   GET_TELEM 0x01 · GET_RPM 0x02 · SET_PWM 0x03 · GET_STATS 0x04
 * SET_PWM is the only write, and it takes the timestamp — the driver
 * does not stamp the command itself.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "fan/uiox_dev_fan.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* Signed decimal, no float: -5 prints as -5, not 251. */
 static void print_i(const char *label, int v)
 {
     if (v < 0) { uix_printf("%s-%u", label, (unsigned int)(-v)); }
     else       { uix_printf("%s%u",  label, (unsigned int)v); }
 }
 
 int app_fan_main(void)
 {
     uiox_dev_fan_t       f;
     uiox_dev_fan_telem_t t;
     uiox_dev_err_t       rc;
     unsigned int         i, rpm = 0u;
 
     uix_printf("fan: opening /dev/fan\n");
 
     rc = uiox_dev_fan_open(&f, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("fan: open failed (%d) — no /dev/fan.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_fan_probe() run?\n");
         return 1;
     }
 
     /* ── one GET_TELEM carries every fan and every sensor ─────────── */
     if (uiox_dev_fan_get_telemetry(&f, &t) != UIOX_DEV_OK) {
         uix_printf("fan: GET_TELEM failed\n");
         (void)uiox_dev_fan_close(&f);
         return 1;
     }
 
     uix_printf("fan: %u fan(s) reported\n", f.num_fans);
     for (i = 0u; i < UIOX_DEV_FAN_MAX_CHANNELS; i++) {
         if (i >= f.num_fans) break;
         uix_printf("  fan[%u]  rpm=%u  pwm=%u (%u%%)\n",
                    i, (unsigned int)t.rpm[i], (unsigned int)t.pwm[i],
                    (unsigned int)((unsigned int)t.pwm[i] * 100u)
                    / UIOX_DEV_FAN_PWM_MAX);
     }
 
     uix_printf("fan: temps\n");
     for (i = 0u; i < UIOX_DEV_FAN_MAX_TEMP_SENSORS; i++) {
         if (t.temp_dc[i] == 0) continue;      /* empty slot */
         uix_printf("  temp[%u] = ", i);
         print_i("", t.temp_dc[i]);
         uix_printf(" dC\n");
     }
 
     if (t.fault_flags)
         uix_printf("fan: FAULTS 0x%08X\n", t.fault_flags);
     else
         uix_printf("fan: faults none\n");
 
     /* ── GET_RPM, the single-value command ───────────────────────── */
     if (uiox_dev_fan_get_rpm(&f, &rpm) == UIOX_DEV_OK)
         uix_printf("fan: rpm via GET_RPM 0x02 = %u\n", rpm);
     else
         uix_printf("fan: GET_RPM failed\n");
 
     /* ── SET_PWM — half speed on fan 0 ────────────────────────────── */
     if (uiox_dev_fan_set_pwm(&f, 0u, 128u, 1000u) == UIOX_DEV_OK)
         uix_printf("fan: fan0 set to duty 128/255 (~50%%)\n");
     else
         uix_printf("fan: SET_PWM rejected\n");
 
     /* ── read back, which proves the write landed ────────────────── */
     {
         unsigned int pct = 0u, r2 = 0u;
         if (uiox_dev_fan_get_pct(&f, 0u, &pct) == UIOX_DEV_OK)
             uix_printf("fan: fan0 now %u%%\n", pct);
         if (uiox_dev_fan_get_rpm(&f, &r2) == UIOX_DEV_OK)
             uix_printf("fan: fan0 rpm %u\n", r2);
     }
 
     /* ── stats ───────────────────────────────────────────────────── */
     {
         unsigned char stats[128];
         unsigned int  j;
         for (j = 0u; j < sizeof stats; j++) stats[j] = 0u;
         if (uiox_dev_fan_get_stats(&f, stats) == UIOX_DEV_OK)
             uix_printf("fan: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("fan: GET_STATS failed\n");
     }
 
     (void)uiox_dev_fan_close(&f);
     uix_printf("fan: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_fan_main();
 }
 