#ifndef __UIX_UIOS__H
#define __UIX_UIOS__H

#include "../../40_SCIX/uix_sys.h"

#include "sys/uix_types.h"          /* uix_uint8_t, uix_uint32_t, uix_pid_t */

#define UIOS_MAX_APPS 10

/* ── restart policy ──────────────────────────────────────────────────
 * What init does when a service exits.  The distinction is the whole
 * reason a policy field exists: a one-shot test app that returns 0 is
 * finished, while a server that returns at all has died. */
typedef enum {
    UIOS_RUN_ONCE   = 0,   /* start, never restart — a test or a job   */
    UIOS_RESTART    = 1,   /* restart on any exit                      */
    UIOS_RESTART_CR = 2    /* restart only on non-zero exit (crash)    */
} uios_restart_t;

/* ── readiness ───────────────────────────────────────────────────────
 * Some services must exist before others can work: a fileserver needs
 * the root mounted, a netserver needs the network table.  A dependent
 * is not started until its prerequisite has been started and has
 * survived UIOS_READY_TICKS passes of the reaper loop — that is a
 * liveness check, not a proof of readiness, and the difference matters:
 * the alternative (ask the service) needs a channel that does not
 * exist yet. */
#define UIOS_READY_TICKS 2

typedef struct {
    const char     *ua_name;
    const char     *ua_path;
    uix_int32_t             ua_policy;       /* sched policy for setscheduler    */
    uix_int32_t             ua_priority;
    uios_restart_t  ua_restart;
    uix_uint8_t         ua_dep;          /* index of prerequisite, 0xFF none */
    uix_pid_t       ua_pid;          /* runtime: -1 idle, 0 reaped       */
    uix_int32_t             ua_exit_code;    /* runtime: last status collected   */
    uix_uint32_t        ua_restarts;     /* runtime: how many times restarted*/
    uix_uint32_t        ua_ticks;        /* runtime: passes since started    */
} uios_app_t;

extern uios_app_t g_apps[UIOS_MAX_APPS];

/* 0xFF in the dependency field: "no prerequisite". */
#define UIOS_NO_DEP  (-1)

void uios_init        (void);
void uios_launch_apps (void);
void uios_run         (void);
void uios_shutdown    (void);

#endif /* __UIX_UIOS__H */
