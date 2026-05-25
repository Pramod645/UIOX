/**
 * @file    standards_demo.c
 * @brief   Demonstrate pathconf + sysconf queries using the UIOX constants.
 *
 * Build:   cc -o standards_demo standards_demo.c
 * Run:     ./standards_demo [path]
 *          Defaults to querying "/" if no path argument is given.
 *
 * @date    2026-05-25
 */

#include <stdio.h>
#include <string.h>
#include "pathconf_constants.h"
#include "sysconf_constants.h"

#include "uiox_pathconf.h"
#include "uiox_sysconf.h"

/* -------------------------------------------------------------------------
 * Dump every pathconf value for a given path
 * ---------------------------------------------------------------------- */
static void dump_pathconf(const char *path)
{
    printf("\n=== pathconf(\"%s\", ...) ===\n\n", path);

    /* Size / name / link limits */
    pathconf_print(path, PC_FILESIZEBITS,       "FILESIZEBITS      ");
    pathconf_print(path, PC_LINKMAX,            "LINK_MAX          ");
    pathconf_print(path, PC_MAXCANON,           "MAX_CANON         ");
    pathconf_print(path, PC_MAXINPUT,           "MAX_INPUT         ");
    pathconf_print(path, PC_NAMEMAX,            "NAME_MAX          ");
    pathconf_print(path, PC_PATHMAX,            "PATH_MAX          ");
    pathconf_print(path, PC_PIPEBUF,            "PIPE_BUF          ");
    pathconf_print(path, PC_SYMLINKMAX,         "SYMLINK_MAX       ");
    //pathconf_print(path, PC_TIMESTAMPRESOLUTION,"TIMESTAMP_RES (ns)");

    /* Behaviour flags */
    pathconf_print(path, PC_CHOWNRESTRICTED,    "CHOWN_RESTRICTED  ");
    pathconf_print(path, PC_NOTRUNC,            "NO_TRUNC          ");
    pathconf_print(path, PC_VDISABLE,           "VDISABLE          ");
    pathconf_print(path, PC_ASYNCIO,            "ASYNC_IO          ");
    pathconf_print(path, PC_PRIOIO,             "PRIO_IO           ");
    pathconf_print(path, PC_SYNCIO,             "SYNC_IO           ");
    pathconf_print(path, PC_2_SYMLINKS,         "2_SYMLINKS        ");
}

/* -------------------------------------------------------------------------
 * Dump every sysconf limit/flag
 * ---------------------------------------------------------------------- */
static void dump_sysconf(void)
{
    printf("\n=== sysconf(...) — Resource Limits ===\n\n");

    /* AIO — deferred, but query anyway */
    sysconf_print(SC_AIO_LISTIO_MAX,       "AIO_LISTIO_MAX     [ch.14]");
    sysconf_print(SC_AIO_MAX,              "AIO_MAX            [ch.14]");
    sysconf_print(SC_AIO_PRIO_DELTA_MAX,   "AIO_PRIO_DELTA_MAX [ch.14]");

    /* Process / argument limits */
    sysconf_print(SC_ARG_MAX,              "ARG_MAX            ");
    sysconf_print(SC_ATEXIT_MAX,           "ATEXIT_MAX         ");
    //sysconf_print(SC_CHARCLASS_NAME_MAX,   "CHARCLASS_NAME_MAX ");
    sysconf_print(SC_CHILD_MAX,            "CHILD_MAX          ");
    sysconf_print(SC_CLK_TCK,             "CLK_TCK            ");
    sysconf_print(SC_COLL_WEIGHTS_MAX,     "COLL_WEIGHTS_MAX   ");
    sysconf_print(SC_DELAYTIMER_MAX,       "DELAYTIMER_MAX     ");
    sysconf_print(SC_HOST_NAME_MAX,        "HOST_NAME_MAX      ");
    sysconf_print(SC_IOV_MAX,              "IOV_MAX            ");
    sysconf_print(SC_LINE_MAX,             "LINE_MAX           ");
    sysconf_print(SC_LOGIN_NAME_MAX,       "LOGIN_NAME_MAX     ");
    sysconf_print(SC_NGROUPS_MAX,          "NGROUPS_MAX        ");
    sysconf_print(SC_GETGR_R_SIZE_MAX,     "GETGR_R_SIZE_MAX   ");
    sysconf_print(SC_GETPW_R_SIZE_MAX,     "GETPW_R_SIZE_MAX   ");
    sysconf_print(SC_OPEN_MAX,             "OPEN_MAX           ");
    sysconf_print(SC_PAGESIZE,             "PAGESIZE (bytes)   ");

    /* Thread limits — deferred */
    sysconf_print(SC_THREAD_DESTRUCTOR_ITERATIONS, "THREAD_DESTR_ITER  [ch.12]");
    sysconf_print(SC_THREAD_KEYS_MAX,      "THREAD_KEYS_MAX    [ch.12]");
    sysconf_print(SC_THREAD_STACK_MIN,     "THREAD_STACK_MIN   [ch.12]");
    sysconf_print(SC_THREAD_THREADS_MAX,   "THREAD_THREADS_MAX [ch.12]");

    /* Signal / IPC limits */
    sysconf_print(SC_RTSIG_MAX,            "RTSIG_MAX          ");
    sysconf_print(SC_SEM_NSEMS_MAX,        "SEM_NSEMS_MAX      ");
    sysconf_print(SC_SEM_VALUE_MAX,        "SEM_VALUE_MAX      ");
    sysconf_print(SC_SIGQUEUE_MAX,         "SIGQUEUE_MAX       ");
    sysconf_print(SC_STREAM_MAX,           "STREAM_MAX         ");
    sysconf_print(SC_SYMLOOP_MAX,          "SYMLOOP_MAX        ");
    sysconf_print(SC_TIMER_MAX,            "TIMER_MAX          ");
    sysconf_print(SC_TTY_NAME_MAX,         "TTY_NAME_MAX       ");
    sysconf_print(SC_TZNAME_MAX,           "TZNAME_MAX         ");

    printf("\n=== sysconf(...) — POSIX Feature Flags ===\n\n");

    sysconf_print(SC_ADVISORY_INFO,           "ADVISORY_INFO         ");
    sysconf_print(SC_ASYNCHRONOUS_IO,         "ASYNCHRONOUS_IO    [R]");
    sysconf_print(SC_BARRIERS,                "BARRIERS           [R]");
    sysconf_print(SC_CLOCK_SELECTION,         "CLOCK_SELECTION    [R]");
    sysconf_print(SC_CPUTIME,                 "CPUTIME               ");
    sysconf_print(SC_FSYNC,                   "FSYNC                 ");
    sysconf_print(SC_IPV6,                    "IPV6                  ");
    sysconf_print(SC_JOB_CONTROL,             "JOB_CONTROL           ");
    sysconf_print(SC_MAPPED_FILES,            "MAPPED_FILES       [R]");
    sysconf_print(SC_MEMLOCK,                 "MEMLOCK               ");
    sysconf_print(SC_MEMLOCK_RANGE,           "MEMLOCK_RANGE         ");
    sysconf_print(SC_MEMORY_PROTECTION,       "MEMORY_PROTECTION  [R]");
    sysconf_print(SC_MESSAGE_PASSING,         "MESSAGE_PASSING       ");
    sysconf_print(SC_MONOTONIC_CLOCK,         "MONOTONIC_CLOCK       ");
    sysconf_print(SC_PRIORITIZED_IO,          "PRIORITIZED_IO        ");
    sysconf_print(SC_PRIORITY_SCHEDULING,     "PRIORITY_SCHEDULING   ");
    sysconf_print(SC_RAW_SOCKETS,             "RAW_SOCKETS           ");
    sysconf_print(SC_READER_WRITER_LOCKS,     "READER_WRITER_LOCKS[R]");
    sysconf_print(SC_REALTIME_SIGNALS,        "REALTIME_SIGNALS   [R]");
    sysconf_print(SC_REGEXP,                  "REGEXP                ");
    sysconf_print(SC_SAVED_IDS,               "SAVED_IDS             ");
    sysconf_print(SC_SEMAPHORES,              "SEMAPHORES         [R]");
    sysconf_print(SC_SHARED_MEMORY_OBJECTS,   "SHARED_MEM_OBJECTS    ");
    sysconf_print(SC_SHELL,                   "SHELL                 ");
    sysconf_print(SC_SPAWN,                   "SPAWN                 ");
    sysconf_print(SC_SPIN_LOCKS,              "SPIN_LOCKS         [R]");
    sysconf_print(SC_SPORADIC_SERVER,         "SPORADIC_SERVER       ");
    sysconf_print(SC_SYNCHRONIZED_IO,         "SYNCHRONIZED_IO       ");

    /* Thread options — deferred */
    sysconf_print(SC_THREAD_ATTR_STACKADDR,      "THREAD_STACKADDR   [ch.12]");
    sysconf_print(SC_THREAD_ATTR_STACKSIZE,      "THREAD_STACKSIZE   [ch.12]");
    sysconf_print(SC_THREAD_CPUTIME,             "THREAD_CPUTIME     [ch.12]");
    sysconf_print(SC_THREAD_PRIO_INHERIT,        "THREAD_PRIO_INH    [ch.12]");
    sysconf_print(SC_THREAD_PRIORITY_SCHEDULING, "THREAD_PRIO_SCHED  [ch.12]");
    sysconf_print(SC_THREAD_PROCESS_SHARED,      "THREAD_PROC_SHR    [ch.12]");
    //sysconf_print(SC_THREAD_ROBUST_PRIO_INHERIT, "THREAD_ROB_INH     [ch.12]");
    //sysconf_print(SC_THREAD_ROBUST_PRIO_PROTECT, "THREAD_ROB_PROT    [ch.12]");
    sysconf_print(SC_THREAD_SAFE_FUNCTIONS,      "THREAD_SAFE_FN  [R][ch.12]");
    sysconf_print(SC_THREAD_SPORADIC_SERVER,     "THREAD_SPORADIC    [ch.12]");
    sysconf_print(SC_THREADS,                    "THREADS         [R][ch.12]");

    sysconf_print(SC_TIMEOUTS,              "TIMEOUTS           [R]");
    sysconf_print(SC_TIMERS,                "TIMERS             [R]");
    sysconf_print(SC_TYPED_MEMORY_OBJECTS,  "TYPED_MEMORY_OBJ      ");
    sysconf_print(SC_VERSION,               "POSIX_VERSION         ");

    /* Data model — deferred */
    //sysconf_print(SC_V7_ILP32_OFF32,  "V7_ILP32_OFF32     [ch.3]");
    //sysconf_print(SC_V7_ILP32_OFFBIG, "V7_ILP32_OFFBIG    [ch.3]");
    //sysconf_print(SC_V7_LP64_OFF64,   "V7_LP64_OFF64      [ch.3]");
    //sysconf_print(SC_V7_LPBIG_OFFBIG, "V7_LPBIG_OFFBIG    [ch.3]");

    printf("\n=== sysconf(...) — X/Open (XSI) Flags ===\n\n");

    sysconf_print(SC_XOPEN_CRYPT,             "XOPEN_CRYPT           ");
    sysconf_print(SC_XOPEN_ENH_I18N,          "XOPEN_ENH_I18N        ");
    sysconf_print(SC_XOPEN_REALTIME,          "XOPEN_REALTIME        ");
    sysconf_print(SC_XOPEN_REALTIME_THREADS,  "XOPEN_REALTIME_THR    ");
    sysconf_print(SC_XOPEN_SHM,               "XOPEN_SHM             ");
    sysconf_print(SC_XOPEN_STREAMS,           "XOPEN_STREAMS [OBSOL] ");
    sysconf_print(SC_XOPEN_UNIX,              "XOPEN_UNIX            ");
    //sysconf_print(SC_XOPEN_UUCP,              "XOPEN_UUCP            ");
    sysconf_print(SC_XOPEN_VERSION,           "XOPEN_VERSION         ");
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/";

    printf("UIOX POSIX Standards Probe\n");
    printf("Platform : %s\n", PLATFORM);
    printf("Query path: %s\n", path);

    dump_pathconf(path);
    dump_sysconf();

    return 0;
}
