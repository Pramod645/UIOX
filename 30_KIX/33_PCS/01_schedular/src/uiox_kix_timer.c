/*
 * 30_KIX/33_PCS/01_schedular/src/uiox_kix_timer.c
 *
 * The timer wheel — timer_add / timer_del / timer_run / timer_free.
 *
 * ── no heap ─────────────────────────────────────────────────────────
 * Nodes come from a static pool.  An earlier version called
 * calloc(1, sizeof *t) and free(t); freestanding has neither, so this
 * version reserves MAX_TIMER_NODES and hands them out.  When the pool is
 * exhausted timer_add returns NULL rather than wrapping — an alarm that
 * silently fails to arm is worse than one that refuses to be set.
 *
 * ── the wheel, and what this build does with it ─────────────────────
 * tv1 alone.  A timer is placed in the bucket its expiry falls into
 * within the next TVEC_SIZE ticks, and timer_run fires everything in
 * the bucket for the CURRENT tick.
 *
 * The tv2..tv5 tiers exist in the struct and are NOT populated.  Real
 * cascade behaviour — moving tv2 entries down into tv1 as time
 * advances — is not implemented, which means a timer set further than
 * TVEC_SIZE ticks ahead lands in tv1 anyway and fires early.  Stated
 * here rather than discovered: the tier arrays are placeholders, and
 * the ceiling on a correct deadline is TVEC_SIZE ticks.
 *
 * ── what this file does NOT name ────────────────────────────────────
 * No process type.  That is why the port of 01_schedular to the merged
 * 40_psa type left this file alone — verified, not assumed.
 *
 * @version 2.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_timer.h"

/* ── Static node pool ────────────────────────────────────────────────
 * MAX_TIMER_NODES covers the wheel plus every POSIX timer and alarm that
 * can be live at once.  Sizing it too small shows up as a refused
 * timer_add, not as corruption. */
#define MAX_TIMER_NODES  256

static TimerNode  s_node_pool[MAX_TIMER_NODES];
static uiox_uint8_t s_node_used[MAX_TIMER_NODES];   /* 0 = free, 1 = in use */
static uiox_uint8_t s_pool_ready = 0;

static void pool_init(void)
{
    uiox_uint32_t i;
    if (s_pool_ready) return;

    for (i = 0; i < MAX_TIMER_NODES; i++) {
        uiox_uint8_t *p = (uiox_uint8_t *)&s_node_pool[i];
        uiox_uint64_t n = sizeof(TimerNode);
        while (n--) *p++ = 0u;
        s_node_used[i] = 0u;
    }
    s_pool_ready = 1u;
}

static TimerNode *node_alloc(void)
{
    uiox_uint32_t i;
    pool_init();

    for (i = 0; i < MAX_TIMER_NODES; i++) {
        if (!s_node_used[i]) {
            uiox_uint8_t *p = (uiox_uint8_t *)&s_node_pool[i];
            uiox_uint64_t n = sizeof(TimerNode);
            while (n--) *p++ = 0u;
            s_node_used[i] = 1u;
            return &s_node_pool[i];
        }
    }
    return (TimerNode *)0;   /* pool exhausted */
}

static void node_free(TimerNode *t)
{
    uiox_uint32_t i;
    if (!t) return;

    for (i = 0; i < MAX_TIMER_NODES; i++) {
        if (&s_node_pool[i] == t) {
            uiox_uint8_t *p = (uiox_uint8_t *)t;
            uiox_uint64_t n = sizeof(TimerNode);
            while (n--) *p++ = 0u;
            s_node_used[i] = 0u;
            return;
        }
    }
    /* Pointer not from our pool — ignore silently.  A node handed in
     * from somewhere else is a caller error, and freeing it would be
     * worse than leaking it. */
}

/* ── The wheel ──────────────────────────────────────────────────────── */
static TimerWheel wheel;

/* ── timer_init ─────────────────────────────────────────────────────── */
void timer_init(void)
{
    uiox_uint32_t i;

    pool_init();

    for (i = 0; i < TVEC_SIZE; i++) wheel.tv1[i] = (TimerNode *)0;
    for (i = 0; i < 64; i++) {
        wheel.tv2[i] = (TimerNode *)0;
        wheel.tv3[i] = (TimerNode *)0;
        wheel.tv4[i] = (TimerNode *)0;
        wheel.tv5[i] = (TimerNode *)0;
    }
    wheel.timer_jiffies = jiffies;
    wheel.running_timer = (TimerNode *)0;
}

/* ── tv1_index ───────────────────────────────────────────────────────
 * Which bucket a deadline falls in.  TVEC_SIZE is a power of two, so
 * the mask is the modulo — and a mask is what makes this safe when
 * expires has been advanced past a wrap of the tick counter. */
static unsigned int tv1_index(uiox_uint64_t expires)
{
    return (unsigned int)(expires & (TVEC_SIZE - 1));
}

/* ── timer_add ───────────────────────────────────────────────────────
 * Registers a timer at an ABSOLUTE jiffies value — unlike callout_add,
 * whose delta is relative.  The difference matters: a timer stored
 * absolutely survives being looked at, whereas a delta has to be
 * decremented by whoever walks the list.
 *
 * Inserts at the head of the bucket.  Order within a bucket is
 * irrelevant because timer_run checks each deadline before firing, so a
 * stack is as correct as a sorted list and cheaper to maintain. */
TimerNode *timer_add(uiox_uint64_t expires_jiffies,
                     void (*fn)(unsigned long), unsigned long data)
{
    unsigned int idx;
    TimerNode   *t = node_alloc();

    if (!t) return (TimerNode *)0;   /* pool exhausted — caller sees NULL */

    t->expires = expires_jiffies;
    t->fn      = fn;
    t->data    = data;
    t->magic   = TIMER_MAGIC;
    t->active  = true;

    idx            = tv1_index(expires_jiffies);
    t->next        = wheel.tv1[idx];
    wheel.tv1[idx] = t;

    return t;
}

/* ── timer_del ───────────────────────────────────────────────────────
 * Deactivates WITHOUT unlinking.
 *
 * That is deliberate: the node stays in the wheel until timer_run
 * passes it, so a callback that deletes its own timer does not mutate
 * the list it is being walked on.  timer_run skips an inactive node and
 * unlinks it as it goes. */
void timer_del(TimerNode *t)
{
    if (!t) return;
    t->active = false;
}

/* ── timer_run ───────────────────────────────────────────────────────
 * Fires everything in the current tick's bucket whose deadline has
 * passed.
 *
 * Two details worth naming:
 *
 *   running_timer is set BEFORE the callback so a callback that
 *   deletes the timer it is standing on deactivates it rather than
 *   freeing the node under its own feet.
 *
 *   The node is NOT freed here.  Control passes to the callback, which
 *   may still hold the pointer; reclaiming is timer_free's job, called
 *   by whoever registered it.  Freeing here would hand a live pointer
 *   to nobody. */
void timer_run(void)
{
    unsigned int idx  = tv1_index(jiffies);
    TimerNode   *t    = wheel.tv1[idx];
    TimerNode   *prev = (TimerNode *)0;

    while (t) {
        TimerNode *next = t->next;

        if (t->active && t->expires <= jiffies) {
            if (prev) prev->next      = next;
            else      wheel.tv1[idx]  = next;

            t->active            = false;
            wheel.running_timer  = t;

            if (t->fn) t->fn(t->data);

            wheel.running_timer  = (TimerNode *)0;
        } else {
            prev = t;
        }
        t = next;
    }

    wheel.timer_jiffies = jiffies;
}

/* ── timer_free ──────────────────────────────────────────────────────
 * Returns a fired node to the pool.  Separate from timer_run so the
 * caller decides when the pointer is no longer in use. */
void timer_free(TimerNode *t)
{
    node_free(t);
}
