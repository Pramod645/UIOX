/*
 * 50_UIX/00_libs/00_uixlibs/dev/mic/uiox_mic_device.h
 *
 * UIOX Microphone top-level application-facing device API — EL0.
 *
 *   21_apps/app_mic.c → dev/mic/uiox_mic_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_mic_subsys.h" removed — it is EL1.  The enums and types it
 *     carried are MIRRORED here, source named beside each.
 *   · uiox_mic_open_params_t lost hw/hw_ops/codec/fmt/dsp — those are
 *     the kernel's bring-up arguments, and an EL0 caller has no HAL.
 *   · uiox_mic_device_t holds a uiox_dev_handle_t, not a
 *     uiox_mic_subsys_t.
 *
 * ── WHICH KERNEL ROWS EXIST ─────────────────────────────────────────
 *   409  mic_read(buf, n_samples)   read()      ← the gate is here
 *   410  mic_gain(gain_db)          set_gain()
 *   411  mic_mute(on)               set_mute()
 *   412  mic_state()                state()
 *
 * ── read() is the one that COPIES OUT ───────────────────────────────
 * uiox_mic_subsys_read fills the caller's buffer and returns a SAMPLE
 * count.  This layer passes a byte count to uiox_dev_stream_read, so
 * n_samples is multiplied by sizeof(int16_t) — the arithmetic the
 * kernel's row 409 performs on the other side, in the same unit.
 */
#ifndef UIOX_MIC_DEVICE_H
#define UIOX_MIC_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_MIC_DEVPATH          "mic"
#define UIOX_MIC_FRAME_SAMPLES    256
#define UIOX_MIC_MAX_READ_SAMPLES 65536u

/* uiox_mic_subsys.h */
typedef enum {
    UIOX_MIC_STATE_STOPPED = 0,
    UIOX_MIC_STATE_RUNNING,
    UIOX_MIC_STATE_MUTED,
    UIOX_MIC_STATE_ERROR,
} uiox_mic_state_t;

/* uiox_mic_subsys.h */
typedef enum {
    UIOX_MIC_EVT_STARTED = 0,
    UIOX_MIC_EVT_STOPPED,
    UIOX_MIC_EVT_VOICE_START,
    UIOX_MIC_EVT_VOICE_END,
    UIOX_MIC_EVT_OVERRUN,
    UIOX_MIC_EVT_GAIN_CHANGE,
} uiox_mic_evt_t;

typedef void (*uiox_mic_evt_cb_t)(uiox_mic_evt_t evt, void *ctx);

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL pointer: the kernel owns the codec
 * and /dev/mic0 is how it is reached. */
typedef struct {
    uiox_dev_handle_t  h;
    uiox_mic_state_t   last_state;
    uix_bool_t         open;
} uiox_mic_device_t;

/* ── the API ─────────────────────────────────────────────────────────
 * Four verbs with kernel rows (409-412):
 */
uiox_dev_err_t uiox_mic_open    (uiox_mic_device_t *dev, int unit, int flags);
uiox_dev_err_t uiox_mic_close   (uiox_mic_device_t *dev);

uix_ssize_t    uiox_mic_read    (uiox_mic_device_t *dev,
                                 uix_int16_t *buf, uix_uint32_t n_samples);
uiox_dev_err_t uiox_mic_set_gain(uiox_mic_device_t *dev, uix_uint8_t gain_db);
uiox_dev_err_t uiox_mic_set_mute(uiox_mic_device_t *dev, uix_bool_t mute);

uiox_mic_state_t uiox_mic_state(const uiox_mic_device_t *dev);

/* ── declared, but with NO kernel row yet ────────────────────────────
 *   voice_active()  reads dsp.vad.voice_active — a field in kernel
 *                   memory, and no row returns it.
 *   energy_dbfs()   returns a FLOAT; the trap has no float convention
 *                   and int64_t does not carry it.
 *   tick()          the kernel's own timer drives the subsystem.
 *   print_stats()   reads ring/mif/dsp state the kernel owns.
 *
 * None is declared here, so a missing symbol is a compile error rather
 * than a link error at the end.
 */

const char *uiox_mic_state_name(uiox_mic_state_t s);
const char *uiox_mic_evt_name  (uiox_mic_evt_t evt);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MIC_DEVICE_H */
