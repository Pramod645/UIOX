/**
 * @file    uiox_mouse_if.c
 * @brief   UIOX Mouse interface driver implementation.
 * @date    2026-06-01
 */

#include "uiox_mouse_if.h"
#include "uiox_klibc.h"

int uiox_mouse_if_config(uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw)
{
    if (!mif || !hw) return -EINVAL;
    memset(mif, 0, sizeof(*mif));
    mif->hw     = hw;
    mif->primed = true;
    return 0;
}

int uiox_mouse_if_poll(uiox_mouse_if_t      *mif,
                        uiox_mouse_ringbuf_t *dst_rb)
{
    if (!mif || !dst_rb || !mif->primed) return -EINVAL;

    mif->stats.poll_count++;

    bool connected = uiox_mouse_hw_connected(mif->hw);
    if (connected != mif->prev_connected) {
        mif->prev_connected = connected;
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = connected ? UIOX_MOUSE_EV_CONNECT
                            : UIOX_MOUSE_EV_DISCONNECT;
        if (uiox_mouse_buf_push(dst_rb, &ev))
            connected ? mif->stats.connect_events++
                      : mif->stats.disconnect_events++;
        else
            mif->stats.reports_dropped++;
    }

    if (!connected) return 0;

    uiox_mouse_raw_t raw;
    memset(&raw, 0, sizeof(raw));
    int r = uiox_mouse_hw_read_report(mif->hw, &raw);
    if (r <= 0) return r;

    mif->stats.reports_received++;
    int pushed = 0;

    if (raw.dx || raw.dy) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_MOVE;
        ev.dx      = raw.dx;
        ev.dy      = raw.dy;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
        else mif->stats.reports_dropped++;
    }

    uint8_t changed = raw.buttons ^ mif->prev_buttons;
    uint8_t b;
    for (b = 0; b < UIOX_MOUSE_MAX_BUTTONS; b++) {
        if (!(changed & (1u << b))) continue;
        bool pressed = (raw.buttons >> b) & 1u;
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = pressed ? UIOX_MOUSE_EV_BTN_PRESS
                             : UIOX_MOUSE_EV_BTN_RELEASE;
        ev.button  = b;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
        else mif->stats.reports_dropped++;
    }
    mif->prev_buttons = raw.buttons;

    if (raw.dz) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_SCROLL_V;
        ev.dz      = raw.dz;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
    }

    if (raw.dw) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_SCROLL_H;
        ev.dw      = raw.dw;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
    }

    return pushed;
}

void uiox_mouse_if_stats_get(const uiox_mouse_if_t *mif,
                               uiox_mouse_if_stats_t *out)
{
    if (!mif || !out) return;
    memcpy(out, &mif->stats, sizeof(*out));
}

void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif)
{
    if (!mif) return;
    memset(&mif->stats, 0, sizeof(mif->stats));
}
