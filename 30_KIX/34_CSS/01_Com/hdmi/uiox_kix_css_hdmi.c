/*
 * 34_CSS/01_Com/hdmi/uiox_kix_css_hdmi.c
 *
 * HDMI's four entry points.  Each unpacks a register slot and calls the
 * subsystem API in uiox_hdmi_subsys.h.  No HDMI logic here.
 *
 * ── no gate in this file ────────────────────────────────────────────
 * All four take values, not pointers.  uiox_kix_sccss.h declares no
 * user-address gate, so a family that needs none declares none.
 *
 * ── the instance is unwatched ───────────────────────────────────────
 * s_hdmi_sys is zeroed until something calls uiox_hdmi_subsys_init(),
 * so the guard turns that into ENODEV rather than faulting.
 *
 * @version 1.0.0  @date 2026-11-21
 */

 #include "uiox_kix_css_hdmi.h"

 #define HDMI_ENODEV  ((int64_t)-19)
 #define HDMI_EINVAL  ((int64_t)-22)
 
 /* ── the mode packing ──────────────────────────────────────────────── */
 #define HDMI_MODE_W(p)   ((uiox_uint64_t)(((p)      ) & 0xFFFFull))
 #define HDMI_MODE_H(p)   ((uiox_uint64_t)(((p) >> 16) & 0xFFFFull))
 #define HDMI_MODE_HZ(p)  ((uiox_uint64_t)(((p) >> 32) & 0xFFull))
 #define HDMI_MODE_HIGH   ((uiox_uint64_t)0x000000FFFFFFFFFFull)  /* bits 0-39 */
 
 int64_t uiox_kix_css_hdmi_state(void)
 {
     uiox_hdmi_subsys_t *sys = uiox_hdmi_subsys_current();
 
     if (!sys) return HDMI_ENODEV;
 
     return (int64_t)sys->state;   /* UIOX_HDMI_SUBSYS_* */
 }
 
 int64_t uiox_kix_css_hdmi_dpms(uiox_uint64_t timeout_ms)
 {
     uiox_hdmi_subsys_t *sys = uiox_hdmi_subsys_current();
 
     if (!sys) return HDMI_ENODEV;
 
     /* 0 means "never blank" — uiox_hdmi_subsys_set_dpms's own
      * convention, passed through rather than refused */
     if (timeout_ms > (uiox_uint64_t)0xFFFFFFFFull) return HDMI_EINVAL;
 
     uiox_hdmi_subsys_set_dpms(sys, (uiox_uint32_t)timeout_ms);
     return 0;                     /* set_dpms returns void */
 }
 
 int64_t uiox_kix_css_hdmi_activity(uiox_uint64_t now_ms)
 {
     uiox_hdmi_subsys_t *sys = uiox_hdmi_subsys_current();
 
     if (!sys) return HDMI_ENODEV;
     if (now_ms > (uiox_uint64_t)0xFFFFFFFFull) return HDMI_EINVAL;
 
     /* wakes the subsystem if DPMS had suspended it */
     uiox_hdmi_subsys_activity(sys, (uiox_uint32_t)now_ms);
     return 0;                     /* activity returns void */
 }
 
 int64_t uiox_kix_css_hdmi_mode(uiox_uint64_t packed)
 {
     uiox_hdmi_subsys_t *sys = uiox_hdmi_subsys_current();
     uiox_uint64_t       w, h, hz;
 
     if (!sys)                     return HDMI_ENODEV;
     if (packed & ~HDMI_MODE_HIGH) return HDMI_EINVAL;   /* bits above 39 */
 
     w  = HDMI_MODE_W(packed);
     h  = HDMI_MODE_H(packed);
     hz = HDMI_MODE_HZ(packed);
 
     /* a mode with no dimensions is not a mode */
     if (!w || !h || !hz)          return HDMI_EINVAL;
 
     return (int64_t)uiox_hdmi_subsys_set_mode(sys,
                                               (uiox_uint16_t)w,
                                               (uiox_uint16_t)h,
                                               (uiox_uint8_t)hz);
 }
 