/*
 * 50_UIX/21_apps/app_rtc.c
 *
 * RTC application — EL0.  Reads the clock, sets an alarm, reads it back.
 *
 *   21_apps/app_rtc.c → dev/rtc/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the RTC driver
 *
 * No 30_KIX header.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "rtc/uiox_dev_rtc.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* A byte prints as a two-digit decimal: 7 -> "07".  No float. */
 static void print2(unsigned char v)
 {
     uix_printf("%u%u", (unsigned int)(v / 10u), (unsigned int)(v % 10u));
 }
 
 int app_rtc_main(void)
 {
     uiox_dev_rtc_t       r;
     uiox_dev_rtc_time_t  t;
     uiox_dev_rtc_alarm_t a;
     uiox_dev_err_t       rc;
 
     uix_printf("rtc: opening /dev/rtc\n");
 
     rc = uiox_dev_rtc_open(&r, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("rtc: open failed (%d) — no /dev/rtc.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_rtc_probe() run?\n");
         return 1;
     }
 
     /* ── read the clock ──────────────────────────────────────────── */
     if (uiox_dev_rtc_get_time(&r, &t) != UIOX_DEV_OK) {
         uix_printf("rtc: GET_TIME failed\n");
         (void)uiox_dev_rtc_close(&r);
         return 1;
     }
 
     uix_printf("rtc: date  ");
     print2(t.yr); uix_printf("-"); print2(t.mo); uix_printf("-"); print2(t.md);
     uix_printf("\n");
     uix_printf("rtc: time  ");
     print2(t.h); uix_printf(":"); print2(t.m); uix_printf(":"); print2(t.s);
     uix_printf("\n");
 
     /* ── read the alarm, then set one two minutes on ───────────────── */
     if (uiox_dev_rtc_get_alarm(&r, &a) == UIOX_DEV_OK)
         uix_printf("rtc: alarm %s\n", a.en ? "ENABLED" : "disabled");
     else
         uix_printf("rtc: GET_ALARM failed\n");
 
     a.s  = 0u;
     a.m  = (unsigned char)((t.m + 2u) % 60u);
     a.h  = t.h;
     a.en = 1u;
 
     if (uiox_dev_rtc_set_alarm(&r, &a) == UIOX_DEV_OK) {
         uix_printf("rtc: alarm set for ");
         print2(a.h); uix_printf(":"); print2(a.m);
         uix_printf("\n");
     } else {
         uix_printf("rtc: SET_ALARM rejected\n");
     }
 
     /* ── read it back, which proves the write landed ─────────────── */
     if (uiox_dev_rtc_get_alarm(&r, &a) == UIOX_DEV_OK)
         uix_printf("rtc: read back — %s at ",
                    a.en ? "ENABLED" : "disabled");
     else
         uix_printf("rtc: read-back failed\n");
     print2(a.h); uix_printf(":"); print2(a.m); uix_printf("\n");
 
     /* ── stats ───────────────────────────────────────────────────── */
     {
         unsigned char stats[128];
         unsigned int  j;
         for (j = 0u; j < sizeof stats; j++) stats[j] = 0u;
         if (uiox_dev_rtc_get_stats(&r, stats) == UIOX_DEV_OK)
             uix_printf("rtc: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("rtc: GET_STATS failed\n");
     }
 
     (void)uiox_dev_rtc_close(&r);
     uix_printf("rtc: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_rtc_main();
 }
 