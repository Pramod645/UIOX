/*
 * 50_UIX/20_uios/uios_apps.c
 *
 * The service table — DATA, not code.
 *
 * Separated from uix_uios.c on purpose: init exists so that "what boots"
 * is a decision you can change without recompiling the supervisor.
 *
 * Keep this file free of logic.  If a scheduling policy or a restart
 * rule needs a conditional here, the split has already failed.
 *
 * ── field order ─────────────────────────────────────────────────────
 *   name · path · policy · priority · restart · dependency · pid ·
 *   exit_code · restarts · ticks
 *
 * The last four are runtime state, reset by uios_init().  They are
 * written here only so the initialiser is complete; do not read them as
 * configuration.
 *
 * ── paths ───────────────────────────────────────────────────────────
 * ABSOLUTE, in the mounted root.  The earlier "/../21_apps_/app_*"
 * spelling reached into the build tree via the process cwd, which
 * resolves to nothing once the kernel has mounted a real volume.  This
 * still needs the volume to actually carry /bin/app_*, which is the
 * same open question as /bin/uios itself.
 *
 * ── order ───────────────────────────────────────────────────────────
 * Array order is START order for everything with no dependency.  A
 * dependent is held until its prerequisite has survived
 * UIOS_READY_TICKS passes — see uios_run.
 */
#include "uix_uios.h"

uios_app_t g_apps[UIOS_MAX_APPS] = {
  /* name        path                  pol pri  restart          dep              pid  rc  rst tck */
    { "logger",  "/bin/app_logger",      0,  0, UIOS_RUN_ONCE,    UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "filesvr", "/bin/app_fileserver",  2,  5, UIOS_RESTART_CR,  UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "netsvr",  "/bin/app_netserver",   2,  5, UIOS_RESTART_CR,  UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "fstest",  "/bin/app_fstest",      0,  0, UIOS_RUN_ONCE,    1,               -1, -1, 0, 0 },
    { "ipctest", "/bin/app_ipctest",     0,  0, UIOS_RUN_ONCE,    2,               -1, -1, 0, 0 },
    { "schedtest","/bin/app_scheduler",  1, 10, UIOS_RUN_ONCE,    UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "memtest", "/bin/app_memtest",     0,  0, UIOS_RUN_ONCE,    UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "sigtest", "/bin/app_sigtest",     0,  0, UIOS_RUN_ONCE,    UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "clocktest","/bin/app_clocktest",  0,  0, UIOS_RUN_ONCE,    UIOS_NO_DEP,     -1, -1, 0, 0 },
    { "shell",   "/bin/app_shell",       0,  0, UIOS_RESTART_CR,  UIOS_NO_DEP,     -1, -1, 0, 0 },
};
