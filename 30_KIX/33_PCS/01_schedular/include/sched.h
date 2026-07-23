/*
 * 30_KIX/33_PCS/01_schedular/include/sched.h
 *
 * Kernel scheduler interface — POSIX-compatible scheduling API.
 *
 * System headers removed (v2.0)
 * ──────────────────────────────
 *   WAS: #include <stdint.h>
 *   WAS: #include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
 *   WAS: #include "../../../../50_UIX/00_libs/00_uixlibs/PoStd/uix_sched.h"
 *   NOW: #include "uiox_klibc.h"   (on -I path via 33_PCS/include)
 *
 * Types previously pulled from 50_UIX headers are inlined here:
 *
 *   uix_pid_t        was: typedef int uix_pid_t       (uix_types.h)
 *   uix_sched_param_t was: struct { int sched_priority } (uix_sched.h)
 *
 * These are now defined as kernel-local types using uiox_klibc.h
 * primitives, removing the deep relative path dependency entirely.
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

/*
 * uiox_klibc.h provides: int8/16/32/64_t, uint*_t, bool, NULL, size_t.
 * It is found via -I<33_PCS/include> which is already in CINCLUDES.
 */
#include "../include/uiox_klibc.h"

/* ── Kernel-local type aliases ──────────────────────────────────────────
 * Previously imported from 50_UIX/00_libs/00_uixlibs/sys/uix_types.h
 * and PoStd/uix_sched.h. Inlined here so the kernel has zero dependency
 * on the user-space library tree.
 * ──────────────────────────────────────────────────────────────────────*/

/* uix_pid_t — process ID (POSIX pid_t equivalent) */
#ifndef UIX_PID_T_DEFINED
#define UIX_PID_T_DEFINED
typedef int  uix_pid_t;
#endif

/* uix_sched_param_t — scheduling parameter (POSIX sched_param equivalent) */
#ifndef UIX_SCHED_PARAM_T_DEFINED
#define UIX_SCHED_PARAM_T_DEFINED
typedef struct uix_sched_param {
    int sched_priority;
} uix_sched_param_t;
#endif

/* ── Scheduling policies ────────────────────────────────────────────── */
#define UIOX_SCHED_OTHER  0   /* default time-sharing                   */
#define UIOX_SCHED_FIFO   1   /* real-time FIFO                         */
#define UIOX_SCHED_RR     2   /* real-time round-robin                  */
#define UIOX_SCHED_BATCH  3   /* batch (Linux extension)                */
#define UIOX_SCHED_IDLE   5   /* idle-priority                          */

/* ── Process slot ───────────────────────────────────────────────────── */
#define MAX_PROCS   64
#define TIME_SLICE  10

typedef struct {
    int        sp_in_use;
    uix_pid_t  sp_pid;
    int        sp_policy;
    int        sp_priority;
    int        sp_ticks_left;
    int        sp_state;
} sched_proc_t;

/* ── Process states ─────────────────────────────────────────────────── */
#define PROC_READY    0
#define PROC_RUNNING  1
#define PROC_BLOCKED  2
#define PROC_ZOMBIE   3

/* ── Scheduler API ──────────────────────────────────────────────────── */
int        kernel_sched_setscheduler(uix_pid_t pid, int policy,
                                      const uix_sched_param_t *p);
int        kernel_sched_getscheduler(uix_pid_t pid);
int        kernel_sched_yield       (void);
int        sched_add_proc           (uix_pid_t pid, int policy, int priority);
int        sched_remove_proc        (uix_pid_t pid);
int        sched_tick               (void);
uix_pid_t  sched_next               (void);

#endif /* KERNEL_SCHED_H */
