/*
 * 50_UIX/21_apps/app_chg.c
 *
 * Charger application — EL0, on the far side of the trap.
 *
 *   21_apps/app_chg.c → dev/chg/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the CHG driver
 *
 * Contains no 30_KIX header.
 *
 * ── SIX COMMANDS, FOUR OF THEM WRITES ───────────────────────────────
 *   GET_STATUS 0x01 · SET_ICHG 0x02 · SET_VCHG 0x03
 *   SET_IIN 0x04    · SET_ENABLE 0x05 · GET_STATS 0x06
 * So this app reads the charger and drives it: charge current, charge
 * voltage, input limit, enable.
 *
 * The six raw ADC channels (VBUS/VBAT/IBAT/VSYS/TDIE/NTC) have NO
 * command — uiox_chg_get_adc() is kernel-side only — so nothing here
 * prints them.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "chg/uiox_dev_chg.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* The kernel's enum values, used only for printing.  A name table, not
  * a duplication of behaviour — the driver decides what the number
  * means; this only makes it legible. */
 static const char *chrg_name(int c)
 {
     switch (c) {
     case 0: return "IDLE";
     case 1: return "PRE-CHARGE";
     case 2: return "FAST-CHARGE";
     case 3: return "TAPER";
     case 4: return "DONE";
     case 5: return "FAULT";
     default: return "?";
     }
 }
 
 static const char *src_name(int s)
 {
     switch (s) {
     case 0: return "None";
     case 1: return "USB-C PD";
     case 2: return "USB-C 5V";
     case 3: return "Barrel Jack";
     case 4: return "USB SDP";
     case 5: return "USB CDP";
     case 6: return "USB DCP";
     default: return "?";
     }
 }
 
 static void print_u(const char *label, unsigned int v, const char *unit)
 {
     uix_printf("%s%u %s\n", label, v, unit);
 }
 
 int app_chg_main(void)
 {
     uiox_dev_chg_t        c;
     uiox_dev_chg_status_t st;
     uiox_dev_err_t        rc;
 
     uix_printf("chg: opening /dev/chg\n");
 
     rc = uiox_dev_chg_open(&c, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("chg: open failed (%d) — no /dev/chg.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_chg_probe() run?\n");
         return 1;
     }
 
     /* ── status: one poll-and-read ───────────────────────────────── */
     if (uiox_dev_chg_get_status(&c, &st) != UIOX_DEV_OK) {
         uix_printf("chg: GET_STATUS failed\n");
         (void)uiox_dev_chg_close(&c);
         return 1;
     }
 
     uix_printf("chg: source  %s\n", src_name(st.src));
     uix_printf("chg: charge  %s\n", chrg_name(st.chrg));
     if (st.faults)
         uix_printf("chg: FAULTS  0x%08X\n", st.faults);
     else
         uix_printf("chg: faults  none\n");
 
     /* ── get_stats (separate command) ────────────────────────────── */
     {
         unsigned char stats[128];
         unsigned int  j;
         for (j = 0u; j < sizeof stats; j++) stats[j] = 0u;
         if (uiox_dev_chg_get_stats(&c, stats) == UIOX_DEV_OK)
             uix_printf("chg: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("chg: GET_STATS failed\n");
     }
 
     /* ── drive it: the four writes ──────────────────────────────────
      * These are the commands CHG has and the other three classes do
      * not.  Charge current 2 A, charge voltage 4.2 V, input limit 3 A,
      * then enable.  Values are the kernel's units. */
     if (uiox_dev_chg_set_ichg(&c, 2000u) == UIOX_DEV_OK)
         print_u("chg: ichg set ", 2000u, "mA");
     else
         uix_printf("chg: SET_ICHG rejected\n");
 
     if (uiox_dev_chg_set_vchg(&c, 4200u) == UIOX_DEV_OK)
         print_u("chg: vchg set ", 4200u, "mV");
     else
         uix_printf("chg: SET_VCHG rejected\n");
 
     if (uiox_dev_chg_set_iin(&c, 3000u) == UIOX_DEV_OK)
         print_u("chg: iin set  ", 3000u, "mA");
     else
         uix_printf("chg: SET_IIN rejected\n");
 
     if (uiox_dev_chg_set_enable(&c, 1) == UIOX_DEV_OK)
         uix_printf("chg: enabled\n");
     else
         uix_printf("chg: SET_ENABLE rejected\n");
 
     /* ── re-read, so the effect of the writes is visible ─────────── */
     if (uiox_dev_chg_get_status(&c, &st) == UIOX_DEV_OK)
         uix_printf("chg: after enable — source %s, charge %s\n",
                    src_name(st.src), chrg_name(st.chrg));
 
     (void)uiox_dev_chg_close(&c);
     uix_printf("chg: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_chg_main();
 }
 