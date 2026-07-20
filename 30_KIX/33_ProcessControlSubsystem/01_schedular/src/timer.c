#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TimerWheel wheel;

/* ─────────────────────────────────────────────────────────────
 * timer_init
 * ───────────────────────────────────────────────────────────── */
void timer_init(void)
{
    memset(&wheel, 0, sizeof wheel);
    wheel.timer_jiffies = jiffies;
    printf("[timer] wheel initialised\n");
}

/* ─────────────────────────────────────────────────────────────
 * Internal: compute bucket index in tv1
 * ───────────────────────────────────────────────────────────── */
static unsigned int tv1_index(uint64_t expires)
{
    return (unsigned int)(expires & (TVEC_SIZE - 1));
}

/* ─────────────────────────────────────────────────────────────
 * timer_add
 * ───────────────────────────────────────────────────────────── */
TimerNode *timer_add(uint64_t expires_jiffies,
                     void (*fn)(unsigned long), unsigned long data)
{
    TimerNode *t = calloc(1, sizeof *t);
    if (!t) { perror("calloc"); return NULL; }

    t->expires = expires_jiffies;
    t->fn      = fn;
    t->data    = data;
    t->magic   = TIMER_MAGIC;
    t->active  = true;

    /*
     * Simple placement: timers expiring within the next TVEC_SIZE
     * ticks go into tv1; further ones would cascade through tv2..tv5.
     * For this simulation we always use tv1.
     */
    unsigned int idx = tv1_index(expires_jiffies);
    t->next         = wheel.tv1[idx];
    wheel.tv1[idx]  = t;

    printf("[timer] added: expires=%llu  idx=%u\n",
           (unsigned long long)expires_jiffies, idx);
    return t;
}

/* ─────────────────────────────────────────────────────────────
 * timer_del
 * ───────────────────────────────────────────────────────────── */
void timer_del(TimerNode *t)
{
    if (!t) return;
    t->active = false;
    printf("[timer] cancelled: expires=%llu\n",
           (unsigned long long)t->expires);
}

/* ─────────────────────────────────────────────────────────────
 * timer_run
 * Called each tick; fires all expired timers in the current bucket.
 * ───────────────────────────────────────────────────────────── */
void timer_run(void)
{
    unsigned int idx = tv1_index(jiffies);
    TimerNode   *t   = wheel.tv1[idx];
    TimerNode   *prev = NULL;

    while (t) {
        TimerNode *next = t->next;

        if (t->active && t->magic == TIMER_MAGIC &&
            t->expires <= jiffies) {
            printf("  [timer] firing: expires=%llu  data=%lu\n",
                   (unsigned long long)t->expires, t->data);
            wheel.running_timer = t;
            t->fn(t->data);
            wheel.running_timer = NULL;
            t->active = false;

            /* Remove from list */
            if (prev) prev->next       = next;
            else      wheel.tv1[idx]   = next;
        } else {
            prev = t;
        }
        t = next;
    }

    wheel.timer_jiffies = jiffies;
}

/* ─────────────────────────────────────────────────────────────
 * timer_free
 * ───────────────────────────────────────────────────────────── */
void timer_free(TimerNode *t)
{
    if (t) free(t);
}
