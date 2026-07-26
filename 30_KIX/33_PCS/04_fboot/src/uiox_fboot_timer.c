/**
 * @file  uiox_fboot_timer.c
 * @brief UIOX Fast Boot — high-resolution boot timer.
 * @date  2026-07-08
 */
#include "../include/uiox_fboot_timer.h"

/* ── Weak platform defaults — override in BSP ────────────────────────── */
__attribute__((weak))
uint64_t uiox_fb_plat_read_counter(void)
{
#if defined(__aarch64__)
    uint64_t v;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v));
    return v;
#elif defined(__riscv)
    uint64_t v;
    __asm__ volatile("rdtime %0" : "=r"(v));
    return v;
#else
    /* Fallback: increment a software counter (test / x86 host build) */
    static volatile uint64_t sw_tick = 0u;
    return ++sw_tick;
#endif
}

__attribute__((weak))
uint64_t uiox_fb_plat_counter_freq_hz(void)
{
#if defined(__aarch64__)
    uint64_t v;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
#else
    return 1000000u;   /* 1 MHz default — 1 tick = 1 µs */
#endif
}

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_fb_err_t uiox_fb_timer_init(uiox_fb_timer_t *t)
{
    if (!t) return UIOX_FB_ERR_INVAL;

    t->freq_hz     = uiox_fb_plat_counter_freq_hz();
    t->reset_ticks = uiox_fb_plat_read_counter();
    t->initialized = (t->freq_hz > 0u);

    return t->initialized ? UIOX_FB_OK : UIOX_FB_ERR_INVAL;
}

/* =========================================================================
 * Ticks → microseconds
 * ====================================================================== */
uint64_t uiox_fb_timer_ticks_to_us(const uiox_fb_timer_t *t, uint64_t ticks)
{
    if (!t || t->freq_hz == 0u) return 0u;
    /* Avoid 64-bit overflow: ticks * 1_000_000 / freq */
    return (ticks / t->freq_hz) * 1000000u
         + (ticks % t->freq_hz) * 1000000u / t->freq_hz;
}

/* =========================================================================
 * Now in microseconds (since timer_init)
 * ====================================================================== */
uint64_t uiox_fb_timer_now_us(const uiox_fb_timer_t *t)
{
    if (!t || !t->initialized) return 0u;
    uint64_t elapsed = uiox_fb_plat_read_counter() - t->reset_ticks;
    return uiox_fb_timer_ticks_to_us(t, elapsed);
}

/* =========================================================================
 * Busy-wait — early boot only, no scheduler available
 * ====================================================================== */
void uiox_fb_timer_udelay(const uiox_fb_timer_t *t, uint64_t us)
{
    if (!t || !t->initialized || us == 0u) return;
    uint64_t deadline = uiox_fb_timer_now_us(t) + us;
    while (uiox_fb_timer_now_us(t) < deadline) {
        /* spin — insert WFI on battery-critical paths */
    }
}
