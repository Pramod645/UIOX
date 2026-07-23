/*
 * 30_KIX/33_PCS/include/uiox_timer.h
 *
 * Jiffies counter and timer-wheel API.
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_TIMER_H
#define UIOX_TIMER_H

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uix_uint64_t g_jiffies;   /* incremented by timer IRQ  */

void          uiox_timer_init(void);
uix_uint64_t  uiox_timer_now(void);
void          uiox_timer_tick(void);       /* called from timer IRQ handler */

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TIMER_H */
