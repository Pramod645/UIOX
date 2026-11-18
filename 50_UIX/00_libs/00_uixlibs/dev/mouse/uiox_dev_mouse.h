/*
 * 50_UIX/00_libs/00_uixlibs/dev/mouse/uiox_dev_mouse.h
 *
 * MOUSE — the application's view of the pointer, at EL0.
 *
 * ── FAMILY 3 (EVENT) ────────────────────────────────────────────────
 * Read-based, like KBD.  uiox_mouse_if.h:44-47:
 *
 *     typedef struct {
 *         uiox_dev_ops_t     head;  // name, probe, init, deinit, ioctl
 *         uiox_event_ops_t   ev;    // init, deinit, pending, read, isr
 *     } uiox_mouse_ops_t;
 *
 * and :64 says it plainly — "Event path — reached from read() on
 * /dev/mouse<N>".  So the shape is open, wait, read, close.
 *
 * ── event_read NEVER RETURNS 0 ──────────────────────────────────────
 * uiox_mouse_if.h:67-68: "event_read returns bytes written or a
 * negative errno — NEVER 0."  A zero return is a fault, not an empty
 * queue; the way to wait is pending, via uix_poll.  The layer encodes
 * that in uiox_dev_mouse_read() rather than leaving each app to
 * discover it.
 *
 * ── WHAT IS DELIBERATELY ABSENT ─────────────────────────────────────
 *   warp(x, y)     a write with no ioctl in the vocabulary.  The
 *                  cursor MOVES by events; it cannot be commanded.
 *   add_zone / add_callback  struct registration and a function
 *                  pointer, both kernel-side.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_MOUSE_H
#define UIOX_DEV_MOUSE_H

#include "../uiox_dev_event.h"     /* the EVENT shape */
#include "../sys/uix_types.h"
#include "../PoStd/uix_poll.h"

#define UIOX_DEV_MOUSE_DEVPATH  "mouse"

/* The kernel's uiox_mouse_event_t, mirrored from
 * 30_KIX/32_FS/00_drvbuff/02_Sensors/mouse/uiox_mouse_buf.h:52-65.
 *
 * TWELVE fields and FIVE of them are SIGNED narrow types — dx, dy are
 * int16_t; dz, dw are int8_t.  A mirror that wrote them unsigned would
 * turn a leftward move of -20 into 65516 and a scroll of -1 into 255,
 * and the build would stay green.  The signed widths are the point of
 * this struct, so they are reproduced exactly.
 *
 * `type` is an enum and int-sized. */
typedef struct {
    int                type;          /* UIOX_MOUSE_EV_*              */
    int                x;             /* absolute cursor X (post-accel) */
    int                y;             /* absolute cursor Y            */
    short              dx;            /* relative X delta — SIGNED    */
    short              dy;            /* relative Y delta — SIGNED    */
    signed char        dz;            /* scroll vertical  — SIGNED    */
    signed char        dw;            /* scroll horizontal — SIGNED   */
    unsigned char      button;        /* button index for BTN_* events */
    unsigned char      buttons;       /* full bitmask at event time   */
    unsigned char      gesture;       /* gesture code for GESTURE     */
    unsigned int       click_count;   /* 1 = click, 2 = double-click  */
    unsigned long long ts_ns;
} uiox_dev_mouse_event_t;

/* The kernel's event types, for printing only. */
#define UIOX_DEV_MOUSE_EV_MOVE        0
#define UIOX_DEV_MOUSE_EV_BTN_PRESS   1
#define UIOX_DEV_MOUSE_EV_BTN_RELEASE 2
#define UIOX_DEV_MOUSE_EV_CLICK       3
#define UIOX_DEV_MOUSE_EV_DBLCLICK    4
#define UIOX_DEV_MOUSE_EV_SCROLL_V    5
#define UIOX_DEV_MOUSE_EV_SCROLL_H    6
#define UIOX_DEV_MOUSE_EV_CONNECT     7
#define UIOX_DEV_MOUSE_EV_DISCONNECT  8
#define UIOX_DEV_MOUSE_EV_GESTURE     9

typedef struct {
    uiox_dev_handle_t h;
    int               last_x;         /* cursor, from the last event */
    int               last_y;
    int               have_pos;
} uiox_dev_mouse_t;

uiox_dev_err_t uiox_dev_mouse_open (uiox_dev_mouse_t *m, int unit, int flags);
uiox_dev_err_t uiox_dev_mouse_close(uiox_dev_mouse_t *m);

/* Blocks until an event is ready, backed by ev.pending. */
uiox_dev_err_t uiox_dev_mouse_wait (uiox_dev_mouse_t *m, int timeout_ms);

/* Pops ONE event and updates the cached cursor from it.  The kernel's
 * event_read never returns 0, so a zero-length read here is reported as
 * UIOX_DEV_EIO — a fault — not as "nothing ready". */
uiox_dev_err_t uiox_dev_mouse_read (uiox_dev_mouse_t *m,
                                    uiox_dev_mouse_event_t *out);

/* The cursor position the driver sent with the last event.  Every event
 * carries absolute x/y after acceleration, so no command is needed for
 * this — it is a remembered field, not a query. */
uiox_dev_err_t uiox_dev_mouse_cursor(const uiox_dev_mouse_t *m,
                                     int *x, int *y);

#endif /* UIOX_DEV_MOUSE_H */
