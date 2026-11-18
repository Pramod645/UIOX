/*
 * 50_UIX/00_libs/00_uixlibs/dev/kbd/uiox_dev_kbd.h
 *
 * KBD — the application's view of the keyboard, at EL0.
 *
 * ── FAMILY 3 (EVENT) ────────────────────────────────────────────────
 * The first EVENT class in this layer, and the first device that is
 * READ rather than ioctl'd.  uiox_kbd_if.h:122 says why:
 *
 *     typedef struct {
 *         uiox_dev_ops_t     head;  // name, probe, init, deinit, ioctl
 *         uiox_event_ops_t   ev;    // init, deinit, pending, read, isr
 *     } uiox_kbd_ops_t;
 *
 * The driver publishes an event source, not a command table — there is
 * no UIOX_KBD_IOC_* anywhere in the keyboard directory.  So the shape
 * here is the EVENT shape: open, wait, read, close.
 *
 * ── WHAT IS DELIBERATELY ABSENT ─────────────────────────────────────
 *   set_leds / set_backlight   no ioctl exists for either.  The header
 *                              notes ioctl carries commands "the other
 *                              way", and this driver offers none.
 *   add_shortcut / add_callback  both take function pointers, stored in
 *                              the kernel's registry and called on the
 *                              next matching event.  A pointer into an
 *                              app cannot be called in kernel context.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_KBD_H
#define UIOX_DEV_KBD_H

#include "../uiox_dev_event.h"     /* the EVENT shape */
#include "../sys/uix_types.h"
#include "../PoStd/uix_poll.h"

#define UIOX_DEV_KBD_DEVPATH  "kbd"

/* The kernel's uiox_kbd_event_t, mirrored from
 * 30_KIX/32_FS/00_drvbuff/02_Sensors/keyboard/uiox_kbd_buf.h:41-49.
 * Eight fields, and the widths are NOT uniform: four single bytes, then
 * keycode, unicode, modifiers, ts_ns.  A word-sized copy would shift
 * every field after `scancode`.
 *
 * The compiler inserts padding before keycode (u16 after four u8) and
 * before ts_ns (u64 at the end) — so this must be the same declaration
 * in the same order, not a hand-packed equivalent. */
typedef struct {
    unsigned char      ev_type;    /* UIOX_KBD_EV_* */
    unsigned char      row;
    unsigned char      col;
    unsigned char      scancode;
    unsigned short     keycode;    /* HID usage ID */
    unsigned int       unicode;    /* 0 = none     */
    unsigned char      modifiers;
    unsigned long long ts_ns;
} uiox_dev_kbd_event_t;

/* The kernel's event types, for printing only — the driver decides what
 * the number means; this makes it legible. */
#define UIOX_DEV_KBD_EV_PRESS    0
#define UIOX_DEV_KBD_EV_RELEASE  1
#define UIOX_DEV_KBD_EV_REPEAT   2
#define UIOX_DEV_KBD_EV_SPECIAL  3

typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_kbd_t;

uiox_dev_err_t uiox_dev_kbd_open (uiox_dev_kbd_t *k, int unit, int flags);
uiox_dev_err_t uiox_dev_kbd_close(uiox_dev_kbd_t *k);

/* Blocks until an event is ready.  Backed by the driver's ev.pending. */
uiox_dev_err_t uiox_dev_kbd_wait (uiox_dev_kbd_t *k, int timeout_ms);

/* Pops ONE event.  Returns UIOX_DEV_OK with *out filled, or
 * UIOX_DEV_ENODEV when nothing is ready — the read path is
 * uiox_kbd_event_read(), which pops the kernel's rx ring. */
uiox_dev_err_t uiox_dev_kbd_read (uiox_dev_kbd_t *k,
                                  uiox_dev_kbd_event_t *out);

#endif /* UIOX_DEV_KBD_H */
