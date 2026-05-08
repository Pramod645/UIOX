/*
 * main.c
 *
 * Demonstration program that exercises all daemon utilities:
 *
 *   1. Initialises itself as a proper daemon (daemonize).
 *   2. Enforces single-instance rule (already_running).
 *   3. Installs signal handlers (SIGHUP reload, SIGTERM exit).
 *   4. Logs startup message and sleeps in a loop.
 *
 * Compile: see Makefile
 * Run    : ./daemon_demo
 * Check  : ps -efj | grep daemon_demo
 *          cat /var/run/daemon.pid
 *          kill -HUP $(cat /var/run/daemon.pid)
 *          kill -TERM $(cat /var/run/daemon.pid)
 */

 #include "../include/daemon.h"
 #include "../include/daemon_log.h"
 #include "../include/daemon_lock.h"
 #include "../include/daemon_signal.h"
 #include <string.h>
 #include <libgen.h>
 
 /* ── Configuration reload callback ───────────────────────── */
 static void reread_config(void)
 {
     /* In a real daemon: parse /etc/<name>.conf here */
     daemon_log(LOG_INFO, "configuration reloaded");
 }
 
 /* ── Main entry point ────────────────────────────────────── */
 int main(int argc, char *argv[])
 {
     char *cmd;
     struct sigaction sa;
 
     /* Determine the bare program name (no path prefix) */
     cmd = strrchr(argv[0], '/');
     cmd = cmd ? cmd + 1 : argv[0];
 
     /* ── Step 1: Become a daemon (Figure 13.1) ─────────── */
     daemonize(cmd);
 
     /* ── Step 2: Enforce single instance (Figure 13.6) ─── */
     if (already_running()) {
         syslog(LOG_ERR, "%s: daemon already running", cmd);
         exit(1);
     }
 
     /*
      * ── Step 3: Reset SIGHUP to SIG_DFL ─────────────────
      * daemonize() left SIGHUP ignored (from the second fork).
      * Reset it so our signal handler / sigwait thread can see
      * it.  This step is required before calling either
      * daemon_signals_init_st() or daemon_signals_init_mt().
      */
     sa.sa_handler = SIG_DFL;
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = 0;
     if (sigaction(SIGHUP, &sa, NULL) < 0) {
         syslog(LOG_ERR, "%s: can't restore SIGHUP: %s",
                cmd, strerror(errno));
         exit(1);
     }
 
     /* ── Step 4: Install signal handlers ────────────────── */
 #ifdef DAEMON_MULTITHREADED
     /*
      * Multi-threaded variant (Figure 13.7):
      * Blocks all signals and spawns a dedicated thread that
      * uses sigwait() to receive and dispatch them.
      */
     daemon_signals_init_mt(reread_config, cmd);
 #else
     /*
      * Single-threaded variant (Figure 13.8):
      * Installs async signal handlers for SIGHUP and SIGTERM.
      */
     daemon_signals_init_st(reread_config, cmd);
 #endif
 
     /* ── Daemon main loop ────────────────────────────────── */
     syslog(LOG_INFO, "%s: started (pid=%ld)",
            cmd, (long)getpid());
 
     for (;;) {
         /*
          * Replace this with real work.
          * The daemon sleeps here and wakes on signal delivery
          * (single-threaded) or continues while the signal
          * thread handles signals (multi-threaded).
          */
         sleep(30);
     }
 
     /* Not reached */
     return 0;
 }
 