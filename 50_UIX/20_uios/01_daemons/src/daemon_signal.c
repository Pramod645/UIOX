#include "../include/daemon_signal.h"

/*
 * Static storage for the reread callback and daemon name.
 * Used by the signal handlers installed by the ST variant.
 */
static reread_fn_t  _reread_cb  = NULL;
static const char  *_daemon_cmd = "daemon";

/* ── Single-threaded signal handlers ─────────────────────── */

/*
 * _sighup_handler()
 *
 * Installed as the SIGHUP handler for single-threaded daemons
 * (Figure 13.8).
 *
 * SIGHUP is repurposed as "reload configuration":
 *   • Daemons have no controlling terminal.
 *   • They are either session leaders without a terminal or
 *     members of orphaned process groups.
 *   • Therefore SIGHUP will never arrive due to a terminal
 *     hangup — it is safe to reuse as a reload signal.
 *
 * The handler blocks SIGTERM while running to prevent races.
 */
static void _sighup_handler(int signo)
{
    (void)signo;
    syslog(LOG_INFO, "%s: re-reading configuration file",
           _daemon_cmd);
    if (_reread_cb)
        _reread_cb();
}

/*
 * _sigterm_handler()
 *
 * Installed as the SIGTERM handler for single-threaded daemons.
 * SIGTERM is the standard "please shut down gracefully" signal.
 * The handler blocks SIGHUP while running.
 */
static void _sigterm_handler(int signo)
{
    (void)signo;
    syslog(LOG_INFO, "%s: got SIGTERM; exiting", _daemon_cmd);
    exit(0);
}

/*
 * daemon_signals_init_st()
 *
 * Sets up signal handlers for a single-threaded daemon.
 *
 * For SIGTERM: the sa_mask includes SIGHUP, so SIGHUP is
 * blocked while SIGTERM is being handled (and vice versa).
 * This prevents a SIGHUP arriving in the middle of a reload
 * from being lost, and prevents SIGTERM from interrupting
 * an in-progress reload.
 */
void daemon_signals_init_st(reread_fn_t reread_fn,
                              const char *cmd)
{
    struct sigaction sa;

    _reread_cb  = reread_fn;
    _daemon_cmd = cmd ? cmd : "daemon";

    /* Install SIGTERM handler (blocks SIGHUP while running) */
    sa.sa_handler = _sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGHUP);     /* block SIGHUP */
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        syslog(LOG_ERR, "%s: can't catch SIGTERM: %s",
               cmd, strerror(errno));
        exit(1);
    }

    /* Install SIGHUP handler (blocks SIGTERM while running) */
    sa.sa_handler = _sighup_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);    /* block SIGTERM */
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        syslog(LOG_ERR, "%s: can't catch SIGHUP: %s",
               cmd, strerror(errno));
        exit(1);
    }
}

/* ── Multi-threaded signal handling thread ───────────────── */

/* Global signal mask exported for pthread_sigmask() */
sigset_t daemon_signal_mask;

/*
 * Arguments forwarded to the signal-handler thread.
 */
typedef struct {
    reread_fn_t  reread_fn;
    const char  *cmd;
} _thr_args_t;

/*
 * _signal_thread()
 *
 * Thread function for the MT variant (Figure 13.7).
 *
 * Runs an infinite loop calling sigwait() on the full signal
 * mask.  Because all signals are blocked in every thread,
 * only this thread will receive signals — via sigwait().
 *
 * This approach is safer than async-signal-safe handlers:
 *   • The thread body runs in full thread context.
 *   • No async-signal-safety restrictions.
 *   • No risk of missing a signal that arrives during handler.
 *
 * SIGHUP  → reload configuration.
 * SIGTERM → log and exit the entire process.
 * Others  → log an informational message.
 */
static void *_signal_thread(void *arg)
{
    _thr_args_t *a = (_thr_args_t *)arg;
    int err, signo;

    for (;;) {
        err = sigwait(&daemon_signal_mask, &signo);
        if (err != 0) {
            syslog(LOG_ERR, "sigwait failed: %s",
                   strerror(err));
            exit(1);
        }

        switch (signo) {
        case SIGHUP:
            syslog(LOG_INFO,
                   "%s: re-reading configuration file",
                   a->cmd);
            if (a->reread_fn)
                a->reread_fn();
            break;

        case SIGTERM:
            syslog(LOG_INFO,
                   "%s: got SIGTERM; exiting", a->cmd);
            exit(0);

        default:
            syslog(LOG_INFO,
                   "%s: unexpected signal %d",
                   a->cmd, signo);
            break;
        }
    }
    return NULL;
}

/*
 * daemon_signals_init_mt()
 *
 * Sets up signal handling for a multi-threaded daemon:
 *
 *   1. Fills daemon_signal_mask with all signals.
 *   2. Calls pthread_sigmask(SIG_BLOCK) to block them all
 *      in the calling thread.  Threads created after this
 *      call inherit the mask, so all threads block all signals.
 *   3. Creates a dedicated thread running _signal_thread()
 *      which uses sigwait() to receive signals one at a time.
 *
 * Caller must reset SIGHUP to SIG_DFL before calling this
 * (daemonize() leaves SIGHUP ignored from the second fork).
 */
void daemon_signals_init_mt(reread_fn_t reread_fn,
                              const char *cmd)
{
    int          err;
    pthread_t    tid;
    _thr_args_t *args;

    args = malloc(sizeof(*args));
    if (!args) {
        syslog(LOG_ERR, "%s: malloc failed", cmd);
        exit(1);
    }
    args->reread_fn = reread_fn;
    args->cmd       = cmd ? cmd : "daemon";

    /* Block all signals in this (and all future) threads */
    sigfillset(&daemon_signal_mask);
    err = pthread_sigmask(SIG_BLOCK, &daemon_signal_mask, NULL);
    if (err != 0) {
        syslog(LOG_ERR, "%s: SIG_BLOCK error: %s",
               cmd, strerror(err));
        exit(1);
    }

    /* Create the dedicated signal-handling thread */
    err = pthread_create(&tid, NULL, _signal_thread, args);
    if (err != 0) {
        syslog(LOG_ERR, "%s: can't create signal thread: %s",
               cmd, strerror(err));
        exit(1);
    }

    /* Detach — we don't need to join it */
    pthread_detach(tid);
}
