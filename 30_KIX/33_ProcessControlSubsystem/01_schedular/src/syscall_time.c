#include "syscall_time.h"
#include "timekeeping.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────
 * POSIX timer pool
 * ───────────────────────────────────────────────────────────── */
static PosixTimer posix_timers[MAX_POSIX_TIMERS];
static bool       posix_timer_pool_init = false;

static void ensure_pool(void)
{
    if (!posix_timer_pool_init) {
        memset(posix_timers, 0, sizeof posix_timers);
        for (int i = 0; i < MAX_POSIX_TIMERS; i++)
            posix_timers[i].id = i;
        posix_timer_pool_init = true;
    }
}

/* ─────────────────────────────────────────────────────────────
 * sys_time
 * ───────────────────────────────────────────────────────────── */
int64_t sys_time(void)
{
    printf("[sys_time] → %ld\n", (long)xtime.tv_sec);
    return xtime.tv_sec;
}

/* ─────────────────────────────────────────────────────────────
 * sys_gettimeofday
 * ───────────────────────────────────────────────────────────── */
int sys_gettimeofday(TimeVal *tv)
{
    if (!tv) return -1;
    tv->tv_sec  = xtime.tv_sec;
    tv->tv_usec = xtime.tv_nsec / 1000;
    printf("[sys_gettimeofday] → sec=%ld  usec=%u\n",
           (long)tv->tv_sec, tv->tv_usec);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_adjtimex — coarse clock adjustment
 * ───────────────────────────────────────────────────────────── */
int sys_adjtimex(int64_t delta_sec, int32_t delta_nsec)
{
    xtime.tv_sec  += delta_sec;
    xtime.tv_nsec  = (uint32_t)((int32_t)xtime.tv_nsec + delta_nsec);
    while (xtime.tv_nsec >= 1000000000U) {
        xtime.tv_nsec -= 1000000000U;
        xtime.tv_sec++;
    }
    printf("[sys_adjtimex] adjusted by %lds %dns  new_sec=%ld\n",
           (long)delta_sec, delta_nsec, (long)xtime.tv_sec);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * Alarm callback (fires in timer wheel)
 * ───────────────────────────────────────────────────────────── */
static void alarm_callback(unsigned long data)
{
    printf("[alarm] SIGALRM for pid=%lu\n", data);
}

/* ─────────────────────────────────────────────────────────────
 * sys_alarm — schedule a SIGALRM in 'seconds' seconds
 * Returns remaining seconds of any previous alarm.
 * ───────────────────────────────────────────────────────────── */
unsigned int sys_alarm(Process *p, unsigned int seconds)
{
    unsigned int remaining = 0;

    if (p->alarm_active) {
        uint64_t left = p->alarm_expire > jiffies
                        ? p->alarm_expire - jiffies : 0;
        remaining = (unsigned int)(left / (uint64_t)HZ);
        p->alarm_active = false;
    }

    if (seconds > 0) {
        p->alarm_expire = jiffies + (uint64_t)seconds * HZ;
        p->alarm_active = true;
        /* Register in timer wheel */
        timer_add(p->alarm_expire, alarm_callback, (unsigned long)p->pid);
        printf("[sys_alarm] pid=%d  fires in %us  (jiffies+%llu)\n",
               p->pid, seconds, (unsigned long long)p->alarm_expire);
    }

    return remaining;
}

/* ─────────────────────────────────────────────────────────────
 * sys_setitimer
 * ───────────────────────────────────────────────────────────── */
int sys_setitimer(Process *p, const ItimerVal *new_val, ItimerVal *old_val)
{
    if (!p || !new_val) return -1;

    if (old_val) {
        unsigned int rem = sys_alarm(p, 0); /* read current remaining */
        old_val->it_value.tv_sec  = rem;
        old_val->it_value.tv_usec = 0;
    }

    unsigned int secs = (unsigned int)new_val->it_value.tv_sec;
    sys_alarm(p, secs);

    printf("[sys_setitimer] pid=%d  interval=%lds\n",
           p->pid, (long)new_val->it_interval.tv_sec);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * POSIX timer: posix_timer_expire callback
 * ───────────────────────────────────────────────────────────── */
static void posix_timer_expire(unsigned long data)
{
    int idx = (int)data;
    if (idx < 0 || idx >= MAX_POSIX_TIMERS) return;
    PosixTimer *pt = &posix_timers[idx];

    printf("[posix_timer] expired id=%d  clock=%s\n",
           idx, pt->clock_id == UIOX_CLOCK_REALTIME ? "REALTIME" : "MONOTONIC");

    /* Count overruns if timer fires again before being read */
    pt->overruns++;

    /* Auto-reload if interval is set */
    if (pt->spec.it_interval.tv_sec > 0) {
        uint64_t interval_ticks =
            (uint64_t)pt->spec.it_interval.tv_sec * HZ;
        pt->node = timer_add(jiffies + interval_ticks,
                             posix_timer_expire, (unsigned long)idx);
    } else {
        pt->active = false;
    }
}

/* ─────────────────────────────────────────────────────────────
 * sys_timer_create
 * ───────────────────────────────────────────────────────────── */
int sys_timer_create(ClockId clock_id, PosixTimer **out_timer)
{
    ensure_pool();
    for (int i = 0; i < MAX_POSIX_TIMERS; i++) {
        if (!posix_timers[i].active) {
            posix_timers[i].clock_id = clock_id;
            posix_timers[i].overruns = 0;
            posix_timers[i].active   = false;
            posix_timers[i].node     = NULL;
            *out_timer               = &posix_timers[i];
            printf("[sys_timer_create] id=%d  clock=%s\n", i,
                   clock_id == UIOX_CLOCK_REALTIME ? "REALTIME" : "MONOTONIC");
            return i;
        }
    }
    return -1; /* no free slot */
}

/* ─────────────────────────────────────────────────────────────
 * sys_timer_settime
 * ───────────────────────────────────────────────────────────── */
int sys_timer_settime(PosixTimer *t, const ItimerVal *spec)
{
    if (!t || !spec) return -1;
    t->spec = *spec;

    if (t->node) { timer_del(t->node); t->node = NULL; }

    if (spec->it_value.tv_sec > 0) {
        uint64_t ticks = (uint64_t)spec->it_value.tv_sec * HZ;
        t->node   = timer_add(jiffies + ticks,
                              posix_timer_expire, (unsigned long)t->id);
        t->active = true;
        printf("[sys_timer_settime] id=%d  expires in %lds\n",
               t->id, (long)spec->it_value.tv_sec);
    } else {
        t->active = false;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_timer_gettime
 * ───────────────────────────────────────────────────────────── */
int sys_timer_gettime(PosixTimer *t, ItimerVal *out_spec)
{
    if (!t || !out_spec) return -1;
    *out_spec = t->spec;
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * sys_timer_getoverrun
 * ───────────────────────────────────────────────────────────── */
int sys_timer_getoverrun(PosixTimer *t)
{
    if (!t) return -1;
    int ov = (int)t->overruns;
    t->overruns = 0;
    printf("[sys_timer_getoverrun] id=%d  overruns=%d\n", t->id, ov);
    return ov;
}

/* ─────────────────────────────────────────────────────────────
 * sys_timer_delete
 * ───────────────────────────────────────────────────────────── */
int sys_timer_delete(PosixTimer *t)
{
    if (!t) return -1;
    if (t->node) { timer_del(t->node); timer_free(t->node); t->node = NULL; }
    t->active = false;
    printf("[sys_timer_delete] id=%d\n", t->id);
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * POSIX clock calls
 * ───────────────────────────────────────────────────────────── */
int sys_clock_gettime(ClockId id, XTime *out)
{
    if (!out) return -1;
    if (id == UIOX_CLOCK_REALTIME) {
        *out = xtime;
    } else {
        /* MONOTONIC: nanoseconds since boot */
        uint64_t ns     = timekeeping_monotonic_ns();
        out->tv_sec     = (int64_t)(ns / 1000000000ULL);
        out->tv_nsec    = (uint32_t)(ns % 1000000000ULL);
    }
    printf("[sys_clock_gettime] clock=%d  sec=%ld  nsec=%u\n",
           id, (long)out->tv_sec, out->tv_nsec);
    return 0;
}

int sys_clock_settime(ClockId id, const XTime *in)
{
    if (!in || id != UIOX_CLOCK_REALTIME) return -1;
    xtime = *in;
    printf("[sys_clock_settime] sec=%ld\n", (long)xtime.tv_sec);
    return 0;
}

int sys_clock_getres(ClockId id, XTime *out_res)
{
    (void)id;
    if (!out_res) return -1;
    /* Resolution = 1 tick = TICK_NSEC nanoseconds */
    out_res->tv_sec  = 0;
    out_res->tv_nsec = TICK_NSEC;
    printf("[sys_clock_getres] resolution=%u ns\n", out_res->tv_nsec);
    return 0;
}

int sys_clock_nanosleep(ClockId id, uint64_t nsecs)
{
    (void)id;
    printf("[sys_clock_nanosleep] sleeping %llu ns\n",
           (unsigned long long)nsecs);
    /* Simulation: just delay */
    ndelay((unsigned long)nsecs);
    return 0;
}
