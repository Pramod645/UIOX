/*
 * 30_KIX/33_PCS/01_schedular/include/uiox_kix_timer.h
 *
 * Dynamic timers — the timer wheel.
 *
 * Bach's Algorithm 2 step 2 is "adjust callout times; schedule callout
 * function if time elapsed".  The CALLOUT TABLE in clock.h handles the
 * delta-tick case; this is the general one — timers at absolute
 * deadlines, which is what an alarm and every POSIX timer are built on.
 *
 * The wheel has five tiers because a single bucket array cannot span
 * both "in 3 ticks" and "in an hour" without either a huge array or a
 * linear scan.  Cascading one tier into the next as time advances is the
 * standard solution, and it is what tvec_base_t does in Linux.
 *
 * ── no heap ─────────────────────────────────────────────────────────
 * Nodes come from a static pool in timer.c, because freestanding has no
 * calloc.  MAX_TIMER_NODES bounds how many timers can be live at once;
 * timer_add returns NULL when it is exhausted rather than wrapping.
 *
 * ── what this layer does NOT do ─────────────────────────────────────
 * It names no process type.  That is deliberate and was checked: the
 * port of 01_schedular to the merged 40_psa type left this file and
 * profiler.c untouched, because neither touches a Process.
 *
 * @version 2.0.0  @date 2026-09-29
 */
#ifndef UIOX_TIMER_H
#define UIOX_TIMER_H

#include "sched_types.h"

/* ── Dynamic timer ───────────────────────────────────────────────────
 *  expires — the jiffies value it fires at (ABSOLUTE, unlike a callout's
 *            relative delta)
 *  fn      — the callback
 *  data    — its argument; by convention a process pid or a pool index
 *  magic   — TIMER_MAGIC, so a freed node handed back as a callback
 *            argument is detectable rather than silently executed
 *  active  — timer_del clears this without unlinking; timer_run skips
 *            inactive nodes and unlinks them when it passes */
typedef struct TimerNode {
    uint64_t          expires;
    void            (*fn)(unsigned long data);
    unsigned long     data;
    unsigned long     magic;
    bool              active;
    struct TimerNode *next;   /* intrusive linked list */
} TimerNode;

/* ── Timer wheel ─────────────────────────────────────────────────────
 * tv1..tv5 are the five tiers: tv1 covers the next 256 ticks, tv2 the
 * next ~16k, and so on.  timer_jiffies records the last tick the wheel
 * processed, so timer_run knows which bucket the current time falls in.
 *
 * running_timer is the timer whose callback is executing — set so a
 * callback that deletes its own timer does not free the node it is
 * standing on. */
typedef struct {
    TimerNode   *tv1[TVEC_SIZE];   /* 0..255 ticks ahead        */
    TimerNode   *tv2[64];          /* 256..16383 ticks ahead    */
    TimerNode   *tv3[64];
    TimerNode   *tv4[64];
    TimerNode   *tv5[64];
    uint64_t     timer_jiffies;    /* last tick this wheel saw  */
    TimerNode   *running_timer;    /* currently executing timer */
} TimerWheel;

/* ── API ─────────────────────────────────────────────────────────────
 * Zero the wheel and prime the node pool.  Idempotent. */
void       timer_init(void);

/* Register a timer to fire at an absolute jiffies value.
 * Returns the node, or NULL when the pool is exhausted.  The caller
 * owns the node until it fires; timer_free reclaims it after. */
TimerNode *timer_add(uint64_t expires_jiffies,
                     void (*fn)(unsigned long), unsigned long data);

/* Deactivate a timer WITHOUT unlinking it.  It stays in the wheel until
 * timer_run passes it, which avoids mutating a list mid-traversal. */
void       timer_del(TimerNode *t);

/* Fire every timer in the current bucket whose deadline has passed.
 * Called from clock_tick on every hardware interrupt — Algorithm 2
 * step 2 is what makes this the tick handler's job. */
void       timer_run(void);

/* Return a fired timer's node to the static pool. */
void       timer_free(TimerNode *t);

#endif /* UIOX_TIMER_H */
