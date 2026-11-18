/*
 * 50_UIX/21_apps/app_tpwd.c
 *
 * Touch-password application — EL0.
 *
 *   21_apps/app_tpwd.c → dev/tpwd/ → sys_read / uix_poll
 *      → SCFS table → the touch-password driver
 *
 * No 30_KIX header.
 *
 * ── THE MIDDLE SHAPE ────────────────────────────────────────────────
 * Waits with a timeout, reads whatever arrives, then reports.  A panel
 * nobody touches produces nothing, so an unbounded loop would block
 * forever; a bounded N-event loop would end as soon as the panel idled.
 * This waits, reads, and prints what it saw.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   enrol / verify / delete  no command; the security engine and its
 *                            credential pool are kernel-side
 *   set_backlight            no command in this vocabulary
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "tpwd/uiox_dev_tpwd.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 int app_tpwd_main(void)
 {
     uiox_dev_tpwd_t         t;
     uiox_dev_tpwd_raw_evt_t ev;
     uiox_dev_err_t          rc;
     unsigned int            got = 0u, i, p;
     int                     timed_out = 0;
 
     uix_printf("tpwd: opening /dev/tp0\n");
 
     rc = uiox_dev_tpwd_open(&t, 0, UIOX_O_RDONLY);
     if (rc != UIOX_DEV_OK) {
         uix_printf("tpwd: open failed (%d) — no /dev/tp0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_tpwd_probe() run?\n");
         return 1;
     }
 
     /* Read for up to ~2 s of quiet, then report.  Each wait allows 1 s;
      * the first timeout ends the loop. */
     for (i = 0u; i < 16u; i++) {
         rc = uiox_dev_tpwd_wait(&t, 1000);
         if (rc == UIOX_DEV_ENODEV) {
             uix_printf("tpwd: fd not serviceable — stopping\n");
             break;
         }
         if (rc != UIOX_DEV_OK) { timed_out = 1; break; }
 
         rc = uiox_dev_tpwd_read(&t, &ev);
         if (rc != UIOX_DEV_OK) {
             uix_printf("tpwd: short read — the driver never returns 0;"
                        " stopping\n");
             break;
         }
 
         got++;
         uix_printf("tpwd: [%u] points=%u  gesture=%u  ts=",
                    i, ev.num_points, ev.gesture_id);
         {
             unsigned int hi = (unsigned int)(ev.ts_ns >> 32);
             unsigned int lo = (unsigned int)(ev.ts_ns & 0xFFFFFFFFull);
             if (hi) uix_printf("%u%09u", hi, lo); else uix_printf("%u", lo);
         }
         uix_printf(" ns\n");
 
         for (p = 0u; p < ev.num_points && p < UIOX_DEV_TPWD_MAX_TOUCH_POINTS; p++)
             uix_printf("        pt[%u] id=%u %s\n", p, ev.pts[p].id,
                        ev.pts[p].active ? "active" : "lifted");
     }
 
     uix_printf("tpwd: %u event(s) read%s\n", got,
                timed_out ? ", then 1 s of quiet" : "");
     uix_printf("tpwd: state %u (NO command reports it; AUTH_* transitions\n",
                (unsigned int)uiox_dev_tpwd_state(&t));
     uix_printf("      live in the kernel subsystem, above read())\n");
 
     uix_printf("tpwd: NOTE — enrol/verify/delete have no EL0 path.\n");
     uix_printf("      The security engine owns the credential pool.\n");
 
     (void)uiox_dev_tpwd_close(&t);
     uix_printf("tpwd: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_tpwd_main();
 }
 