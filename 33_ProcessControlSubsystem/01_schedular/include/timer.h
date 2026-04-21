#ifndef UIOX_TIMER_H
#define UIOX_TIMER_H

#include "sched_types.h"

/* ─────────────────────────────────────────────────────────────
 * Dynamic timer (timer_list equivalent)
 *
 * expires — jiffies value when the timer fires
 * fn      — callback function
 * data    — argument passed to callback
 * magic   — integrity sentinel (TIMER_MAGIC)
 * ───────────────────────────────────────────────────────────── */
typedef struct TimerNode {
    uint64_t          expires;
    void            (*fn)(unsigned long data);
    unsigned long     data;
    unsigned long     magic;
    bool              active;
    struct TimerNode *next;   /* intrusive linked list           */
} TimerNode;

/* ─────────────────────────────────────────────────────────────
 * Timer wheel (tvec_base_t equivalent)
 *
 * Five arrays of bucket lists (tv1..tv5), modelled as a simple
 * hash wheel on the low bits of 'expires'.
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    TimerNode   *tv1[TVEC_SIZE];   /* 0..255 ticks ahead          */
    TimerNode   *tv2[64];          /* 256..16383 ticks ahead       */
    TimerNode   *tv3[64];
    TimerNode   *tv4[64];
    TimerNode   *tv5[64];
    uint64_t     timer_jiffies;    /* last tick this wheel saw     */
    TimerNode   *running_timer;    /* currently executing timer    */
} TimerWheel;

/* ─────────────────────────────────────────────────────────────
 * Timer API
 * ───────────────────────────────────────────────────────────── */
void       timer_init(void);

/* Allocate and register a new dynamic timer */
TimerNode *timer_add(uint64_t expires_jiffies,
                     void (*fn)(unsigned long), unsigned long data);

/* Cancel a timer (deactivate; does not free) */
void       timer_del(TimerNode *t);

/*
 * Run all timers whose expires <= current jiffies.
 * Called by clock_tick on every hardware interrupt.
 */
void       timer_run(void);

/* Free a timer node returned by timer_add */
void       timer_free(TimerNode *t);

#endif /* UIOX_TIMER_H */
