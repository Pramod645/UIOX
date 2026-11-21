/*
 * 34_CSS/01_Com/bt/uiox_kix_css_bt.c
 *
 * BT's four entry points.  Each unpacks its register slots and calls
 * the subsystem API in uiox_bt_subsys.h.  No BT logic lives here.
 *
 * ── no gate in this file ────────────────────────────────────────────
 * All four take values, not pointers: a duration, a handle, nothing.
 * uiox_kix_sccss.h declares no user-address gate, so a family that
 * needs none declares none — and this one does not.
 *
 * ── the instance is unwatched ───────────────────────────────────────
 * s_bt_sys is zeroed until something calls uiox_bt_subsys_init().  The
 * guard below turns that into ENODEV; without it, scan_start would find
 * state == OFF and refuse anyway, but disconnect would reach an empty
 * manager.  Refusing early is the honest answer.
 *
 * @version 1.0.0  @date 2026-11-21
 */

 #include "uiox_kix_css_bt.h"

 #define BT_ENODEV   ((int64_t)-19)
 #define BT_EINVAL   ((int64_t)-22)
 
 int64_t uiox_kix_css_bt_scan_start(uiox_uint64_t dur_s)
 {
     uiox_bt_subsys_t *sys = uiox_kix_css_bt_current();
 
     if (!sys || sys->state == UIOX_BT_SUBSYS_OFF) return BT_ENODEV;
     if (dur_s > 0x30u)                            return BT_EINVAL;  /* 48 * 1.28 s */
 
     return (int64_t)uiox_bt_subsys_scan_start(sys, (uiox_uint8_t)dur_s);
 }
 
 int64_t uiox_kix_css_bt_scan_stop(void)
 {
     uiox_bt_subsys_t *sys = uiox_kix_css_bt_current();
 
     if (!sys || sys->state == UIOX_BT_SUBSYS_OFF) return BT_ENODEV;
 
     return (int64_t)uiox_bt_subsys_scan_stop(sys);
 }
 
 int64_t uiox_kix_css_bt_disconnect(uiox_uint64_t handle)
 {
     uiox_bt_subsys_t *sys = uiox_kix_css_bt_current();
 
     if (!sys || sys->state == UIOX_BT_SUBSYS_OFF) return BT_ENODEV;
 
     return (int64_t)uiox_bt_subsys_disconnect(sys,
                                               (uiox_uint16_t)handle);
 }
 
 int64_t uiox_kix_css_bt_state(void)
 {
     uiox_bt_subsys_t *sys = uiox_kix_css_bt_current();
 
     if (!sys) return BT_ENODEV;
 
     return (int64_t)sys->state;   /* UIOX_BT_SUBSYS_* */
 }
 