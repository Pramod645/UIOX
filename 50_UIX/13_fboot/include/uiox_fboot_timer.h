/**
 * @file  uiox_fboot_timer.h
 * @brief UIOX Fast Boot — high-resolution boot timer abstraction.
 *
 * Wraps the hardware free-running counter (ARM generic timer /
 * RISC-V mtime) into a microsecond-resolution monotonic clock that is
 * available from the very first instruction after reset.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_FBOOT_TIMER_H
#define UIOX_FBOOT_TIMER_H

#include "uiox_fboot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Platform hooks — implement in BSP layer
 * ====================================================================== */

/**
 * @brief Read hardware counter in raw ticks.
 *        ARM64: MRS x0, CNTPCT_EL0
 *        RISC-V: RDTIME
 */
__attribute__((weak)) uint64_t uiox_fb_plat_read_counter(void);

/**
 * @brief Return counter frequency in Hz.
 *        ARM64: MRS x0, CNTFRQ_EL0
 *        Typical: 24 MHz, 50 MHz, 100 MHz
 */
__attribute__((weak)) uint64_t uiox_fb_plat_counter_freq_hz(void);

/* =========================================================================
 * Timer context
 * ====================================================================== */
typedef struct {
    uint64_t  freq_hz;
    uint64_t  reset_ticks;      /**< Counter value captured at reset       */
    bool      initialized;
} uiox_fb_timer_t;

/* =========================================================================
 * API
 * ====================================================================== */

/** Initialise — call as early as possible (before DDR init). */
uiox_fb_err_t uiox_fb_timer_init   (uiox_fb_timer_t *t);

/** Read microseconds elapsed since timer_init(). */
uint64_t      uiox_fb_timer_now_us (const uiox_fb_timer_t *t);

/** Convert raw ticks → microseconds. */
uint64_t      uiox_fb_timer_ticks_to_us(const uiox_fb_timer_t *t,
                                         uint64_t ticks);

/** Busy-wait for @us microseconds. Use only during very early boot. */
void          uiox_fb_timer_udelay (const uiox_fb_timer_t *t, uint64_t us);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FBOOT_TIMER_H */
