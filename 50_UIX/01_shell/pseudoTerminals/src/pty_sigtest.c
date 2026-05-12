/*
 * pty_sigtest.c — Exercise 19.9 (APUE §19.9)
 *
 * Demonstrates pty_fork() with signal handling in the child.
 *
 * The child (exec'd by pty_fork) catches:
 *   SIGTERM  — sent by parent via TIOCSIG/TIOCSIGNAL ioctl on master
 *   SIGWINCH — sent by kernel when parent changes slave window size
 *              via TIOCSWINSZ ioctl on master
 *
 * Parent sequence:
 *   1. pty_fork()
 *   2. Send SIGTERM to PTY slave's process group via ioctl.
 *   3. Read slave output to confirm SIGTERM was received.
 *   4. Change slave window size via TIOCSWINSZ on master.
 *   5. Read slave output to confirm SIGWINCH was received.
 *   6. Exit (causes slave to get SIGHUP from losing CTY).
 *
 * Platform note:
 *   Solaris:  TIOCSIGNAL ioctl to send signal via PTY master.
 *   Others:   TIOCSIG    ioctl to send signal via PTY master.
 */

 #include "../include/pty.h"
 #include "../include/pty_fork.h"
 
 /* ── Child program: catches SIGTERM and SIGWINCH ─────────── */
 
 static volatile sig_atomic_t term_caught  = 0;
 static volatile sig_atomic_t winch_caught = 0;
 
 static void child_sigterm(int signo)
 {
     (void)signo;
     term_caught = 1;
 }
 
 static void child_sigwinch(int signo)
 {
     (void)signo;
     winch_caught = 1;
 }
 
 /*
  * child_main() — runs in child after pty_fork().
  *
  * Installs handlers, then loops printing status when signals arrive.
  */
 static void child_main(void)
 {
     struct sigaction sa;
     struct winsize   ws;
 
     /* Install SIGTERM handler */
     sa.sa_handler = child_sigterm;
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = 0;
     sigaction(SIGTERM, &sa, NULL);
 
     /* Install SIGWINCH handler */
     sa.sa_handler = child_sigwinch;
     sigaction(SIGWINCH, &sa, NULL);
 
     printf("child: ready, pid=%ld\n", (long)getpid());
     fflush(stdout);
 
     /* Wait for signals */
     for (;;) {
         pause();  /* sleep until signal */
 
         if (term_caught) {
             term_caught = 0;
             printf("child: caught SIGTERM\n");
             fflush(stdout);
         }
 
         if (winch_caught) {
             winch_caught = 0;
             /* Read new window size from controlling terminal */
             if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
                 printf("child: caught SIGWINCH, "
                        "rows=%d cols=%d\n",
                        ws.ws_row, ws.ws_col);
             } else {
                 printf("child: caught SIGWINCH\n");
             }
             fflush(stdout);
         }
     }
 }
 
 /* ── Parent: sends signals and reads responses ────────────── */
 
 static void read_slave_output(int fdm)
 {
     char   buf[256];
     int    n;
     fd_set rset;
     struct timeval tv;
 
     FD_ZERO(&rset);
     FD_SET(fdm, &rset);
     tv.tv_sec  = 2;
     tv.tv_usec = 0;
 
     if (select(fdm + 1, &rset, NULL, NULL, &tv) <= 0)
         return;
 
     n = read(fdm, buf, sizeof(buf) - 1);
     if (n > 0) {
         buf[n] = '\0';
         fprintf(stderr, "parent read: %s", buf);
     }
 }
 
 int main(void)
 {
     int          fdm;
     pid_t        pid;
     char         slave_name[PTY_SLAVE_NAME_SZ];
     struct termios t;
     struct winsize ws;
 
     /* Fork with PTY */
     pid = pty_fork(&fdm, slave_name, sizeof(slave_name), NULL, NULL);
 
     if (pid < 0) {
         perror("pty_fork");
         exit(1);
     }
 
     if (pid == 0) {
         /* ── Child ──────────────────────────────────────── */
         child_main();   /* never returns */
         exit(0);
     }
 
     /* ── Parent ─────────────────────────────────────────── */
     fprintf(stderr, "parent: slave = %s, child pid = %ld\n",
             slave_name, (long)pid);
 
     /* Read child's "ready" message */
     read_slave_output(fdm);
 
     /* ── Step 1: Send SIGTERM via PTY master ioctl ─────── */
     {
         int sig = SIGTERM;
 #if defined(__sun)
         if (ioctl(fdm, TIOCSIGNAL, sig) < 0)
             perror("TIOCSIGNAL");
 #else
         if (ioctl(fdm, TIOCSIG, sig) < 0)
             perror("TIOCSIG");
 #endif
     }
 
     /* Read child's confirmation */
     read_slave_output(fdm);
 
     /* ── Step 2: Change slave window size ───────────────── */
     ws.ws_row    = 50;
     ws.ws_col    = 132;
     ws.ws_xpixel = 0;
     ws.ws_ypixel = 0;
 
     if (ioctl(fdm, TIOCSWINSZ, &ws) < 0)
         perror("TIOCSWINSZ");
 
     /* SIGWINCH is sent automatically by kernel to slave's fg group */
     read_slave_output(fdm);
 
     /* ── Step 3: Parent exits ───────────────────────────── */
     /*
      * When parent exits, the PTY master fd is closed.
      * The slave gets HUP because there are no more masters.
      * The slave's foreground process group receives SIGHUP.
      * The child (if it hasn't caught SIGHUP) will terminate.
      */
     fprintf(stderr, "parent: exiting\n");
     exit(0);
 }
 