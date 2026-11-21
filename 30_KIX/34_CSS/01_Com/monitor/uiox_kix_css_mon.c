/*
 * 34_CSS/01_Com/monitor/uiox_kix_css_mon.c
 *
 * MONITOR's four entry points.  Each unpacks a register slot and calls
 * the subsystem API in uiox_mon_subsys.h.  No monitor logic here.
 *
 * ── no gate in this file ────────────────────────────────────────────
 * All four take values, not pointers.
 *
 * ── the three voids return 0 ────────────────────────────────────────
 * set_dpms_timeout, set_fps and activity all return void, so each row
 * answers 0 itself once the call has been made — the table's fn_t
 * returns int64_t and cannot take their return.
 *
 * @version 1.0.0  @date 2026-11-21
 */

 #include "uiox_kix_css_mon.h"

 #define MON_ENODEV  ((int64_t)-19)
 #define MON_EINVAL  ((int64_t)-22)
 
 int64_t uiox_kix_css_mon_state(void)
 {
     uiox_mon_subsys_t *sys = uiox_mon_subsys_current();
 
     if (!sys) return MON_ENODEV;
 
     return (int64_t)sys->state;   /* UIOX_MON_SUBSYS_* */
 }
 
 int64_t uiox_kix_css_mon_dpms(uiox_uint64_t timeout_ms)
 {
     uiox_mon_subsys_t *sys = uiox_mon_subsys_current();
 
     if (!sys) return MON_ENODEV;
 
     /* 0 = disabled — uiox_mon_subsys.h:64 states that convention, so it
      * is passed through rather than refused */
     if (timeout_ms > (uiox_uint64_t)0xFFFFFFFFull) return MON_EINVAL;
 
     uiox_mon_subsys_set_dpms_timeout(sys, (uiox_uint32_t)timeout_ms);
     return 0;                     /* returns void */
 }
 
 int64_t uiox_kix_css_mon_fps(uiox_uint64_t fps)
 {
     uiox_mon_subsys_t *sys = uiox_mon_subsys_current();
 
     if (!sys) return MON_ENODEV;
 
     /* 0 = unlimited — the same convention, stated at :67 */
     if (fps > (uiox_uint64_t)0xFFFFFFFFull) return MON_EINVAL;
 
     uiox_mon_subsys_set_fps(sys, (uiox_uint32_t)fps);
     return 0;                     /* returns void */
 }
 
 int64_t uiox_kix_css_mon_activity(uiox_uint64_t now_ms)
 {
     uiox_mon_subsys_t *sys = uiox_mon_subsys_current();
 
     if (!sys) return MON_ENODEV;
     if (now_ms > (uiox_uint64_t)0xFFFFFFFFull) return MON_EINVAL;
 
     /* resets the DPMS idle timer, and wakes the panel if it had dimmed */
     uiox_mon_subsys_activity(sys, (uiox_uint32_t)now_ms);
     return 0;                     /* returns void */
 }
 