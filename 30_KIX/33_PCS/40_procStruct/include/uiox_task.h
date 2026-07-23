/*
 * 30_KIX/33_PCS/40_procStruct/include/uiox_task.h
 *
 * UIOX process / task descriptor — unified type used by the scheduler,
 * memory manager, IPC, signal delivery, and syscall layers.
 *
 * Naming conventions (matches live 33_PCS codebase)
 * ───────────────────────────────────────────────────
 *  types      uiox_<noun>_t          e.g. uiox_task_t
 *  functions  uiox_<module>_<verb>() e.g. uiox_task_alloc()
 *  fields     p_<word>               e.g. p_pid, p_state
 *  globals    g_<word>               e.g. g_current
 *  macros     UIOX_<WORD>            e.g. UIOX_MAX_PIDS
 *  BSP types  uix_uintptr_t, uix_size_t, uix_pid_t  (from uix_types.h)
 *
 * Freestanding: no system headers — types from uix_types.h only.
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_TASK_H
#define UIOX_TASK_H

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Forward declarations ──────────────────────────────────────────── */
struct uiox_task;
struct uiox_mm_desc;

/* ── Task states ────────────────────────────────────────────────────── */
typedef enum {
    UIOX_TASK_RUNNING          = 0,   /* on run queue, executing         */
    UIOX_TASK_INTERRUPTIBLE    = 1,   /* sleeping; wakes on signal/event */
    UIOX_TASK_UNINTERRUPTIBLE  = 2,   /* sleeping; wakes on event only   */
    UIOX_TASK_ZOMBIE           = 3,   /* exited; waiting for wait()      */
    UIOX_TASK_STOPPED          = 4,   /* SIGSTOP / ptrace stopped        */
    UIOX_TASK_DEAD             = 5,   /* being torn down                 */
    UIOX_TASK_NEW              = 6,   /* allocated, not yet queued       */
} uiox_task_state_t;

/* ── Scheduling policy ──────────────────────────────────────────────── */
typedef enum {
    UIOX_SCHED_RR       = 0,   /* round-robin (default)               */
    UIOX_SCHED_RT       = 1,   /* fixed priority, preemptive          */
    UIOX_SCHED_IDLE     = 2,   /* only runs when nothing else ready   */
} uiox_sched_policy_t;

/* ── Arch context save area (opaque blob; arch layer casts to hwcontext_t) */
#define UIOX_ARCH_CTX_SIZE  512u
typedef struct {
    uix_uint8_t raw[UIOX_ARCH_CTX_SIZE];
} uiox_arch_ctx_t;

/* ── Signal state ───────────────────────────────────────────────────── */
typedef struct {
    uix_uint32_t s_pending;    /* bitmask of signals pending delivery  */
    uix_uint32_t s_blocked;    /* bitmask of masked signals            */
} uiox_sig_state_t;

/* ── File descriptor table stub ─────────────────────────────────────── */
#define UIOX_MAX_FDS  32u
typedef struct {
    uix_uintptr_t f_fds[UIOX_MAX_FDS];   /* NULL = slot unused          */
    uix_uint8_t   f_flags[UIOX_MAX_FDS];
} uiox_fd_table_t;

/* ── Main task descriptor ───────────────────────────────────────────── */
typedef struct uiox_task {
    /* ── Identity ─────────────────────────────────────────────────── */
    uix_pid_t           p_pid;          /* process ID                   */
    uix_pid_t           p_ppid;         /* parent process ID            */
    uix_uid_t           p_uid;          /* user ID                      */
    uix_gid_t           p_gid;          /* group ID                     */

    /* ── Scheduling ───────────────────────────────────────────────── */
    uiox_task_state_t   p_state;        /* current task state           */
    uiox_sched_policy_t p_policy;       /* scheduling policy            */
    int                 p_priority;     /* static priority  0=highest   */
    int                 p_dyn_prio;     /* dynamic (boosted) priority   */
    int                 p_time_slice;   /* ticks remaining this quantum */
    int                 p_quantum;      /* full quantum length (ticks)  */
    uix_uint64_t        p_runtime;      /* total CPU ticks consumed     */
    uix_uint64_t        p_sleep_until;  /* tick deadline for timed sleep*/

    /* ── Run-queue linkage ────────────────────────────────────────── */
    struct uiox_task   *p_next;
    struct uiox_task   *p_prev;

    /* ── Process tree ─────────────────────────────────────────────── */
    struct uiox_task   *p_parent;
    struct uiox_task   *p_first_child;
    struct uiox_task   *p_sibling;

    /* ── Arch context ─────────────────────────────────────────────── */
    uiox_arch_ctx_t     p_ctx;          /* saved registers on switch    */

    /* ── Memory ───────────────────────────────────────────────────── */
    struct uiox_mm_desc *p_mm;          /* NULL = kernel thread         */
    uix_uintptr_t       p_kstack;       /* kernel stack base (virtual)  */
    uix_uintptr_t       p_ksp;          /* saved kernel SP on switch    */
    uix_uintptr_t       p_usp;          /* saved user SP on switch      */

    /* ── Files ────────────────────────────────────────────────────── */
    uiox_fd_table_t     p_files;

    /* ── Signals ──────────────────────────────────────────────────── */
    uiox_sig_state_t    p_signals;

    /* ── Exit / accounting ────────────────────────────────────────── */
    int                 p_exit_code;
    uix_uint64_t        p_create_tick;
    uix_uint32_t        p_nr_switches;
} uiox_task_t;

/* ── Limits ─────────────────────────────────────────────────────────── */
#define UIOX_MAX_PIDS           4096u
#define UIOX_PID_IDLE           0u
#define UIOX_PID_INIT           1u
#define UIOX_PRIO_MIN           0
#define UIOX_PRIO_MAX           139
#define UIOX_PRIO_DEFAULT       100
#define UIOX_QUANTUM_DEFAULT    10

/* ── API ────────────────────────────────────────────────────────────── */
uiox_task_t *uiox_task_alloc(void);
void         uiox_task_free(uiox_task_t *t);
void         uiox_task_init(uiox_task_t *t, uix_pid_t pid,
                            uiox_task_t *parent, int priority);
void         uiox_task_set_state(uiox_task_t *t, uiox_task_state_t s);
uix_pid_t    uiox_task_new_pid(void);

/* ── Globals (defined in uiox_task.c) ───────────────────────────────── */
extern uiox_task_t  g_idle_task;
extern uiox_task_t *g_current;

#ifdef __cplusplus
}
#endif
#endif /* UIOX_TASK_H */
