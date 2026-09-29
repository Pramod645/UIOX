/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_time_service.h
 *
 * Time SERVICE — what the scheduler offers to whoever asks for a time,
 * a timer, or a sleep.
 *
 * ── why the file is called this ─────────────────────────────────────
 * It was syscall_time.h, then time_calls.h.  Both named the CATEGORY
 * rather than the job — "things you can call" is true of every file in
 * the tree — and the sys_ prefix in the first name was worse: it said
 * "an entry point a syscall number reaches" about sixteen functions, of
 * which four had a number routed to them.
 *
 * A service layer is what this is: the scheduler OWNS the clock state
 * (xtime, jiffies, the timer wheel) and this exposes it.  The sys_ prefix
 * is now reserved for what SCiX genuinely routes to, and the functions
 * here are named for what they do.
 *
 * ── the three layers, and who owns what ────────────────────────────
 *   40_SystemCallInterface      number -> subsystem     knows no state
 *   50_scpcs dispatch           number -> entry point   knows the names
 *   01_schedular time_service   the work                reads xtime,
 *                                                       jiffies, the wheel
 *
 * Only the last needs the scheduler's state, which is why it lives here
 * rather than one layer up.
 *
 * ── which of these a number can reach today ────────────────────────
 *   reachable     gettimeofday 67 · setitimer 69 · getitimer 70 ·
 *                 clock_gettime 87 · clock_settime 88 · clock_getres 89 ·
 *                 nanosleep 91 · adjtimex 140
 *   not yet       time, timer_create, timer_settime, timer_gettime,
 *                 timer_getoverrun, timer_delete
 *
 * The six unreachable ones are correct code with no row pointing at
 * them.  That is the state of the NUMBERING, not of this layer — and
 * because the names no longer claim to be syscall entry points, the
 * distinction is visible instead of implied.
 *
 * ── pointers are kernel-side ────────────────────────────────────────
 * Every function below takes or returns a KERNEL pointer.  None of them
 * validates a user address: that is the boundary's job, and each 50_scpcs
 * wrapper gates its own pointer before calling in.  A function here may
 * therefore write through what it is given.
 *
 * @version 5.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_TIME_SERVICE_H
#define UIOX_KIX_TIME_SERVICE_H

#include "sched_types.h"
#include "uiox_kix_timer.h"

/* ── timeval — microsecond resolution wall-clock snapshot ─────────── */
typedef struct {
    int64_t  tv_sec;
    uint32_t tv_usec;
} TimeVal;

/* ── itimerval — interval timer (setitimer / getitimer / alarm) ───── */
typedef struct {
    TimeVal it_interval;   /* reload interval after expiry */
    TimeVal it_value;      /* time until next expiry       */
} ItimerVal;

/* ── POSIX clock IDs ─────────────────────────────────────────────────
 * These values MUST match what 50_scpcs uses in its clock wrapper, so
 * the two agree on what clock 0 and clock 1 mean. */
typedef enum {
    UIOX_CLOCK_REALTIME  = 0,  /* wall-clock time (= xtime) */
    UIOX_CLOCK_MONOTONIC = 1   /* monotonic, no warp        */
} ClockId;

/* ── POSIX timer handle ─────────────────────────────────────────────── */
#define MAX_POSIX_TIMERS 16

typedef struct {
    ClockId    clock_id;
    ItimerVal  spec;
    uint64_t   overruns;
    bool       active;
    TimerNode *node;         /* underlying dynamic timer */
    int        id;
} PosixTimer;

/* ── The services ──────────────────────────────────────────────────── */

/* seconds since epoch */
int64_t uiox_kix_time_service_time(void);

/* seconds + microseconds since epoch, written to a kernel TimeVal */
int uiox_kix_time_service_gettimeofday(TimeVal *tv);

/* shift the wall clock by a delta.  Does NOT move the monotonic clock */
int uiox_kix_time_service_adjtimex(int64_t delta_sec, int32_t delta_nsec);

/* interval timer for one process; reports the previous one via old_val */
int uiox_kix_time_service_setitimer(uiox_sched_proc_t *p,
                                    const ItimerVal *new_val,
                                    ItimerVal *old_val);

/* read the remaining time WITHOUT disturbing the timer.  The difference
 * from setitimer matters: setitimer cancels before arming, so a poll
 * routed through it would destroy what it was reading */
int uiox_kix_time_service_getitimer(uiox_sched_proc_t *p, ItimerVal *value);

/* one-shot alarm; returns seconds REMAINING on any previous alarm */
unsigned int uiox_kix_time_service_alarm(uiox_sched_proc_t *p,
                                         unsigned int seconds);

/* POSIX per-process timers */
int uiox_kix_time_service_timer_create(ClockId clock_id, PosixTimer **out);
int uiox_kix_time_service_timer_settime(PosixTimer *t, const ItimerVal *spec);
int uiox_kix_time_service_timer_gettime(PosixTimer *t, ItimerVal *out_spec);
int uiox_kix_time_service_timer_getoverrun(PosixTimer *t);
int uiox_kix_time_service_timer_delete(PosixTimer *t);

/* clocks */
int uiox_kix_time_service_clock_gettime(ClockId id, XTime *out);
int uiox_kix_time_service_clock_settime(ClockId id, const XTime *in);
int uiox_kix_time_service_clock_getres(ClockId id, XTime *out_res);

/* sleep.  Busy-waits today — the call that should use proc_sleep */
int uiox_kix_time_service_clock_nanosleep(ClockId id, uint64_t nsecs);

#endif /* UIOX_KIX_TIME_SERVICE_H */
