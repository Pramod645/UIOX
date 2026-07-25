/**
 * @file    uiox_mouse_if.h
 * @brief   UIOX Mouse interface driver.
 * @version 1.0.0
 * @date    2026-06-01
 */

#ifndef UIOX_MOUSE_IF_H
#define UIOX_MOUSE_IF_H

#include "uiox_mouse_hw.h"
#include "uiox_mouse_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t poll_count;
    uint64_t reports_received;
    uint64_t reports_dropped;
    uint64_t connect_events;
    uint64_t disconnect_events;
} uiox_mouse_if_stats_t;

typedef struct {
    uiox_mouse_hw_t       *hw;
    bool                   primed;
    bool                   prev_connected;
    uint8_t                prev_buttons;
    uiox_mouse_if_stats_t  stats;
} uiox_mouse_if_t;

int  uiox_mouse_if_config    (uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw);
int  uiox_mouse_if_poll      (uiox_mouse_if_t *mif,
                               uiox_mouse_ringbuf_t *dst_rb);
void uiox_mouse_if_stats_get (const uiox_mouse_if_t *mif,
                               uiox_mouse_if_stats_t *out);
void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MOUSE_IF_H */
