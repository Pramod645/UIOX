#ifndef DAEMON_SIGNAL_H
#define DAEMON_SIGNAL_H

/*
 * daemon_signal.h
 *
 * Signal handling for daemon processes — based on APUE
 * Sections 13.6 (Figures 13.7 and 13.8).
 *
 * Daemons conventionally re-read their configuration file
 * when they receive SIGHUP, because:
 *   • Daemons have no controlling terminal and cannot
 *     receive SIGHUP from a terminal hangup.
 *   • The signal is therefore "free" and safe to reuse as
 *     a "reload config" notification.
 *
 * Two implementation strategies are provided:
 *
 *   Single-threaded:
 *     Install sigaction handlers for SIGHUP and SIGTERM
 *     (Figure 13.8).
 *
 *   Multi-threaded:
 *     Block all signals in all threads, then dedicate one
 *     thread to call sigwait() in a loop (Figure 13.7).
 *     This avoids async-signal-safety concerns.
 */

#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

/*
 * Callback type for configuration reload.
 * The daemon provides its own implementation.
 */
typedef void (*reread_fn_t)(void);

/* ── Single-threaded signal setup (Figure 13.8) ─────────── */

/*
 * daemon_signals_init_st()
 *
 * Installs signal handlers for a single-threaded daemon:
 *   SIGHUP  → calls reread_fn to reload configuration.
 *   SIGTERM → logs a message and calls exit(0).
 *
 * The handlers block each other while running to prevent
 * races (SIGHUP handler blocks SIGTERM and vice versa).
 *
 * @param reread_fn  Function to call when SIGHUP arrives.
 * @param cmd        Daemon name (used in syslog messages).
 */
void daemon_signals_init_st(reread_fn_t reread_fn,
                             const char *cmd);

/* ── Multi-threaded signal setup (Figure 13.7) ──────────── */

/*
 * daemon_signals_init_mt()
 *
 * Sets up signal handling for a multi-threaded daemon:
 *   1. Blocks ALL signals in the calling thread (which
 *      propagates to all subsequently created threads).
 *   2. Spawns a dedicated signal-handler thread that calls
 *      sigwait() in a loop waiting for SIGHUP / SIGTERM.
 *   3. On SIGHUP  → calls reread_fn.
 *   4. On SIGTERM → logs and exits.
 *
 * Must be called AFTER daemonize() and AFTER restoring the
 * SIGHUP default disposition (see note below).
 *
 * @param reread_fn  Function to call when SIGHUP arrives.
 * @param cmd        Daemon name (used in syslog messages).
 *
 * Note: daemonize() ignores SIGHUP during the second fork.
 * Before calling this function, reset SIGHUP to SIG_DFL so
 * the sigwait thread can see it:
 *
 *   struct sigaction sa;
 *   sa.sa_handler = SIG_DFL;
 *   sigemptyset(&sa.sa_mask);
 *   sa.sa_flags = 0;
 *   sigaction(SIGHUP, &sa, NULL);
 */
void daemon_signals_init_mt(reread_fn_t reread_fn,
                             const char *cmd);

/* ── Global signal mask (used by MT variant) ─────────────── */
extern sigset_t daemon_signal_mask;

#endif /* DAEMON_SIGNAL_H */
