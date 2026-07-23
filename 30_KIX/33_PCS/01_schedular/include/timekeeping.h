#ifndef UIOX_TIMEKEEPING_H
#define UIOX_TIMEKEEPING_H

#include "sched_types.h"

/* ─────────────────────────────────────────────────────────────
 * Timer source descriptor (timer_opts equivalent)
 *
 * Table 6-1:
 *   name           — string identifying the timer source
 *   mark_offset    — records exact time of last tick
 *   get_offset     — returns time elapsed since last tick (ns)
 *   monotonic_clock— nanoseconds since kernel init
 *   delay          — busy-wait for a given loop count
 * ───────────────────────────────────────────────────────────── */
typedef struct TimerOpts {
    const char   *name;
    TimerSource   source;
    void        (*mark_offset)(void);
    uint64_t    (*get_offset)(void);
    uint64_t    (*monotonic_clock)(void);
    void        (*delay)(unsigned long loops);
} TimerOpts;

/* ─────────────────────────────────────────────────────────────
 * Timekeeping API
 * ───────────────────────────────────────────────────────────── */

/* Initialise timekeeping (time_init equivalent) */
void      timekeeping_init(void);

/* Select best available timer source */
TimerOpts *select_timer(void);

/* Read simulated CMOS/RTC time (get_cmos_time equivalent) */
int64_t   get_cmos_time(void);

/*
 * update_times — called at every tick to advance xtime.
 * Also invokes calc_load().
 */
void      update_times(void);

/* Returns current wall-clock seconds since epoch */
int64_t   timekeeping_get_seconds(void);

/* Returns nanoseconds since kernel init (monotonic) */
uint64_t  timekeeping_monotonic_ns(void);

/* TSC calibration stub */
uint64_t  calibrate_tsc(void);

/* Delay functions (busy-wait simulation) */
void      udelay(unsigned long usecs);
void      ndelay(unsigned long nsecs);
void      calibrate_delay(void);

#endif /* UIOX_TIMEKEEPING_H */
