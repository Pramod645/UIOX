#ifndef UIOX_SYSCALL_TIME_H
#define UIOX_SYSCALL_TIME_H

#include "sched_types.h"
#include "timer.h"

/* ─────────────────────────────────────────────────────────────
 * timeval — microsecond resolution wall-clock snapshot
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    int64_t  tv_sec;
    uint32_t tv_usec;
} TimeVal;

/* ─────────────────────────────────────────────────────────────
 * itimerval — interval timer (setitimer / alarm)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    TimeVal it_interval;   /* reload interval after expiry       */
    TimeVal it_value;      /* time until next expiry             */
} ItimerVal;

/* ─────────────────────────────────────────────────────────────
 * POSIX clock IDs
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    UIOX_CLOCK_REALTIME  = 0,  /* wall-clock time (= xtime)       */
    UIOX_CLOCK_MONOTONIC = 1   /* monotonic, no warp              */
} ClockId;

/* ─────────────────────────────────────────────────────────────
 * POSIX timer handle
 * ───────────────────────────────────────────────────────────── */
#define MAX_POSIX_TIMERS 16

typedef struct {
    ClockId    clock_id;
    ItimerVal  spec;
    uint64_t   overruns;
    bool       active;
    TimerNode *node;         /* underlying dynamic timer           */
    int        id;
} PosixTimer;

/* ─────────────────────────────────────────────────────────────
 * System call API (user-space analogues)
 * ───────────────────────────────────────────────────────────── */

/* time() — seconds since epoch */
int64_t  sys_time(void);

/* gettimeofday() — seconds + microseconds since epoch */
int      sys_gettimeofday(TimeVal *tv);

/* adjtimex() — adjust kernel clock parameters */
int      sys_adjtimex(int64_t delta_sec, int32_t delta_nsec);

/* setitimer() — set interval timer for a process */
int      sys_setitimer(Process *p, const ItimerVal *new_val,
                        ItimerVal *old_val);

/* alarm() — set one-shot alarm in seconds */
unsigned int sys_alarm(Process *p, unsigned int seconds);

/* ── POSIX timer system calls ─────────────────────────────── */
int  sys_timer_create(ClockId clock_id, PosixTimer **out_timer);
int  sys_timer_settime(PosixTimer *t, const ItimerVal *spec);
int  sys_timer_gettime(PosixTimer *t, ItimerVal *out_spec);
int  sys_timer_getoverrun(PosixTimer *t);
int  sys_timer_delete(PosixTimer *t);

int  sys_clock_gettime(ClockId id, XTime *out);
int  sys_clock_settime(ClockId id, const XTime *in);
int  sys_clock_getres(ClockId id, XTime *out_res);
int  sys_clock_nanosleep(ClockId id, uint64_t nsecs);

#endif /* UIOX_SYSCALL_TIME_H */
