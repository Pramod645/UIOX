/*
 * 50_UIX/00_libs/00_uixlibs/dev/monitor/uiox_mon_device.h
 *
 * UIOX Monitor top-level application-facing device API — EL0.
 *
 *   21_apps/app_mon.c → dev/monitor/uiox_mon_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_mon_subsys.h" removed — it is EL1.  The two enums it
 *     carried are MIRRORED here, source named beside each.
 *   · uiox_mon_open_params_t lost hw/hw_ops/if_type/dsp/hotplug_cb/
 *     vblank_cb — bring-up, a callback into EL0, and the DSP config.
 *     KEPT: pref_w/pref_h/pref_hz, dpms_timeout_ms, target_fps — the
 *     policy the app chooses, and rows 426/427 carry the last two.
 *   · uiox_mon_device_t holds a uiox_dev_handle_t, not a
 *     uiox_mon_subsys_t.
 *
 * ── WHICH KERNEL ROWS EXIST ─────────────────────────────────────────
 *   425  mon_state()          state()
 *   426  mon_dpms(timeout_ms) open()'s dpms_timeout_ms
 *   427  mon_fps(fps)         open()'s target_fps
 *   428  mon_activity(now_ms) activity()
 *
 * ── CONTROL-ONLY ────────────────────────────────────────────────────
 * Every row is a VALUE, so this layer has no user-address check.
 *
 *   acquire() / present(fb)
 *       the triple buffer is held as fb_display / fb_render /
 *       fb_pending POINTERS inside the subsystem, and the pool lives in
 *       32_FS/00_drvbuff/01_Com/monitor/uiox_mon_buf.h.  A frame must
 *       survive BETWEEN TWO SYSCALLS and then be flipped.  The trap
 *       carries no state across calls.
 *       ⚠ Unblocking it needs a POOL INDEX — the same decision HDMI
 *         carries, confirmed from both pools.
 *
 *   set_backlight / set_dpms / set_gamma / set_brightness / set_contrast
 *   set_colour_mode / osd_add / osd_remove
 *       reach the hardware or the DSP directly (uiox_mon_hw_set_*,
 *       uiox_mon_dsp_*), and none has a row.
 *   tick / set_*_cb / get_resolution / print_*
 *       the kernel's timer, callbacks into EL0, and kernel state.
 */
#ifndef UIOX_MON_DEVICE_H
#define UIOX_MON_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_MON_DEVPATH     "mon"

/* uiox_mon_subsys.h */
typedef enum {
    UIOX_MON_SUBSYS_STOPPED = 0,
    UIOX_MON_SUBSYS_RUNNING,
    UIOX_MON_SUBSYS_DPMS_SAVING,
} uiox_mon_subsys_state_t;

/* uiox_mon_dsp.h / uiox_mon_hw.h — the DPMS levels */
typedef enum {
    UIOX_MON_DPMS_ON = 0,
    UIOX_MON_DPMS_STANDBY,
    UIOX_MON_DPMS_SUSPEND,
    UIOX_MON_DPMS_OFF,
} uiox_mon_dpms_t;

/* ── what an EL0 caller chooses at open ──────────────────────────── */
typedef struct {
    uix_uint16_t  pref_w;
    uix_uint16_t  pref_h;
    uix_uint8_t   pref_hz;
    uix_uint32_t  dpms_timeout_ms;   /**< 0 = disabled    */
    uix_uint32_t  target_fps;        /**< 0 = unlimited   */
} uiox_mon_open_params_t;

/* ── the device handle ─────────────────────────────────────────────── */
typedef struct {
    uiox_dev_handle_t        h;
    uiox_mon_subsys_state_t  last_state;
    uix_bool_t               open;
} uiox_mon_device_t;

/* ── the API ───────────────────────────────────────────────────────── */
uiox_dev_err_t uiox_mon_open    (uiox_mon_device_t *dev,
                                 const uiox_mon_open_params_t *p,
                                 int unit, int flags);
uiox_dev_err_t uiox_mon_close   (uiox_mon_device_t *dev);

uiox_dev_err_t uiox_mon_set_dpms(uiox_mon_device_t *dev, uix_uint32_t timeout_ms);
uiox_dev_err_t uiox_mon_set_fps (uiox_mon_device_t *dev, uix_uint32_t fps);
uiox_dev_err_t uiox_mon_activity(uiox_mon_device_t *dev, uix_uint32_t now_ms);

uiox_mon_subsys_state_t uiox_mon_state(const uiox_mon_device_t *dev);

const char *uiox_mon_state_name(uiox_mon_subsys_state_t s);
const char *uiox_mon_dpms_name (uiox_mon_dpms_t dpms);

#endif /* UIOX_MON_DEVICE_H */
