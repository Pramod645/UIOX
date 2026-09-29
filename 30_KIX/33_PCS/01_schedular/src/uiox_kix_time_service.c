/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_time_service.c
 *
 * Time SERVICE — the implementations.  See the header for why the file
 * and its functions are named this, and which of them a number reaches.
 *
 * ── who calls these ─────────────────────────────────────────────────
 *   50_scpcs/src/uiox_kix_scpcs_dispatch.c   eight of them, by number
 *   any kernel code wanting a time or a timer   directly
 *
 * ── the one that is still wrong ─────────────────────────────────────
 * clock_nanosleep busy-waits through ndelay().  Honouring the duration
 * while occupying the processor starves every other runnable process —
 * worse than not sleeping.  The fix is proc_sleep plus a callout firing
 * uiox_kix_psa_proc_wakeup, which is the highest value integration left
 * in the process layer.  Left visible rather than commented away.
 *
 * @version 5.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_time_service.h"
#include "../include/uiox_kix_timekeeping.h"
#include "../include/uiox_kix_scheduler.h"   /* xtime, jiffies, HZ, TICK_NSEC */

/* ── POSIX timer pool ────────────────────────────────────────────────
 * Static, because freestanding has no heap.  id is the slot index and
 * never changes, so a TimerNode's data argument can carry it and the
 * expiry callback can find its way back. */
static PosixTimer posix_timers[MAX_POSIX_TIMERS];
static bool       posix_timer_pool_init = false;

static void ensure_pool(void)
{
    if (!posix_timer_pool_init) {
        int i;
        for (i = 0; i < MAX_POSIX_TIMERS; i++) {
            posix_timers[i].clock_id = UIOX_CLOCK_REALTIME;
            posix_timers[i].overruns = 0;
            posix_timers[i].active   = false;
            posix_timers[i].node     = (TimerNode *)0;
            posix_timers[i].id       = i;
            posix_timers[i].spec.it_interval.tv_sec  = 0;
            posix_timers[i].spec.it_interval.tv_usec = 0;
            posix_timers[i].spec.it_value.tv_sec     = 0;
            posix_timers[i].spec.it_value.tv_usec    = 0;
        }
        posix_timer_pool_init = true;
    }
}

/* ── uiox_kix_time_service_time ────────────────────────────────────── */
int64_t uiox_kix_time_service_time(void)
{
    return xtime.tv_sec;
}

/* ── uiox_kix_time_service_gettimeofday ────────────────────────────── */
int uiox_kix_time_service_gettimeofday(TimeVal *tv)
{
    if (!tv) return -1;
    tv->tv_sec  = xtime.tv_sec;
    tv->tv_usec = xtime.tv_nsec / 1000;
    return 0;
}

/* ── uiox_kix_time_service_adjtimex ──────────────────────────────────
 * Shifts the WALL clock.  The monotonic clock is derived from the tick
 * count, so an adjustment here does not move it — which is the whole
 * reason both clocks exist.
 *
 * Privilege is checked by the CALLER: this layer has no notion of
 * entitlement. */
int uiox_kix_time_service_adjtimex(int64_t delta_sec, int32_t delta_nsec)
{
    xtime.tv_sec  += delta_sec;
    xtime.tv_nsec  = (uint32_t)((int32_t)xtime.tv_nsec + delta_nsec);

    while (xtime.tv_nsec >= 1000000000U) {
        xtime.tv_nsec -= 1000000000U;
        xtime.tv_sec++;
    }
    return 0;
}

/* ── alarm_remaining ─────────────────────────────────────────────────
 * Seconds left on the process's alarm, 0 when none is armed.
 *
 * Shared by alarm() and getitimer() rather than written twice: the two
 * asks are the same arithmetic, and duplicating it is how two answers
 * to one question start to drift. */
static unsigned int alarm_remaining(const uiox_sched_proc_t *p)
{
    uint64_t left;

    if (!p || !p->p.p_alarm_active) return 0;

    left = p->p.p_alarm_expire > jiffies ? p->p.p_alarm_expire - jiffies : 0;
    return (unsigned int)(left / (uint64_t)HZ);
}

/* ── alarm_callback ──────────────────────────────────────────────────── */
static void alarm_callback(unsigned long data)
{
    (void)data;   /* a real kernel sends SIGALRM to pid data here */
}

/* ── uiox_kix_time_service_alarm ─────────────────────────────────────
 * POSIX alarm returns the seconds REMAINING on a previously set alarm,
 * and a new alarm REPLACES the old.  Both require remembering the
 * outstanding callout and its deadline — which is why the PROCESS ENTRY
 * carries p_alarm_expire and p_alarm_active.
 *
 * Those two live on uiox_kix_psa_proc_t, not on the scheduler wrapper:
 * alarm() is a process syscall, and the wrapper holds scheduling POLICY
 * rather than per-process state a syscall reads and writes. */
unsigned int uiox_kix_time_service_alarm(uiox_sched_proc_t *p,
                                         unsigned int seconds)
{
    unsigned int remaining;

    if (!p) return 0;

    remaining = alarm_remaining(p);

    /* Cancel any previous alarm before arming a new one — that IS the
     * replacement semantics, not an optimisation. */
    p->p.p_alarm_active = false;

    if (seconds > 0) {
        p->p.p_alarm_expire = jiffies + (uint64_t)seconds * HZ;
        p->p.p_alarm_active = true;
        timer_add(p->p.p_alarm_expire, alarm_callback,
                  (unsigned long)p->p.p_pid);
    }

    return remaining;
}

/* ── uiox_kix_time_service_setitimer ───────────────────────────────── */
int uiox_kix_time_service_setitimer(uiox_sched_proc_t *p,
                                    const ItimerVal *new_val,
                                    ItimerVal *old_val)
{
    unsigned int secs;

    if (!p || !new_val) return -1;

    if (old_val) {
        old_val->it_value.tv_sec     = (int64_t)alarm_remaining(p);
        old_val->it_value.tv_usec    = 0;
        old_val->it_interval.tv_sec  = 0;
        old_val->it_interval.tv_usec = 0;
    }

    secs = (unsigned int)new_val->it_value.tv_sec;
    (void)uiox_kix_time_service_alarm(p, secs);

    return 0;
}

/* ── uiox_kix_time_service_getitimer ─────────────────────────────────
 * Reads the remaining time WITHOUT disturbing the timer.
 *
 * That is the whole difference from setitimer, and the reason this is a
 * separate function rather than a NULL-new_val branch: setitimer
 * CANCELS the previous alarm before arming, so a poll routed through it
 * would destroy the timer it was asked to report.
 *
 * which is accepted and not interpreted — there is one alarm slot on a
 * process, so all three of ITIMER_REAL / VIRTUAL / PROF report it. */
int uiox_kix_time_service_getitimer(uiox_sched_proc_t *p, ItimerVal *value)
{
    if (!p || !value) return -1;

    value->it_value.tv_sec     = (int64_t)alarm_remaining(p);
    value->it_value.tv_usec    = 0;
    value->it_interval.tv_sec  = 0;
    value->it_interval.tv_usec = 0;

    return 0;
}

/* ── posix_timer_expire ──────────────────────────────────────────────── */
static void posix_timer_expire(unsigned long data)
{
    int         idx = (int)data;
    PosixTimer *pt;

    if (idx < 0 || idx >= MAX_POSIX_TIMERS) return;
    pt = &posix_timers[idx];

    pt->overruns++;

    /* A repeating timer re-arms itself; a one-shot deactivates.  The
     * re-arm reads it_interval, which is what separates the two. */
    if (pt->spec.it_interval.tv_sec > 0) {
        uint64_t interval_ticks =
            (uint64_t)pt->spec.it_interval.tv_sec * HZ;
        pt->node = timer_add(jiffies + interval_ticks,
                             posix_timer_expire, (unsigned long)idx);
    } else {
        pt->active = false;
    }
}

/* ── POSIX per-process timers ────────────────────────────────────────── */
int uiox_kix_time_service_timer_create(ClockId clock_id, PosixTimer **out)
{
    int i;
    if (!out) return -1;

    ensure_pool();
    for (i = 0; i < MAX_POSIX_TIMERS; i++) {
        if (!posix_timers[i].active) {
            posix_timers[i].clock_id = clock_id;
            posix_timers[i].overruns = 0;
            posix_timers[i].active   = false;
            posix_timers[i].node     = (TimerNode *)0;
            *out                     = &posix_timers[i];
            return i;
        }
    }
    return -1;   /* pool exhausted */
}

int uiox_kix_time_service_timer_settime(PosixTimer *t, const ItimerVal *spec)
{
    if (!t || !spec) return -1;

    t->spec = *spec;

    if (t->node) { timer_del(t->node); t->node = (TimerNode *)0; }

    if (spec->it_value.tv_sec > 0) {
        uint64_t ticks = (uint64_t)spec->it_value.tv_sec * HZ;
        t->node = timer_add(jiffies + ticks, posix_timer_expire,
                            (unsigned long)t->id);
        t->active = true;
    } else {
        t->active = false;   /* a zero it_value disarms */
    }
    return 0;
}

int uiox_kix_time_service_timer_gettime(PosixTimer *t, ItimerVal *out_spec)
{
    if (!t || !out_spec) return -1;
    *out_spec = t->spec;
    return 0;
}

int uiox_kix_time_service_timer_getoverrun(PosixTimer *t)
{
    int ov;
    if (!t) return -1;

    /* Reads and CLEARS, per POSIX — the count is per interval, not
     * cumulative, so a caller polling twice must not see it twice. */
    ov = (int)t->overruns;
    t->overruns = 0;
    return ov;
}

int uiox_kix_time_service_timer_delete(PosixTimer *t)
{
    if (!t) return -1;

    if (t->node) {
        timer_del(t->node);
        timer_free(t->node);
        t->node = (TimerNode *)0;
    }
    t->active = false;
    return 0;
}

/* ── Clocks ──────────────────────────────────────────────────────────
 * REALTIME reads xtime, which adjtimex may move.
 * MONOTONIC reads the tick-derived counter, which it may not. */
int uiox_kix_time_service_clock_gettime(ClockId id, XTime *out)
{
    if (!out) return -1;

    if (id == UIOX_CLOCK_REALTIME) {
        *out = xtime;
    } else {
        uint64_t ns  = timekeeping_monotonic_ns();
        out->tv_sec  = (int64_t)(ns / 1000000000ULL);
        out->tv_nsec = (uint32_t)(ns % 1000000000ULL);
    }
    return 0;
}

int uiox_kix_time_service_clock_settime(ClockId id, const XTime *in)
{
    if (!in) return -1;
    if (id != UIOX_CLOCK_REALTIME) return -1;   /* monotonic cannot be set */

    xtime = *in;
    return 0;
}

/* ── uiox_kix_time_service_clock_getres ──────────────────────────────
 * The resolution is the tick length: xtime advances once per tick, so
 * this kernel cannot report a time finer than one tick.  Both clocks
 * report the same resolution for that reason. */
int uiox_kix_time_service_clock_getres(ClockId id, XTime *out_res)
{
    (void)id;
    if (!out_res) return -1;

    out_res->tv_sec  = 0;
    out_res->tv_nsec = (uint32_t)TICK_NSEC;
    return 0;
}

/* ── uiox_kix_time_service_clock_nanosleep ───────────────────────────
 * Busy-waits.  See the banner — this is the call that should sleep. */
int uiox_kix_time_service_clock_nanosleep(ClockId id, uint64_t nsecs)
{
    (void)id;
    ndelay((unsigned long)nsecs);
    return 0;
}
