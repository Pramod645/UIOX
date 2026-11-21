/*
 * 34_CSS/01_Com/hdmi/uiox_kix_css_hdmi.h
 *
 * HDMI's syscall entry points — numbers 421-424, CSS-PRIVATE.
 *
 * ── FOUR ROWS, AND NONE OF THEM NEEDS A GATE ────────────────────────
 * Every argument and every return is a VALUE: a state, a timeout in ms,
 * a timestamp, a mode.  uiox_kix_sccss.h declares no user-address gate
 * because whether one is needed depends on what a family's calls take —
 * and this family's take none.
 *
 * ── hdmi_mode's three values ────────────────────────────────────────
 * Packed into one register:
 *   bits  0-15  width
 *   bits 16-31  height
 *   bits 32-39  refresh Hz
 * Everything above bit 39 is rejected, so a caller cannot smuggle a
 * fourth value through the unused bits.  Three small integers would fit
 * six slots separately; the packed form keeps the row's arity at one.
 *
 * ── WHAT IS NOT HERE, and why ───────────────────────────────────────
 *   acquire() / present(fb)
 *       The triple buffer is held as fb_display / fb_render /
 *       fb_pending POINTERS inside the subsystem
 *       (uiox_hdmi_subsys.h:64-66), and the pool lives in
 *       32_FS/00_drvbuff/01_Com/hdmi/uiox_hdmi_buf.h.  A frame must
 *       survive BETWEEN TWO SYSCALLS — acquire, render, present — and
 *       then be handed to uiox_hdmi_if_flip, which gives it to the
 *       display engine.  The trap carries no state across calls, so
 *       neither can be a row.
 *
 *       ⚠ UNBLOCKING IT NEEDS A POOL INDEX the subsystem resolves — a
 *         small integer the caller holds instead of a pointer, the way
 *         devparse.c names a class or a socket names an fd.  That is a
 *         kernel-side addition to uiox_hdmi_buf.h, not something a
 *         dispatch row can invent.  MONITOR shares the decision, so
 *         settling it once unblocks both display classes.
 *
 *   tick() / start() / stop() / set_evt_cb()
 *       lifecycle and the kernel's own timer.
 *
 * ── one instance ────────────────────────────────────────────────────
 * Slots are ignored: uiox_hdmi_probe() registers one display.
 *
 * @version 1.0.0  @date 2026-11-21
 */
#ifndef UIOX_KIX_CSS_HDMI_H
#define UIOX_KIX_CSS_HDMI_H

#include "uiox_kix_sccss.h"
#include "uiox_hdmi_subsys.h"     /* uiox_hdmi_subsys_t, the state enum */

uiox_hdmi_subsys_t *uiox_hdmi_subsys_current(void);

int64_t uiox_kix_css_hdmi_state   (void);
int64_t uiox_kix_css_hdmi_dpms    (uiox_uint64_t timeout_ms);
int64_t uiox_kix_css_hdmi_activity(uiox_uint64_t now_ms);
int64_t uiox_kix_css_hdmi_mode    (uiox_uint64_t packed);

#endif /* UIOX_KIX_CSS_HDMI_H */
