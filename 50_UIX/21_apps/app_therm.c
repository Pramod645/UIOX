/*
 * 50_UIX/21_apps/app_therm.c
 *
 * Thermal application — EL0, on the far side of the trap.
 *
 * ── THE PATH THIS FILE EXERCISES ────────────────────────────────────
 *   21_apps/app_therm.c            this file                    EL0
 *        ↓  uiox_dev_therm_*()
 *   00_uixlibs/dev/therm/          the device layer             EL0
 *        ↓  uiox_dev_reg_get() / _set()
 *   00_uixlibs/dev/uiox_dev_reg.c  the REG shape                EL0
 *        ↓  uix_ioctl()
 *   40_SCIX/uix_sys.h              my_syscallN → svc            the trap
 *        ↓
 *   32_FS/10_scfs                  the syscall table            kernel
 *        ↓  ioctl dispatch
 *   30_DSS/20_DD/02_Sensors/thermal  the thermal driver         kernel
 *
 * Includes three things, all userspace: the therm device layer, the
 * shared handle/error enum, and uix_printf.  No 30_KIX header.
 *
 * ── THE DRIVER'S WHOLE VOCABULARY IS FOUR COMMANDS ──────────────────
 *   GET_TEMP  0x01   channel 0 ONLY — no channel argument (see below)
 *   GET_TELEM 0x02   the snapshot: num_channels, temp_dc[], alert[]
 *   SET_ALERT 0x03   int16_t ×2
 *   GET_STATS 0x04   uiox_therm_if_stats_t
 *
 * GET_TEMP is not used here.  uiox_therm_if.c:221-227 implements it as
 * *arg = temp_dc[0] — it takes no channel, so a caller asking for
 * channel 3 would get channel 0 with no error.  Everything goes through
 * GET_TELEM instead, which carries the whole picture in one call.
 *
 * ── WHAT IS NOT REACHABLE, AND WHY ──────────────────────────────────
 *   sensor names   no command carries one; names resolve in the
 *                  kernel's sensor manager, which is not exposed
 *   throttled      uiox_therm_subsys.policy state, kernel-side, in
 *   emergency      neither GET_TELEM nor GET_STATS
 * So no call to either appears below.  A function that returned a guess
 * would be worse than its absence.
 *
 * Temperatures are deci-degrees Celsius — 350 is 35.0 C.  Nothing here
 * uses float: the apps build with -nostdinc -ffreestanding, so '%f'
 * would need a soft-float library the image does not carry.
 *
 * v1.1.0  @date 2026-11-18
 */

 #include "therm/uiox_dev_therm.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* ═══════════════════════════════════════════════════════════════════
  * Printing a deci-degree without float
  * ═══════════════════════════════════════════════════════════════════ */
 
 static int abs_short(short v) { return v < 0 ? -(int)v : (int)v; }
 
 static void print_dc(const char *label, short dc)
 {
     unsigned int whole = (unsigned int)(abs_short(dc) / 10);
     unsigned int frac  = (unsigned int)(abs_short(dc) % 10);
     uix_printf("%s%s%u.%u C", label, dc < 0 ? "-" : "", whole, frac);
 }
 
 /* ═══════════════════════════════════════════════════════════════════
  * The demo
  * ═══════════════════════════════════════════════════════════════════ */
 
 int app_therm_main(void)
 {
     uiox_dev_therm_t       t;
     uiox_dev_therm_telem_t snap;
     uiox_dev_err_t         rc;
     unsigned int           ch;
 
     uix_printf("therm: opening /dev/thermal\n");
 
     rc = uiox_dev_therm_open(&t, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("therm: open failed (%d) — no /dev/thermal.\n", (int)rc);
         uix_printf("       Is the node created, and did the probe run?\n");
         return 1;
     }
 
     /* ── one snapshot carries everything ─────────────────────────── */
     if (uiox_dev_therm_snapshot(&t, &snap) != UIOX_DEV_OK) {
         uix_printf("therm: GET_TELEM failed\n");
         (void)uiox_dev_therm_close(&t);
         return 1;
     }
 
     uix_printf("therm: open, %u channel(s)\n", snap.num_channels);
     if (snap.num_channels == 0u) {
         uix_printf("therm: driver reports no channels\n");
         (void)uiox_dev_therm_close(&t);
         return 1;
     }
 
     /* ── per channel, from the snapshot already in hand ──────────── */
     for (ch = 0u; ch < snap.num_channels; ch++) {
         char         label[24];
         unsigned int i = 0u;
         const char  *p = "  ch[";
 
         while (*p && i < 20u) label[i++] = *p++;
         label[i++] = (char)('0' + (ch % 10u));
         label[i++] = ']';
         label[i++] = ' ';
         label[i]   = '\0';
 
         print_dc(label, snap.temp_dc[ch]);
         uix_printf("  alert=%s\n", snap.alert[ch] ? "YES" : "no");
     }
 
     /* ── alert window ────────────────────────────────────────────── */
     /* 80.0 C trip, 75.0 C hysteresis.  The layer refuses hysteresis at
      * or above the trip, because clearing would then never happen. */
     if (uiox_dev_therm_set_alert(&t, 800, 750) == UIOX_DEV_OK)
         uix_printf("therm: alert set  high=80.0 C  hyst=75.0 C\n");
     else
         uix_printf("therm: SET_ALERT rejected\n");
 
     /* ── stats — the driver's own counters, opaque to this file ──── */
     {
         /* uiox_therm_if_stats_t is a kernel type, so the buffer is
          * declared as raw storage here and its address passed through.
          * A caller that wants the fields includes the kernel header. */
         unsigned char stats[128];
         unsigned int  i;
         for (i = 0u; i < sizeof stats; i++) stats[i] = 0u;
         if (uiox_dev_therm_get_stats(&t, stats) == UIOX_DEV_OK)
             uix_printf("therm: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("therm: GET_STATS failed\n");
     }
 
     (void)uiox_dev_therm_close(&t);
     uix_printf("therm: closed\n");
     return 0;
 }
 
 /* The entry point.  21_apps convention: one per app, launched by the
  * shell or the service table, never a main() of its own. */
 int main(void)
 {
     return app_therm_main();
 }
 