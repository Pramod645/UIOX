/*
 * 30_KIX/33_PCS/40_procStruct/src/uiox_task.c
 *
 * Task descriptor — static pool allocation, init, state transitions.
 *
 * @version 2.0.0  @date 2026-07-23
 */

#include "../include/uiox_task.h"

/* ── Static pool ────────────────────────────────────────────────────── */
static uiox_task_t  s_pool[UIOX_MAX_PIDS];
static uix_uint8_t  s_used[UIOX_MAX_PIDS];   /* 0 = free, 1 = in use   */
static uix_pid_t    s_next_pid = UIOX_PID_INIT;

/* ── Globals ────────────────────────────────────────────────────────── */
uiox_task_t  g_idle_task;
uiox_task_t *g_current = (uiox_task_t *)0;

/* ────────────────────────────────────────────────────────────────────
 * uiox_task_alloc — pop a zeroed slot from the static pool.
 * Returns NULL when the pool is exhausted.
 * ──────────────────────────────────────────────────────────────────── */
uiox_task_t *uiox_task_alloc(void)
{
    uix_uint32_t i;
    for (i = 0u; i < UIOX_MAX_PIDS; i++) {
        if (s_used[i] == 0u) {
            uix_uint8_t *p = (uix_uint8_t *)&s_pool[i];
            uix_size_t   n = sizeof(uiox_task_t);
            while (n--) { *p++ = 0u; }
            s_used[i] = 1u;
            return &s_pool[i];
        }
    }
    return (uiox_task_t *)0;
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_task_free — scrub and return a slot to the pool.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_task_free(uiox_task_t *t)
{
    uix_uint32_t i;
    if (!t) return;
    for (i = 0u; i < UIOX_MAX_PIDS; i++) {
        if (&s_pool[i] == t) {
            uix_uint8_t *p = (uix_uint8_t *)t;
            uix_size_t   n = sizeof(uiox_task_t);
            while (n--) { *p++ = 0u; }
            s_used[i] = 0u;
            return;
        }
    }
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_task_init — fill in a freshly allocated task descriptor.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_task_init(uiox_task_t *t, uix_pid_t pid,
                    uiox_task_t *parent, int priority)
{
    if (!t) return;
    t->p_pid         = pid;
    t->p_ppid        = parent ? parent->p_pid : 0u;
    t->p_state       = UIOX_TASK_NEW;
    t->p_policy      = UIOX_SCHED_RR;
    t->p_priority    = priority;
    t->p_dyn_prio    = priority;
    t->p_time_slice  = UIOX_QUANTUM_DEFAULT;
    t->p_quantum     = UIOX_QUANTUM_DEFAULT;
    t->p_parent      = parent;
    t->p_next        = (uiox_task_t *)0;
    t->p_prev        = (uiox_task_t *)0;
    t->p_first_child = (uiox_task_t *)0;
    t->p_sibling     = (uiox_task_t *)0;
    t->p_mm          = (struct uiox_mm_desc *)0;
    t->p_exit_code   = 0;
    t->p_nr_switches = 0u;
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_task_set_state — single choke-point for all state transitions.
 * ──────────────────────────────────────────────────────────────────── */
void uiox_task_set_state(uiox_task_t *t, uiox_task_state_t s)
{
    if (t) t->p_state = s;
}

/* ────────────────────────────────────────────────────────────────────
 * uiox_task_new_pid — allocate the next available PID.
 * ──────────────────────────────────────────────────────────────────── */
uix_pid_t uiox_task_new_pid(void)
{
    uix_pid_t pid = s_next_pid++;
    if (s_next_pid >= (uix_pid_t)UIOX_MAX_PIDS)
        s_next_pid = UIOX_PID_INIT + 1u;
    return pid;
}
