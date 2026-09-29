/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_timekeeping.c
 *
 * Time source selection and the monotonic clock.
 *
 * ── what this file does NOT do, and why ─────────────────────────────
 * It does not advance xtime.  An earlier version carried update_times()
 * doing exactly the three lines clock_tick already does:
 *
 *     xtime.tv_nsec += TICK_NSEC;
 *     if (xtime.tv_nsec >= 1000000000UL) { tv_nsec -= 1e9; tv_sec++; }
 *
 * Two functions writing one counter means a tick that calls both
 * advances the clock TWICE — a wall clock that runs at double rate, and
 * a bug that looks like a broken hardware timer.
 *
 * Bach's Algorithm 2 step 1 is "restart clock", and advancing the wall
 * clock is the tick handler's job.  That is clock.c.  This file owns the
 * SOURCE — which timer is selected, what its resolution is, and the
 * monotonic reading that does not warp when the wall clock is adjusted.
 *
 * ── the split, stated once ──────────────────────────────────────────
 *   clock.c        advances xtime on every tick
 *   uiox_kix_timekeeping.c  selects the timer source, calibrates, provides
 *                  monotonic_ns() and the delay loops
 *   syscall_time.c reads xtime and exposes the sys_* entry points
 *
 * ── freestanding notes carried over ─────────────────────────────────
 *   get_cmos_time() does not call time(NULL) — there is no libc.  It
 *   returns xtime.tv_sec, which timekeeping_init() seeds from the
 *   BSP-provided epoch.  A real kernel reads CMOS ports 0x70/0x71 here.
 *
 *   calibrate_tsc() returns a fixed 1 GHz.  A real kernel measures TSC
 *   ticks across a known PIT interval.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_timekeeping.h"
#include "../include/uiox_kix_scheduler.h"    /* xtime, jiffies, TICK_NSEC */

/* ── Module-private state ──────────────────────────────────────────── */
static uint64_t   tsc_frequency  = 1000000000ULL;  /* 1 GHz default    */
static uint64_t   monotonic_base = 0;              /* ns since init     */
static uint64_t   loops_per_usec = 1000;           /* calibrated delay  */
static TimerOpts *active_timer   = (TimerOpts *)0;

/* ── Timer source implementations ────────────────────────────────────
 * All four report monotonic time the same way — jiffies times the tick
 * length — because this build has no way to read a hardware counter.
 * The table exists so the SELECTION is real and the seam is visible:
 * when a target has a TSC or an HPET, only the function bodies below
 * change, not the selection or the callers. */
static void     pit_mark_offset(void)            { }
static uint64_t pit_get_offset(void)             { return TICK_NSEC / 2; }
static uint64_t pit_monotonic(void)              { return jiffies * TICK_NSEC; }
static void     pit_delay(unsigned long loops)   { volatile unsigned long l = loops; while (l--); }

static void     tsc_mark_offset(void)            { }
static uint64_t tsc_get_offset(void)             { return 0; }
static uint64_t tsc_monotonic(void)              { return jiffies * TICK_NSEC; }
static void     tsc_delay(unsigned long loops)   { pit_delay(loops); }

static void     hpet_mark_offset(void)           { }
static uint64_t hpet_get_offset(void)            { return 0; }
static uint64_t hpet_monotonic(void)             { return jiffies * TICK_NSEC; }
static void     hpet_delay(unsigned long l)      { pit_delay(l); }

/* ── Timer source table — HPET > TSC > PIT > none ─────────────────── */
static TimerOpts timer_table[] = {
    { "timer_hpet", TIMER_SRC_HPET,
      hpet_mark_offset, hpet_get_offset, hpet_monotonic, hpet_delay },
    { "timer_tsc",  TIMER_SRC_TSC,
      tsc_mark_offset,  tsc_get_offset,  tsc_monotonic,  tsc_delay  },
    { "timer_pit",  TIMER_SRC_PIT,
      pit_mark_offset,  pit_get_offset,  pit_monotonic,  pit_delay  },
    { "timer_none", TIMER_SRC_NONE,
      (void (*)(void))0, (uint64_t (*)(void))0, (uint64_t (*)(void))0,
      pit_delay }
};
#define TIMER_TABLE_COUNT 4

/* ── select_timer ────────────────────────────────────────────────────
 * Picks the best available source.  In this build every source reports
 * as available, so HPET is taken first — the ordering in the table IS
 * the preference, and it is by quality: HPET > TSC > PIT > none.
 *
 * Returns the chosen descriptor and records it as active, so
 * timekeeping_monotonic_ns() has something to call. */
TimerOpts *select_timer(void)
{
    int i;
    for (i = 0; i < TIMER_TABLE_COUNT; i++) {
        if (timer_table[i].source != TIMER_SRC_NONE) {
            active_timer = &timer_table[i];
            return active_timer;
        }
    }
    active_timer = &timer_table[TIMER_TABLE_COUNT - 1];   /* none */
    return active_timer;
}

/* ── get_cmos_time ───────────────────────────────────────────────────
 * Reads the wall clock at boot.
 *
 * A real kernel reads CMOS I/O ports 0x70 / 0x71 here.  This build
 * returns xtime.tv_sec, which timekeeping_init() has already seeded —
 * and it does NOT call time(NULL), because freestanding has no libc. */
int64_t get_cmos_time(void)
{
    return xtime.tv_sec;
}

/* ── calibrate_tsc ───────────────────────────────────────────────────── */
uint64_t calibrate_tsc(void)
{
    tsc_frequency = 1000000000ULL;   /* fixed 1 GHz in simulation */
    return tsc_frequency;
}

/* ── calibrate_delay ─────────────────────────────────────────────────
 * Sets the loop count one microsecond is worth.  Simulated; a real
 * kernel times a known interval against the PIT. */
void calibrate_delay(void)
{
    loops_per_usec = 1000;
}

/* ── timekeeping_init ────────────────────────────────────────────────
 * Order matters: select the source, calibrate it, calibrate the delay
 * loop, THEN read the clock.  Reading before selecting would leave
 * active_timer NULL and monotonic_ns() falling back to the base. */
void timekeeping_init(void)
{
    select_timer();
    calibrate_tsc();
    calibrate_delay();

    xtime.tv_sec  = get_cmos_time();
    xtime.tv_nsec = 0;

    monotonic_base = 0;   /* monotonic time starts at zero, not at epoch */
}

/* ── timekeeping_get_seconds ────────────────────────────────────────── */
int64_t timekeeping_get_seconds(void)
{
    return xtime.tv_sec;
}

/* ── timekeeping_monotonic_ns ────────────────────────────────────────
 * Nanoseconds since init.  Monotonic because it is derived from the
 * TICK COUNT, not from xtime — so an adjtimex that shifts the wall
 * clock does not move this reading backwards.  That is the whole point
 * of having a separate monotonic clock. */
uint64_t timekeeping_monotonic_ns(void)
{
    if (active_timer && active_timer->monotonic_clock)
        return active_timer->monotonic_clock();
    return monotonic_base;
}

/* ── udelay / ndelay ─────────────────────────────────────────────────
 * Busy-wait.  These exist for the boot path and for hardware
 * handshakes, where a sleep is impossible because the scheduler is not
 * running yet.
 *
 * They are NOT for sys_clock_nanosleep, which uses them today — a
 * busy-wait there honours the requested duration while occupying the
 * processor for its whole length, starving every other runnable
 * process.  The fix is proc_sleep plus a callout, not a better
 * busy-wait. */
void udelay(unsigned long usecs)
{
    volatile unsigned long loops = usecs * loops_per_usec;
    while (loops--);
}

void ndelay(unsigned long nsecs)
{
    volatile unsigned long loops = (nsecs * loops_per_usec) / 1000;
    while (loops--);
}
