/*
 * 50_UIX/21_apps/app_bms.c
 *
 * BMS application — EL0, on the far side of the trap.
 *
 *   21_apps/app_bms.c → dev/bms/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the BMS driver
 *
 * Includes only the device layer, the shared handle/error enum, and
 * uix_printf.  No 30_KIX header.
 *
 * ── FOUR COMMANDS, ALL READS ────────────────────────────────────────
 *   GET_TELEM 0x01 · GET_PACK_MV 0x02 · GET_CELLS 0x03 · GET_STATS 0x04
 *
 * This app uses three of the four.  GET_CELLS (0x03) is not used,
 * because uiox_bms_telem_t carries cell_mv[16] and temp_dc[4] inside
 * it — one GET_TELEM answers cells, temps and the pack together, so a
 * second call would be the same data twice.
 *
 * It does not switch FETs: no command does.  uiox_bms_set_chg_fet() and
 * uiox_bms_set_dsg_fet() exist in uiox_bms_device.h but have no ioctl
 * behind them, so they stay kernel-side.
 *
 * v1.1.0  @date 2026-11-18
 */

 #include "bms/uiox_dev_bms.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* Signed decimal, no float: -1200 mA prints as -1200.  The apps build
  * with -nostdinc -ffreestanding, so %f would need a soft-float library
  * the image does not carry. */
 static void print_i(const char *label, int v)
 {
     if (v < 0) { uix_printf("%s-%u", label, (unsigned int)(-v)); }
     else       { uix_printf("%s%u",  label, (unsigned int)v); }
 }
 
 static void print_mv(const char *label, unsigned int mv)
 {
     uix_printf("%s%u mV\n", label, mv);
 }
 
 static void print_mah(const char *label, int mah)
 {
     print_i(label, mah);
     uix_printf(" mAh\n");
 }
 
 int app_bms_main(void)
 {
     uiox_dev_bms_t       b;
     uiox_dev_bms_telem_t s;
     uiox_dev_err_t       rc;
     unsigned int         i;
 
     uix_printf("bms: opening /dev/bms\n");
 
     rc = uiox_dev_bms_open(&b, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("bms: open failed (%d) — no /dev/bms.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_bms_probe() run?\n");
         return 1;
     }
 
     /* ── one snapshot carries the whole pack ─────────────────────────
      * GET_TELEM fills nine fields at once: the pack summary, the fault
      * flags, every cell voltage and every temperature.  Everything this
      * app prints below comes from this one call (plus GET_PACK_MV and
      * GET_STATS at the end, which are separate commands). */
     if (uiox_dev_bms_snapshot(&b, &s) != UIOX_DEV_OK) {
         uix_printf("bms: GET_TELEM failed\n");
         (void)uiox_dev_bms_close(&b);
         return 1;
     }
 
     /* ── the pack summary ────────────────────────────────────────── */
     print_mv ("bms: pack      ", s.pack_mv);
     print_i  ("bms: current   ", s.current_ma);
     uix_printf(" mA\n");
     uix_printf("bms: SoC       %u %%\n", s.soc_pct);
     uix_printf("bms: SoH       %u %%\n", s.soh_pct);
     print_mah("bms: remaining ", s.remain_mah);
     print_mah("bms: full      ", s.full_mah);
 
     /* Derived, not commanded: charging is the sign of the current, and
      * presence is whether the pack reports any voltage at all. */
     uix_printf("bms: charging  %s\n", s.current_ma > 0 ? "YES" : "no");
     uix_printf("bms: present   %s\n", s.pack_mv > 0u ? "YES" : "no");
 
     /* ── faults ──────────────────────────────────────────────────── */
     if (s.fault_flags)
         uix_printf("bms: FAULTS    0x%08X\n", s.fault_flags);
     else
         uix_printf("bms: faults    none\n");
 
     /* ── per-cell, from the same snapshot ────────────────────────────
      * b.num_cells was set by snapshot(): it counts up to the last
      * non-zero cell_mv entry, because a cell reading 0 mV is not a
      * cell.  No GET_CELLS call needed — the array is already here. */
     if (b.num_cells > 0u) {
         uix_printf("bms: %u cell(s)\n", b.num_cells);
         for (i = 0u; i < b.num_cells; i++)
             uix_printf("  cell[%u] = %u mV\n", i, s.cell_mv[i]);
     } else {
         uix_printf("bms: no cells reported\n");
     }
 
     /* ── temperatures, also in the snapshot ──────────────────────────
      * Four slots (UIOX_BMS_MAX_TEMPS).  A zero means the slot is not
      * populated, so it is skipped rather than printed as 0.0 C. */
     for (i = 0u; i < UIOX_DEV_BMS_MAX_TEMPS; i++) {
         if (s.temp_dc[i] == 0) continue;
         uix_printf("  temp[%u] = ", i);
         print_i("", (int)s.temp_dc[i]);
         uix_printf(" dC\n");
     }
 
     /* ── stats — a separate command, kernel type passed through ──────
      * uiox_bms_if_stats_t lives in a 30_KIX header, so the buffer is
      * raw storage and its address is handed over.  A caller that wants
      * the fields includes the kernel header. */
     {
         unsigned char stats[128];
         unsigned int  j;
         for (j = 0u; j < sizeof stats; j++) stats[j] = 0u;
         if (uiox_dev_bms_get_stats(&b, stats) == UIOX_DEV_OK)
             uix_printf("bms: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("bms: GET_STATS failed\n");
     }
 
     /* ── the single-value command ────────────────────────────────────
      * GET_PACK_MV 0x02 exists alongside GET_TELEM.  Calling it here
      * shows the two agree, and that a one-value read costs one trap
      * rather than the whole snapshot. */
     {
         unsigned int mv = 0u;
         if (uiox_dev_bms_pack_mv(&b, &mv) == UIOX_DEV_OK)
             uix_printf("bms: pack via GET_PACK_MV 0x02 = %u mV\n", mv);
         else
             uix_printf("bms: GET_PACK_MV failed\n");
     }
 
     (void)uiox_dev_bms_close(&b);
     uix_printf("bms: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_bms_main();
 }
 