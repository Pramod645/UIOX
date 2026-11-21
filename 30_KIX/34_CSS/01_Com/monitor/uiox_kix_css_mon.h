/*
 * 34_CSS/01_Com/monitor/uiox_kix_css_mon.h
 *
 * MONITOR's syscall entry points — numbers 425-428, CSS-PRIVATE.
 *
 * ── FOUR ROWS, AND NONE OF THEM NEEDS A GATE ────────────────────────
 * Every argument and return is a VALUE: a state, a DPMS timeout, a
 * frame rate, a timestamp.  uiox_kix_sccss.h declares no user-address
 * gate because whether one is needed depends on what a family's calls
 * take — and this family's take none.
 *
 * ── WHAT IS NOT HERE, and why ───────────────────────────────────────
 *   acquire() / present(fb)
 *       The triple buffer is held as fb_display / fb_render /
 *       fb_pending POINTERS inside the subsystem
 *       (uiox_mon_subsys.h:46-48), and the pool lives in
 *       32_FS/00_drvbuff/01_Com/monitor/uiox_mon_buf.h.  A frame must
 *       survive BETWEEN TWO SYSCALLS — acquire, draw, present — and
 *       then be flipped on the next VBlank.  The trap carries no state
 *       across calls, so neither can be a row.
 *
 *       ⚠ UNBLOCKING IT NEEDS A POOL INDEX the subsystem resolves.
 *         This is CONFIRMED FOR BOTH DISPLAY CLASSES, not inferred:
 *         HDMI's pool is at .../hdmi/uiox_hdmi_buf.h and MONITOR's at
 *         .../monitor/uiox_mon_buf.h, and both are pointer-addressed.
 *         One index decision unblocks both.
 *
 *   set_hotplug_cb() / set_vblank_cb()
 *       they register FUNCTION POINTERS into EL0 — a callback the
 *       kernel would call later, which is worse than a credential.
 *   tick() / start() / stop()
 *       lifecycle and the kernel's own timer.
 *
 * ── one instance ────────────────────────────────────────────────────
 * Slots are ignored: there is one panel and no array to index.
 *
 * @version 1.0.0  @date 2026-11-21
 */
#ifndef UIOX_KIX_CSS_MON_H
#define UIOX_KIX_CSS_MON_H

#include "uiox_kix_sccss.h"
#include "uiox_mon_subsys.h"      /* uiox_mon_subsys_t, the state enum */

uiox_mon_subsys_t *uiox_mon_subsys_current(void);

int64_t uiox_kix_css_mon_state   (void);
int64_t uiox_kix_css_mon_dpms    (uiox_uint64_t timeout_ms);
int64_t uiox_kix_css_mon_fps     (uiox_uint64_t fps);
int64_t uiox_kix_css_mon_activity(uiox_uint64_t now_ms);

#endif /* UIOX_KIX_CSS_MON_H */
