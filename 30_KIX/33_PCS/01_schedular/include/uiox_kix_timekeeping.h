/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_timekeeping.h
 *
 * The time SOURCE — which counter is trusted, and what the delay loop
 * is worth.
 *
 * ── what is NOT here, and why ───────────────────────────────────────
 * There is no update_times().  An earlier version declared and defined
 * one, and it advanced xtime with the same three lines clock_tick
 * already runs — so a tick that called both moved the wall clock TWICE.
 *
 * The declaration is gone with the definition.  Advancing xtime is
 * Algorithm 2 step 1, "restart clock", and that is clock.c's job.
 *
 * ── the split, stated once ──────────────────────────────────────────
 *   clock.c        advances xtime on every tick
 *   timekeeping.c  selects the source, calibrates, provides the
 *                  monotonic reading and the delay loops
 *   time_service   reads xtime and serves the syscalls above it
 *
 * ── why a source table rather than a #define ────────────────────────
 * A machine may have an HPET, a TSC, a PIT, or none.  Which is used
 * changes the resolution and whether monotonic time can be read without
 * an interrupt.  Encoding that as a table with a selection order — HPET
 * > TSC > PIT > none — means a target supplies its own function bodies
 * without touching the selection or any caller.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#ifndef UIOX_TIMEKEEPING_H
#define UIOX_TIMEKEEPING_H

#include "sched_types.h"

/* ── Timer source descriptor ─────────────────────────────────────────
 *  mark_offset     — record the exact time of the last tick
 *  get_offset      — ns elapsed since that tick, for sub-tick accuracy
 *  monotonic_clock — ns since kernel init
 *  delay           — busy-wait a loop count
 *
 * A source with NULL pointers is the "none" entry: it can still delay,
 * but it reports monotonic time from jiffies alone. */
typedef struct TimerOpts {
    const char   *name;
    TimerSource   source;
    void        (*mark_offset)(void);
    uint64_t    (*get_offset)(void);
    uint64_t    (*monotonic_clock)(void);
    void        (*delay)(unsigned long loops);
} TimerOpts;

/* ── API ─────────────────────────────────────────────────────────────
 * Select a source, calibrate it, calibrate the delay loop, then seed
 * xtime from the boot epoch.  ORDER MATTERS: reading the clock before
 * selecting leaves active_timer NULL and monotonic_ns() falling back. */
void       timekeeping_init(void);

/* Pick the best available source by the table's preference order. */
TimerOpts *select_timer(void);

/* Read the wall clock at boot.  A real kernel reads CMOS ports
 * 0x70/0x71 here; this returns xtime.tv_sec, already seeded. */
int64_t    get_cmos_time(void);

/* Returns the current wall-clock seconds. */
int64_t    timekeeping_get_seconds(void);

/* Nanoseconds since init.  MONOTONIC: derived from the tick count, not
 * from xtime, so an adjtimex that shifts the wall clock does not make
 * this reading go backwards.  That is the whole reason it exists. */
uint64_t   timekeeping_monotonic_ns(void);

/* Fixed 1 GHz in this build; a real kernel measures TSC against the PIT. */
uint64_t   calibrate_tsc(void);

/* Busy-wait.  For boot and hardware handshakes, where the scheduler is
 * not running yet and a sleep is impossible. */
void       udelay(unsigned long usecs);
void       ndelay(unsigned long nsecs);
void       calibrate_delay(void);

#endif /* UIOX_TIMEKEEPING_H */
